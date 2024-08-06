#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include "Framework/Core/Window.h"
#include "WindowsUtility.h"

namespace Fyuu {

	class WindowsWindow final : public Window {

	public:

		WindowsWindow(std::string const& name);
		~WindowsWindow() noexcept;

		void SetTitle(std::string const& title) override;
		void SetSize(std::uint32_t width, std::uint32_t height) override;
		void SetPosition(int x, int y) override;
		void Show() override;

		void* GetNativeWindow() const noexcept { return m_hwnd; }
		HWND const& GetWindowHandle() const noexcept { return m_hwnd; }

	private:

		HWND m_hwnd;
		windows_util::String m_cname{};
		int m_x;
		int m_y;

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;
		LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

	};

}

#endif // !WINDOWS_WINDOW_H
