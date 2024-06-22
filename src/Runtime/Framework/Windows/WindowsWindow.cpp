
#include "pch.h"

#include <stdexcept>

#include "WindowsWindow.h"
#include "Utility.h"



using namespace Fyuu;
using namespace Fyuu::windows_util;

Fyuu::WindowsWindow::WindowsWindow(std::string name)
	:BaseWindow(name) {

	WNDCLASSEXA wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = m_instance;
	wcex.lpfnWndProc = WindowProc;
	
	RegisterClassExA(&wcex);

	m_hwnd = CreateWindowExA(0, name.c_str(), "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_instance, this);
	if (!m_hwnd) {
		UnregisterClassA(name.c_str(), m_instance);
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}

	RECT rect{};
	GetWindowRect(m_hwnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
	m_x = rect.right;
	m_y = rect.top;
	

}

void Fyuu::WindowsWindow::SetTitle(std::string title) {
	if (!SetWindowTextA(m_hwnd, title.c_str())) {
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
	if (!ShowWindow(m_hwnd, SW_SHOW)) {
		throw std::runtime_error(GetLastErrorFromWinAPI());
	}
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
	return LRESULT();
}
