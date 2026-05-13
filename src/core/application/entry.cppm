module;
#include <version>
#include <cstdlib>
#if !defined(__cpp_lib_modules)
#include <exception>
#include <mutex>
#include <format>
#include <print>
#endif // !defined(__cpp_lib_modules)
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED 0
#include <SDL3/SDL_main.h>
#include "api_macro.h"
#include "fyuu_application.h"
module fyuu_engine:entry;
import :app_instance;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

extern "C" {
	LIB_API int LIB_CALL Fyuu_Run(int argc, char** argv, Fyuu_App* app) {
		static std::mutex s_mutex;
		static Fyuu_App* s_app = nullptr;
		std::lock_guard<std::mutex> lock(s_mutex);
		s_app = app;
		return SDL_RunApp(
			argc, argv,
			[](int argc, char** argv) -> int {
				try {

					if (!fyuu_engine::application::Initialize(argc, argv, s_app)) {
						return EXIT_FAILURE;
					}

					if (fyuu_engine::application::ShowHelp()) {
						return EXIT_SUCCESS;
					}

					// Main loop
					while (fyuu_engine::application::IsRunning()) {
						fyuu_engine::application::Tick();
					}

					fyuu_engine::application::Shutdown();

					return EXIT_SUCCESS;
				}
				catch (std::exception const& ex) {
					std::println("Exception thrown in EngineMain(): {}\n", ex.what());
					return EXIT_FAILURE;
				}
				catch (...) {
					std::println("Unknown exception thrown in EngineMain()\n");
					return EXIT_FAILURE;
				}
			},
			nullptr
		);
	}
}
