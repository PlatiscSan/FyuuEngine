module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>
#endif // WIN32

module windows_window;
import win32exception;

namespace fyuu_engine::windows {
#ifdef WIN32
	/*
	*	WindowsWindow
	*/

	LRESULT CALLBACK WindowsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		static std::size_t active_window = 0u;

		if (msg == WM_CREATE) {
			auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return 0;
		}

		auto window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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

	typename WindowsWindow::ClassName WindowsWindow::GenerateClassName() {
		ClassName classname{};
		boost::uuids::to_chars(GenerateUUID(), classname.begin());
		return classname;
	}

	HWND WindowsWindow::CreateWindowImpl(
		WindowsWindow* window,
		std::string_view title,
		std::uint32_t width,
		std::uint32_t height
	) {

		HINSTANCE instance = GetModuleHandle(nullptr);
		WNDCLASSEX wc{};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WindowsWindow::WndProc;
		wc.hInstance = instance;
		wc.lpszClassName = window->m_class_name.data();

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

	void WindowsWindow::OnDestroy() noexcept {
		m_message_bus.Publish<WindowCloseEvent>(this);
	}

	void WindowsWindow::OnResize(LPARAM lparam) noexcept {
		UINT width = LOWORD(lparam);
		UINT height = HIWORD(lparam);
		m_message_bus.Publish<WindowResizeEvent>(this, width, height);
	}

	void WindowsWindow::OnKeyDown(WPARAM wparam, LPARAM lparam) noexcept {

		bool repeat = (lparam & 0x40000000) != 0;
		KeyboardEvent::Action action = repeat ?
			KeyboardEvent::Action::Repeat :
			KeyboardEvent::Action::Press;

		m_message_bus.Publish<KeyboardEvent>(this, static_cast<int>(wparam), action);

	}

	void WindowsWindow::OnKeyUp(WPARAM wparam, LPARAM lparam) noexcept {
		m_message_bus.Publish<KeyboardEvent>(
			this,
			static_cast<int>(wparam),
			KeyboardEvent::Action::Release
		);
	}

	void WindowsWindow::OnMouseButtonDown(UINT msg, LPARAM lparam) noexcept {

		MouseButtonEvent::Button button = MouseButtonEvent::Button::Left;

		if (msg == WM_RBUTTONDOWN) {
			button = MouseButtonEvent::Button::Right;
		}
		if (msg == WM_MBUTTONDOWN) {
			button = MouseButtonEvent::Button::Middle;
		}

		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		m_message_bus.Publish<MouseButtonEvent>(
			this,
			static_cast<double>(x),
			static_cast<double>(y),
			button,
			MouseButtonEvent::Action::Press
		);
	}

	void WindowsWindow::OnMouseButtonUp(UINT msg, LPARAM lparam) noexcept {

		MouseButtonEvent::Button button = MouseButtonEvent::Button::Left;
		if (msg == WM_RBUTTONUP) {
			button = MouseButtonEvent::Button::Right;
		}
		if (msg == WM_MBUTTONUP) {
			button = MouseButtonEvent::Button::Middle;
		}

		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		m_message_bus.Publish<MouseButtonEvent>(
			this,
			static_cast<double>(x),
			static_cast<double>(y),
			button,
			MouseButtonEvent::Action::Release
		);

	}

	void WindowsWindow::OnMouseMove(LPARAM lparam) noexcept {
		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);

		m_message_bus.Publish<MouseMoveEvent>(
			this,
			static_cast<double>(x),
			static_cast<double>(y)
		);
	}

	LRESULT WindowsWindow::DefaultHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

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


		case WM_KEYUP:
		case WM_SYSKEYUP:
			OnKeyUp(wparam, lparam);
			break;

		case WM_MOUSEMOVE:
			OnMouseMove(lparam);
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
			/*
			*	pass message to DefWindowProc()
			*/
			return 0;
		}

		/*
		*	Do not pass message to DefWindowProc()
		*/
		return 1;

	}

	void WindowsWindow::ShowImpl() {
		ShowWindow(m_hwnd, SW_SHOW);
	}

	void WindowsWindow::HideImpl() {
		ShowWindow(m_hwnd, SW_HIDE);
	}

	void WindowsWindow::ProcessEventsImpl() {

		MSG msg{};

		while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		};

	}

	void WindowsWindow::SetTitleImpl(std::string_view title) {
		auto compatible_title = StringToTString(title);
		SetWindowText(m_hwnd, compatible_title.data());
	}

	std::pair<std::uint32_t, std::uint32_t> WindowsWindow::GetWidthAndHeightImpl() const noexcept {
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	HWND WindowsWindow::GetHWND() const noexcept {
		return m_hwnd;
	}

	WindowsWindow::WindowsWindow(std::string_view title, std::uint32_t width, std::uint32_t height)
		: Base(),
		m_msg_processors(
			{
				{
					GenerateUUID(),
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { 
						return DefaultHandler(hwnd,msg,wparam,lparam);
					}
				}
			}
		),
		m_class_name(WindowsWindow::GenerateClassName()),
		m_hwnd(WindowsWindow::CreateWindowImpl(this, title, width, height)) {

	}

	WindowsWindow::~WindowsWindow() noexcept {
		UnregisterClass(m_class_name.data(), GetModuleHandle(nullptr));
	}

	boost::uuids::uuid WindowsWindow::AttachMsgProcessor(MsgProcessor const& processor) {
		auto uuid = GenerateUUID();
		(void)m_msg_processors.emplace_back(uuid, processor);
		return uuid;
	}

	boost::uuids::uuid WindowsWindow::StrictAttachMsgProcessor(MsgProcessor const& processor) {
		auto uuid = GenerateUUID();
		auto modifier = m_msg_processors.LockedModifier();
		(void)modifier.emplace_back(uuid, processor);
		return uuid;
	}

	void WindowsWindow::DetachBackMsgProcessor() {
		if (m_msg_processors.size() > 1u) {
			// if size() == 1, only default handler in the vector, then do nothing.
			(void)m_msg_processors.pop_back();
		}
	}

	void WindowsWindow::DetachMsgProcessor(boost::uuids::uuid const& uuid) {
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

#endif // WIN32
}
