module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>
#endif // WIN32

export module windows_window;
export import window_interface;

export namespace core {
#ifdef WIN32
	class Win32Exception : public std::exception {
	private:
		std::vector<TCHAR> m_error_message;
#ifdef UNICODE
		mutable std::shared_ptr<char[]> m_converted;
#endif // UNICODE

	public:
		explicit Win32Exception(DWORD error_code = ::GetLastError())
			: m_error_message() {

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
				m_error_message.resize(length);
				std::memcpy(m_error_message.data(), buffer, length);

			}

		}

		TCHAR const* ErrorMessage() const noexcept {
			return m_error_message.data();
		}

		char const* what() const noexcept override {
#ifdef UNICODE
			if (m_converted) {
				return m_converted.get();
			}

			LPCWSTR src = m_error_message.data();
			std::size_t src_len = std::wcslen(src);

			int utf8_len = WideCharToMultiByte(
				CP_UTF8,
				0,
				reinterpret_cast<LPCWSTR>(src), 
				static_cast<int>(src_len),
				nullptr,
				0,
				nullptr, 
				nullptr
			);
			if (utf8_len <= 0) {
				return "Win32Exception: Encoding conversion failed.\0";
			}

			m_converted = std::make_shared_for_overwrite<char[]>(static_cast<std::size_t>(utf8_len) + 1);
			WideCharToMultiByte(
				CP_UTF8, 
				0,
				reinterpret_cast<LPCWSTR>(src), 
				static_cast<int>(src_len),
				m_converted.get(), 
				utf8_len,
				nullptr, 
				nullptr
			);
			m_converted[utf8_len] = '\0';
			return m_converted.get();
#else
			return m_error_message.data();
#endif
		}


	};

	class WindowsWindow : public IWindow {
	private:
		std::array<TCHAR, 36u> m_wc_name;
		HWND m_hwnd = nullptr;
		util::MessageBus m_message_bus;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

			if (msg == WM_CREATE) {
				auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
				return 0;
			}

			auto window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!window) {
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}

			switch (msg) {
			case WM_CLOSE:
			{
				WindowCloseEvent window_close(window);
				window->m_message_bus.Publish(window_close);
				break;
			}
			case WM_SIZE:
			{
				UINT width = LOWORD(lparam);
				UINT height = HIWORD(lparam);
				WindowResizeEvent window_resize(window, width, height);
				window->m_message_bus.Publish(window_resize);
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN: 
			{
				bool repeat = (lparam & 0x40000000) != 0;
				KeyEvent::Action action = repeat ?
					KeyEvent::Action::Repeat : KeyEvent::Action::Press;
				KeyEvent key(window, static_cast<int>(wparam), action);
				window->m_message_bus.Publish(key);
				break;
			}

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			{
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
					window,
					static_cast<double>(x),
					static_cast<double>(y),
					button,
					MouseButtonEvent::Action::Press
				);

				window->m_message_bus.Publish(mouse_button);
				break;
			}

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP: 
			{
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
					window,
					static_cast<double>(x),
					static_cast<double>(y),
					button,
					MouseButtonEvent::Action::Release
				);

				window->m_message_bus.Publish(mouse_button);
				break;
			}

			default:
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}

			return 0;

		}

		static std::wstring ConvertToWideChar(std::string_view multi_bytes_str) {

			std::wstring result_str;

			if (multi_bytes_str.empty()) {
				return result_str;
			}

			int wchar_len = MultiByteToWideChar(
				CP_ACP,
				0,
				multi_bytes_str.data(),
				-1,
				nullptr,
				0 
			);

			if (wchar_len <= 0) {
				throw Win32Exception();
			}

			result_str.resize(wchar_len, 0);

			int result = MultiByteToWideChar(
				CP_ACP,
				0,
				multi_bytes_str.data(),
				-1,
				result_str.data(),
				wchar_len
			);

			if (result == 0) {
				throw Win32Exception();
			}

			return result_str;
		}

	public:
		void Close() noexcept override {
			if (m_hwnd) {
				DestroyWindow(m_hwnd);
				m_hwnd = nullptr;
				UnregisterClass(m_wc_name.data(), GetModuleHandle(nullptr));
			}
		}

		~WindowsWindow() noexcept override {
			Close();
		}

		void Create(std::string_view title, std::uint32_t width, std::uint32_t height) override {

			if (m_hwnd) {
				return;
			}

			static boost::uuids::random_generator gen;
			boost::uuids::to_chars(gen(), m_wc_name.begin());

			HINSTANCE instance = GetModuleHandle(nullptr);
			WNDCLASSEX wc{};
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.lpfnWndProc = WindowsWindow::WndProc;
			wc.hInstance = instance;
			wc.lpszClassName = m_wc_name.data();
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

			if (!RegisterClassEx(&wc)) {
				throw Win32Exception();
			}

#ifdef UNICODE 
			std::wstring wchar_title = WindowsWindow::ConvertToWideChar(title);
			auto compatible_title = !wchar_title.empty() ? wchar_title.data() : L"Default title";
#else
			auto compatible_title = !title.empty() ? title.data() : "Default title";
#endif // UNICODE

			m_hwnd = CreateWindowEx(
				0,
				wc.lpszClassName, 
				compatible_title,
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, 
				CW_USEDEFAULT, 
				width, 
				height,
				nullptr, 
				nullptr, 
				instance,
				this
			);

			if (!m_hwnd) {
				throw Win32Exception();
			}

		}

		void Show() override {
			ShowWindow(m_hwnd, SW_SHOW);
		}

		void Hide() override {
			ShowWindow(m_hwnd, SW_HIDE);
		}

		void ProcessEvents() override {
			MSG msg{};
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		void SetTitle(std::string_view title) override {
#ifdef UNICODE 
			std::wstring wchar_title = WindowsWindow::ConvertToWideChar(title);
			auto compatible_title = !wchar_title.empty() ? wchar_title.data() : L"Default title";
#else
			auto compatible_title = !title.empty() ? title.data() : "Default title";
#endif // UNICODE
			SetWindowText(m_hwnd, compatible_title);
		}

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeight() const noexcept override {
			RECT rect;
			GetClientRect(m_hwnd, &rect);
			return { rect.right - rect.left, rect.bottom - rect.top };
		}

		void* Native() const noexcept override {
			return m_hwnd;
		}

		util::MessageBus* GetMessageBus() noexcept override {
			return &m_message_bus;
		}

	};

#endif 

}