#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include "../Interface/IWindowManager.h"
#include "../Common/Singleton.h"

#include <Windows.h>
#include <windowsx.h>

namespace Fyuu {

	class WindowsWindow final : public IWindow {

	public:

		WindowsWindow(std::string name, HINSTANCE handle);
		~WindowsWindow() noexcept;

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

	class WindowsWindowManager : public IWindowManager, public Singleton<WindowsWindowManager> {

		friend class Singleton<WindowsWindowManager>;

	public:

		IWindow* Create(std::string name) override;
		IWindow* Find(std::string name) override;
		void Destroy(IWindow* window) override;
		void Destroy(std::string name) override;

	private:

		WindowsWindowManager() = default;

	private:

		std::vector<std::shared_ptr<WindowsWindow>> m_windows;

	};

}

#endif // !WINDOWS_WINDOW_H
