
#include "pch.h"
#include "WindowsWindow.h"
#include "AppInstance.h"
#include "Framework/Event/MessageBus.h"

using namespace Fyuu::windows_util;

Fyuu::WindowsWindow::WindowsWindow(std::string const& name)
	:Window(name) {

	m_cname = std::move(std::get<String>(ConvertString(m_name)));

	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wcex.hInstance = AppInstance();
	wcex.lpfnWndProc = WindowProc;
	wcex.lpszClassName = m_cname.c_str();
	wcex.hbrBackground = GetStockBrush(NULL_BRUSH);


	RECT desktop_rect{};
	GetWindowRect(GetDesktopWindow(), &desktop_rect);

	AdjustWindowRect(&desktop_rect, WS_POPUP, false);
	int width = desktop_rect.right - desktop_rect.left;
	int height = desktop_rect.bottom - desktop_rect.top;

	if (!RegisterClassEx(&wcex)) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}

	m_hwnd = CreateWindowEx(0, m_cname.c_str(), TEXT(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wcex.hInstance, this);
	if (!m_hwnd) {
		UnregisterClass(wcex.lpszClassName, wcex.hInstance);
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
	UnregisterClass(m_cname.c_str(), AppInstance());
}

void Fyuu::WindowsWindow::SetTitle(std::string const& title) {
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

	switch (msg) {

	case WM_NCCREATE:

		this_window = static_cast<WindowsWindow*>(reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this_window));
		this_window->m_hwnd = hwnd;

		break;

	default:
		this_window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		break;
	}

	return this_window ? this_window->HandleMessage(msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);

}

LRESULT Fyuu::WindowsWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {

	switch (msg) {

	case WM_CREATE:
		break;

	case WM_DESTROY:
		MessageBus::GetInstance().Publish(WindowDestroyEvent(this));
		break;
	case WM_SIZE:
		break;
	case WM_SETFOCUS:
		break;
	case WM_KILLFOCUS:
		break;

	case WM_CLOSE:
		MessageBus::GetInstance().Publish(WindowCloseEvent(this));
		break;

	case WM_SHOWWINDOW:
		break;

	case WM_KEYDOWN:
		break;
	case WM_KEYUP:
		break;

	default:
		break;
	}

	return DefWindowProc(m_hwnd, msg, wparam, lparam);

}
