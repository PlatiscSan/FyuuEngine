#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include "../Common/BaseWindow.h"

#include <Windows.h>
#include <windowsx.h>

namespace Fyuu {

	class WindowsWindow final : public BaseWindow {

	public:

		WindowsWindow(std::string name);

		void SetTitle(std::string title) override;
		void SetSize(std::uint32_t width, std::uint32_t height) override;
		void SetPosition(int x, int y) override;
		void Show() override;

	private:

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;
		
		LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

	private:

		HINSTANCE m_instance = GetModuleHandle(nullptr);
		HWND m_hwnd = nullptr;


	};

}

#endif // !WINDOWS_WINDOW_H
