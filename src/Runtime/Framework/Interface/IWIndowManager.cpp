
#include "pch.h"

#include "FyuuGraphics.h"

#if defined(_WIN32)
	#include "../Windows/WindowsWindowManager.h"
#else
	#include "IWindowManager.h"
#endif // defined(_WIN32)

using namespace Fyuu;

#if defined(_WIN32)

FYUU_API Fyuu_Window Fyuu_CreateWindow(char const* name) {

	try {
		return WindowsWindowManager::GetInstance()->Create(name);
	}
	catch (std::exception const& ex) {
		MessageBoxA(nullptr, ex.what(), "Creation Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}
	
}

FYUU_API Fyuu_Window Fyuu_FindWindow(char const* name) {
	return WindowsWindowManager::GetInstance()->Find(name);
}

FYUU_API void Fyuu_SetWindowTitle(Fyuu_Window _window, char const* title) {

	WindowsWindow* window = static_cast<WindowsWindow*>(_window);
	try {
		window->SetTitle(title);
	}
	catch (std::exception const& ex) {
		MessageBoxA(nullptr, ex.what(), "Window Error", MB_OK | MB_ICONERROR);
	}

}

FYUU_API void Fyuu_SetWindowSize(Fyuu_Window _window, uint32_t width, uint32_t height) {

	WindowsWindow* window = static_cast<WindowsWindow*>(_window);
	try {
		window->SetSize(width, height);
	}
	catch (std::exception const& ex) {
		MessageBoxA(nullptr, ex.what(), "Window Error", MB_OK | MB_ICONERROR);
	}

}

FYUU_API void Fyuu_SetWindowPosition(Fyuu_Window _window, int x, int y) {

	WindowsWindow* window = static_cast<WindowsWindow*>(_window);
	try {
		window->SetPosition(x, y);
	}
	catch (std::exception const& ex) {
		MessageBoxA(nullptr, ex.what(), "Window Error", MB_OK | MB_ICONERROR);
	}
}

FYUU_API void Fyuu_ShowWindow(Fyuu_Window _window) {

	WindowsWindow* window = static_cast<WindowsWindow*>(_window);
	window->Show();

}

FYUU_API void Fyuu_DestroyWindow(Fyuu_Window window) {
	WindowsWindowManager::GetInstance()->Destroy(static_cast<WindowsWindow*>(window));
}

FYUU_API char const* Fyuu_GetWindowName(Fyuu_Window _window) {
	WindowsWindow* window = static_cast<WindowsWindow*>(_window);
	return window->GetName().c_str();
}

#else


#endif // defined(_WIN32)