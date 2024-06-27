#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include "../Interface/IWindowManager.h"

#include <Windows.h>
#include <windowsx.h>

namespace Fyuu {

	class WindowsWindow final : public IWindow {

	public:

		WindowsWindow(std::string name, HINSTANCE handle);

		void SetTitle(std::string title) override;
		void SetSize(std::uint32_t width, std::uint32_t height) override;
		void SetPosition(int x, int y) override;
		void Show() override;

	private:

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;
		
		LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

	private:

		HINSTANCE m_handle = nullptr;
		HWND m_hwnd = nullptr;


	};


}

#endif // !WINDOWS_WINDOW_H
