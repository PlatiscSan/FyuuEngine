
#include "pch.h"
#include "WindowsWindow.h"
#include "WindowsAppInstance.h"
#include "Framework/Core/MessageBus.h"
#include "Framework/Event/WindowEvent.h"

using namespace Fyuu::core;
using namespace Fyuu::core::message_bus;
using namespace Fyuu::utility::windows;

Fyuu::core::WindowsWindow::WindowsWindow(std::string const& name)
	:Window(name) {

	m_cname = std::move(std::get<String>(ConvertString(m_name)));

	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wcex.hInstance = WindowsAppInstance();
	wcex.lpfnWndProc = WindowProc;
	wcex.lpszClassName = m_cname.c_str();
	wcex.hbrBackground = GetStockBrush(NULL_BRUSH);


	RECT desktop_rect{};
	GetWindowRect(GetDesktopWindow(), &desktop_rect);

	AdjustWindowRect(&desktop_rect, WS_POPUP, false);
	int width = desktop_rect.right - desktop_rect.left;
	int height = desktop_rect.bottom - desktop_rect.top;

	if (!RegisterClassEx(&wcex)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	m_hwnd = CreateWindowEx(0, m_cname.c_str(), TEXT(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wcex.hInstance, this);
	if (!m_hwnd) {
		UnregisterClass(wcex.lpszClassName, wcex.hInstance);
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}

	RECT rect{};
	GetWindowRect(m_hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
	m_x = rect.right;
	m_y = rect.top;

}

Fyuu::core::WindowsWindow::~WindowsWindow() noexcept {
	UnregisterClass(m_cname.c_str(), WindowsAppInstance());
}

void Fyuu::core::WindowsWindow::SetTitle(std::string const& title) {
	if (!SetWindowText(m_hwnd, std::get<String>(ConvertString(title)).c_str())) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}
}

void Fyuu::core::WindowsWindow::SetSize(std::uint32_t width, std::uint32_t height) {
	if (!SetWindowPos(m_hwnd, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}
}

void Fyuu::core::WindowsWindow::SetPosition(int x, int y) {
	if (!SetWindowPos(m_hwnd, 0, x, y, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE)) {
		throw std::runtime_error(GetLastErrorMessageFromWinAPI());
	}
}

void Fyuu::core::WindowsWindow::Show() {
	ShowWindow(m_hwnd, SW_SHOW);
}


LRESULT Fyuu::core::WindowsWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

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

LRESULT Fyuu::core::WindowsWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {

	switch (msg) {

	case WM_CREATE:
		break;

	case WM_DESTROY:
		Publish(WindowDestroyEvent(this));
		break;
	case WM_SIZE:
		break;
	case WM_SETFOCUS:
		break;
	case WM_KILLFOCUS:
		break;

	case WM_CLOSE:
		break;

	case WM_SHOWWINDOW:
		break;

	default:
		break;
	}

	return DefWindowProc(m_hwnd, msg, wparam, lparam);

}
