
#include "pch.h"

#include "FyuuApp.h"
#include "../Manager/MessageBus.h"

#if defined(_WIN32)
	#include "../Windows/WindowsApplication.h"
	#include "../Windows/WindowsWindowManager.h"
#else
	#include "IApplication.h"
	#include "IWindowManager.h"
#endif // defined(_WIN32)

using namespace Fyuu;

#if defined(_WIN32)

FYUU_API Fyuu_error_t Fyuu_Init(int argc, char* argv[]) {

	try {
		WindowsApplication::GetInstance(argc, argv);
		MessageBus::GetInstance();
		WindowsWindowManager::GetInstance();
	}
	catch (std::exception const& ex) {
		MessageBoxA(nullptr, ex.what(), "Init Failed", MB_OK | MB_ICONERROR);
		return FYUU_INIT_FAILED;
	}

	return FYUU_SUCCESS;

}

FYUU_API void Fyuu_Tick() {
	WindowsApplication::GetInstance(0, nullptr)->Tick();
}

FYUU_API bool Fyuu_IsQuit() {
	return WindowsApplication::GetInstance(0, nullptr)->IsQuit();;
}

FYUU_API void Fyuu_RequestQuit() {
	WindowsApplication::GetInstance(0, nullptr)->RequestQuit();
}

FYUU_API void Fyuu_Quit() {
	WindowsApplication::DestroyInstance();
}

#endif // defined(_WIN32)

