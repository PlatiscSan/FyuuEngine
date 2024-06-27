
#include "pch.h"

#include "WindowsApplication.h"
#include "WindowsWindowManager.h"
#include "../Manager/MessageBus.h"
#include "Utility.h"

#include "FyuuEvent.h"

using namespace Fyuu;
using namespace Fyuu::windows_util;

Fyuu::WindowsWindow::WindowsWindow(std::string name, HINSTANCE handle)
	:IWindow(name), m_handle(handle) {

	String _name = std::get<String>(ConvertString(m_name)).c_str();

	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = m_handle;
	wcex.lpfnWndProc = WindowProc;
	wcex.lpszClassName = _name.c_str();

	
	RegisterClassEx(&wcex);

	m_hwnd = CreateWindowEx(0, wcex.lpszClassName, TEXT(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_handle, this);
	if (!m_hwnd) {
		UnregisterClass(wcex.lpszClassName, m_handle);
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}

	RECT rect{};
	GetWindowRect(m_hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
	m_x = rect.right;
	m_y = rect.top;
	

}

Fyuu::WindowsWindow::~WindowsWindow() noexcept {

	DestroyWindow(m_hwnd);
	UnregisterClass(std::get<String>(ConvertString(m_name)).c_str(), m_handle);
}

void Fyuu::WindowsWindow::SetTitle(std::string title) {
	if (!SetWindowText(m_hwnd, std::get<String>(ConvertString(title)).c_str())) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}
}

void Fyuu::WindowsWindow::SetSize(std::uint32_t width, std::uint32_t height) {
	if (!SetWindowPos(m_hwnd, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER)) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}
}

void Fyuu::WindowsWindow::SetPosition(int x, int y) {
	if (!SetWindowPos(m_hwnd, 0, x, y, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE)) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}
}

void Fyuu::WindowsWindow::Show() {
	ShowWindow(m_hwnd, SW_SHOW);
}


LRESULT Fyuu::WindowsWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {
	
	WindowsWindow* this_window = nullptr;

	if (msg == WM_NCCREATE) {
		this_window = static_cast<WindowsWindow*>(reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this_window));

		this_window->m_hwnd = hwnd;
	}
	else {
		this_window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (this_window) {
		return this_window->HandleMessage(msg, wparam, lparam);
	}
	else {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

}

LRESULT Fyuu::WindowsWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
	
	static Fyuu_WindowEvent window_event{};

	switch (msg) {

	case WM_DESTROY:
		window_event.window = this;
		window_event.type = FYUU_WINDOW_EVENT_DESTROY;
		MessageBus::GetInstance()->Publish(window_event);
		return DefWindowProc(m_hwnd, msg, wparam, lparam);

	case WM_SETFOCUS:
		window_event.window = this;
		window_event.type = FYUU_WINDOW_SET_FOCUS;
		MessageBus::GetInstance()->Publish(window_event);
		return DefWindowProc(m_hwnd, msg, wparam, lparam);

	case WM_KILLFOCUS:
		window_event.window = this;
		window_event.type = FYUU_WINDOW_LOSE_FOCUS;
		MessageBus::GetInstance()->Publish(window_event);
		return DefWindowProc(m_hwnd, msg, wparam, lparam);

	case WM_CLOSE:
		window_event.window = this;
		window_event.type = FYUU_WINDOW_EVENT_CLOSE;
		MessageBus::GetInstance()->Publish(window_event);
		return DefWindowProc(m_hwnd, msg, wparam, lparam);


	default:
		return DefWindowProc(m_hwnd, msg, wparam, lparam);
	}

}

IWindow* Fyuu::WindowsWindowManager::Create(std::string name) {

	std::shared_ptr<WindowsWindow> window;
	try {
		window = std::make_shared<WindowsWindow>(name, WindowsApplication::GetInstance(0, nullptr)->GetApplicationHandle());
	}
	catch (std::exception const&) {
		throw;
	}

	m_windows.push_back(window);
	return window.get();

}

IWindow* Fyuu::WindowsWindowManager::Find(std::string name) {

	for (auto& window : m_windows) {
		if (window->GetName() == name) {
			return window.get();
		}
	}

	return nullptr;

}

void WindowsWindowManager::Destroy(IWindow* _window) {

	auto it = std::find_if(m_windows.begin(), m_windows.end(),
		[&](std::shared_ptr<WindowsWindow> const& window) {
			return window.get() == _window;
		}
	);
	if (it != m_windows.end()) { 
		m_windows.erase(it);
	}

}

void WindowsWindowManager::Destroy(std::string name) {

	auto it = std::find_if(m_windows.begin(), m_windows.end(),
		[&](std::shared_ptr<WindowsWindow> const& window) {
			return window->GetName() == name;
		}
	);
	if (it != m_windows.end()) { 
		m_windows.erase(it);
	}

}
