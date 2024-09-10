
#include "pch.h"
#include "GameCore.h"
#include "Logger.h"
#include "GameTimer.h"
#include "ThreadPool.h"
#include "FileSystem.h"
#include "MessageBus.h"
#include "FyuuCore.h"
#include "MemoryManager.h"

using namespace Fyuu::core;
using namespace Fyuu::core::log;
using namespace Fyuu::core::thread;
using namespace Fyuu::core::filesystem;
using namespace Fyuu::core::performance;
using namespace Fyuu::core::message_bus;
using namespace Fyuu::core::memory;

static std::shared_ptr<Window> s_window = nullptr;
static std::function<void()> s_msg_loop;
static std::function<void()> s_update_callback;
static std::string s_last_error;
static std::atomic_bool s_is_quit;

void Fyuu::core::SetMainWindow(std::shared_ptr<Window> const& window) {
	s_window = window;
}

std::shared_ptr<Window> const& Fyuu::core::MainWindow() {
	return s_window;
}

void Fyuu::core::SetUpdateCallback(std::function<void()> const& callback) {
	s_update_callback = callback;
}

void Fyuu::core::RequestQuit() {
	s_is_quit = true;
}

bool Fyuu::core::UpdateApplication() {
	if (s_update_callback) {
		s_update_callback();
	}
	TimerTick();
	return !s_is_quit;
}

void Fyuu::core::SetMessageLoop(std::function<void()> const& msg_loop) {
	s_msg_loop = msg_loop;
}


FYUU_API int FYUU_CALL Fyuu_RunApplication(int argc, char** argv) {

	InitializeLog();
	Info("Log is initialized");
	InitializeThreads();
	AddSearchPath("Asset/");

	try {

#if defined(_WIN32)
		InitializeWindowsApp(argc, argv);
#elif defined(__linux__)
		InitializeLinuxApp(argc, argv);
#endif // defined(_WIN32)


	}
	catch (std::exception const& ex) {
		s_last_error = ex.what();
		Fatal(ex.what());
		ClearSubscriptions();
		StopThreads();
		QuitLog();
		return EXIT_FAILURE;
	}

	s_window->SetTitle("Fyuu Engine Window");
	s_window->Show();
	s_msg_loop();

	Info("App quit");
	ClearSubscriptions();
	StopThreads();
	QuitLog();
	return EXIT_SUCCESS;

}

FYUU_API void* FYUU_CALL Fyuu_Malloc(size_t size_in_bytes) {
	return MemoryAllocate(size_in_bytes);
}

FYUU_API void FYUU_CALL Fyuu_Free(void* p) {
	MemoryDeallocate(p);
}

FYUU_API char const* FYUU_CALL Fyuu_GetLastError() {
	return s_last_error.c_str();
}