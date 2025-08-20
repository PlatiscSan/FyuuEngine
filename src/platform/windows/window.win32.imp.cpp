module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>
#endif // WIN32

module window:win32;
import event_system;

namespace platform {
	concurrency::SynchronizedFunction<boost::uuids::uuid()> GenerateUUID = []() {
		static boost::uuids::random_generator gen;
		return gen();
		};

#ifdef WIN32
#ifdef UNICODE
	std::string TStringToString(std::wstring_view src) {

		int utf8_len = WideCharToMultiByte(
			CP_UTF8,
			0,
			reinterpret_cast<LPCWSTR>(src.data()),
			static_cast<int>(src.size()),
			nullptr,
			0,
			nullptr,
			nullptr
		);
		if (utf8_len <= 0) {
			return "Win32Exception: Encoding conversion failed.\0";
		}

		std::string output(static_cast<std::size_t>(utf8_len) + 1u, 0);
		WideCharToMultiByte(
			CP_UTF8,
			0,
			reinterpret_cast<LPCWSTR>(src.data()),
			static_cast<int>(src.size()),
			output.data(),
			utf8_len,
			nullptr,
			nullptr
		);
		output[utf8_len] = '\0';
		return output;
	}

	std::wstring StringToTString(std::string_view src) {
		if (src.empty()) {
			return std::basic_string<TCHAR>();
		}

		int wide_len = MultiByteToWideChar(
			CP_UTF8,
			0,
			src.data(),
			static_cast<int>(src.size()),
			nullptr,
			0
		);

		if (wide_len <= 0) {
			return _T("Win32Exception: Encoding conversion failed.");
		}

		std::basic_string<TCHAR> output;
		output.resize(static_cast<size_t>(wide_len));

		int result = MultiByteToWideChar(
			CP_UTF8,
			0,
			src.data(),
			static_cast<int>(src.size()),
			output.data(),
			wide_len
		);

		if (result == 0) {
			return _T("Win32Exception: Encoding conversion failed.");
		}

		return output;

	}
#endif

	/*
	*	Win32Exception
	*/

	Win32Exception::Win32Exception(DWORD error_code) 
		: m_error_message()
#ifdef UNICODE
		, m_multi_bytes_error_message()
#endif // UNICODE
	{

		TCHAR* buffer = nullptr;
		DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS;

		DWORD size = FormatMessage(
			flags,
			nullptr,
			error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<TCHAR*>(&buffer),
			0,
			nullptr
		);

		if (size > 0 && buffer != nullptr) {

			std::size_t length = static_cast<std::size_t>(size) + 1;
			m_error_message.resize(length);
			std::memcpy(m_error_message.data(), buffer, length);
			LocalFree(buffer);

		}
		else {
			TCHAR const default_msg[] = TEXT("Unknown Win32 error\0");
			constexpr std::size_t length = sizeof(default_msg) / sizeof(TCHAR);
			m_error_message.resize(length, 0);
			std::memcpy(m_error_message.data(), default_msg, length);

		}

	}

	TCHAR const* Win32Exception::What() const noexcept {
		return m_error_message.data();
	}

	char const* Win32Exception::what() const noexcept {
#ifdef UNICODE
		if (m_multi_bytes_error_message.empty()) {
			m_multi_bytes_error_message = TStringToString(m_error_message.data());
		}
		return m_multi_bytes_error_message.data();
#else
		return m_error_message.data();
#endif // UNICODE
	}

	/*
	*	Win32Window
	*/

	LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		static std::size_t active_window = 0u;

		if (msg == WM_CREATE) {
			auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return 0;
		}

		auto window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (!window) {
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}

		{
			auto locked_view = window->m_msg_processors.LockedView();
			for (auto iter = locked_view.crbegin(); iter != locked_view.crend(); ++iter) {
				auto const& [_, handler] = (*iter);
				LRESULT result = handler(hwnd, msg, wparam, lparam);
				if (result != 0) {
					/*
					*	When the result is not zero, it indicates that the message has been handled.
					*	Returning the result allows the message to be processed by the caller.
					*/
					return result;
				}
			}
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);

	}

	typename Win32Window::ClassName Win32Window::GenerateClassName() {
		ClassName classname{};
		boost::uuids::to_chars(GenerateUUID(), classname.begin());
		return classname;
	}

	HWND Win32Window::CreateWindowImp(
		Win32Window* window,
		std::string_view title,
		std::uint32_t width,
		std::uint32_t height
	) {

		HINSTANCE instance = GetModuleHandle(nullptr);
		WNDCLASSEX wc{};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = Win32Window::WndProc;
		wc.hInstance = instance;
		wc.lpszClassName = window->m_class_name.data();
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		if (!RegisterClassEx(&wc)) {
			throw Win32Exception();
		}

		auto compatible_title = StringToTString(title);

		auto hwnd = CreateWindowEx(
			0,
			wc.lpszClassName,
			compatible_title.data(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			nullptr,
			nullptr,
			instance,
			window
		);

		if (!hwnd) {
			UnregisterClass(window->m_class_name.data(), GetModuleHandle(nullptr));
			throw Win32Exception();
		}

		return hwnd;

	}

	void Win32Window::OnDestroy() noexcept {
		WindowCloseEvent window_close(this);
		m_message_bus.Publish(window_close);
	}

	void Win32Window::OnResize(LPARAM lparam) noexcept {
		UINT width = LOWORD(lparam);
		UINT height = HIWORD(lparam);
		WindowResizeEvent window_resize(this, width, height);
		m_message_bus.Publish(window_resize);
	}

	void Win32Window::OnKeyDown(WPARAM wparam, LPARAM lparam) noexcept {
		bool repeat = (lparam & 0x40000000) != 0;
		KeyEvent::Action action = repeat ?
			KeyEvent::Action::Repeat : KeyEvent::Action::Press;
		KeyEvent key(this, static_cast<int>(wparam), action);
		m_message_bus.Publish(key);
	}

	void Win32Window::OnMouseButtonDown(UINT msg, LPARAM lparam) noexcept {
		MouseButtonEvent::Button button = MouseButtonEvent::Button::Left;
		if (msg == WM_RBUTTONDOWN) {
			button = MouseButtonEvent::Button::Right;
		}
		if (msg == WM_MBUTTONDOWN) {
			button = MouseButtonEvent::Button::Middle;
		}

		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		MouseButtonEvent mouse_button(
			this,
			static_cast<double>(x),
			static_cast<double>(y),
			button,
			MouseButtonEvent::Action::Press
		);

		m_message_bus.Publish(mouse_button);
	}

	void Win32Window::OnMouseButtonUp(UINT msg, LPARAM lparam) noexcept {

		MouseButtonEvent::Button button = MouseButtonEvent::Button::Left;
		if (msg == WM_RBUTTONUP) {
			button = MouseButtonEvent::Button::Right;
		}
		if (msg == WM_MBUTTONUP) {
			button = MouseButtonEvent::Button::Middle;
		}

		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		MouseButtonEvent mouse_button(
			this,
			static_cast<double>(x),
			static_cast<double>(y),
			button,
			MouseButtonEvent::Action::Release
		);

		m_message_bus.Publish(mouse_button);

	}

	LRESULT Win32Window::DefaultHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		switch (msg) {
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			OnDestroy();
			break;
		case WM_SIZE:
			OnResize(lparam);
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			OnKeyDown(wparam, lparam);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			OnMouseButtonDown(msg, lparam);
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			OnMouseButtonUp(msg, lparam);
			break;

		default:
			return 0;
		}

		return 1;

	}

	Win32Window::Win32Window(std::string_view title, std::uint32_t width, std::uint32_t height)
		: m_msg_processors(
			{
				{
					GenerateUUID(),
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { 
						return DefaultHandler(hwnd,msg,wparam,lparam);
					}
				}
			}
		),
		m_message_bus(),
		m_class_name(Win32Window::GenerateClassName()),
		m_hwnd(Win32Window::CreateWindowImp(this, title, width, height)) {

	}

	Win32Window::~Win32Window() noexcept {
		UnregisterClass(m_class_name.data(), GetModuleHandle(nullptr));
	}

	void Win32Window::DetachBackMsgProcessor() {
		if (m_msg_processors.size() > 1u) {
			// if size() == 1, only default handler in the vector, then do nothing.
			(void)m_msg_processors.pop_back();
		}
	}

	void Win32Window::DetachMsgProcessor(boost::uuids::uuid const& uuid) {
		auto modifier = m_msg_processors.LockedModifier();
		std::size_t index = 0;
		for (auto iter = modifier.begin(); iter != modifier.end(); ++iter) {
			auto& [element_uuid, _] = (*iter);
			if (element_uuid == uuid) {
				modifier.erase(iter);
				return;
			}
		}
	}

	void Win32Window::Show() {
		ShowWindow(m_hwnd, SW_SHOW);
	}

	void Win32Window::Hide() {
		ShowWindow(m_hwnd, SW_HIDE);
	}

	void Win32Window::ProcessEvents() {

		MSG msg{};
		bool has_message;
		do {
			has_message = PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE);
			if (has_message) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} while (has_message);

	}

	void Win32Window::SetTitle(std::string_view title) {
		auto compatible_title = StringToTString(title);
		SetWindowText(m_hwnd, compatible_title.data());
	}

	std::pair<std::uint32_t, std::uint32_t> Win32Window::GetWidthAndHeight() const noexcept {
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	void* Win32Window::Native() const noexcept {
		return m_hwnd;
	}

	HWND Win32Window::GetHWND() const noexcept {
		return m_hwnd;
	}

	util::MessageBus* Win32Window::GetMessageBus() noexcept {
		return &m_message_bus;
	}

#endif
}
