module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <filesystem>
#include <format>
#include <print>
#endif // !defined(__cpp_lib_modules)
#include "fyuu_application.h"
#include <CLI/CLI.hpp>
#include <SDL3/SDL.h>
export module fyuu_engine:app_instance;
#if defined(__cpp_lib_modules)	
import std;
#endif // defined(__cpp_lib_modules)
import :log;
import :renderer_instance;

namespace fs = std::filesystem;

namespace {
	bool s_show_help = false;	
	Fyuu_App* s_app = nullptr;
	SDL_Window* s_main_surface = nullptr; 
	bool s_is_running = true;

	void CreateMainSurface() {

		static constexpr SDL_WindowFlags window_flags =
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
		
		s_main_surface = SDL_CreateWindow(s_app->title, s_app->surface_width, s_app->surface_height, window_flags);
		if (!s_main_surface) {
			throw std::runtime_error(std::format("Calling SDL_CreateWindow(), SDL reports {}", SDL_GetError()));
		}

	}

	void DestroyMainSurface() {
		if (s_main_surface) {
			SDL_DestroyWindow(s_main_surface);
			s_main_surface = nullptr;
		}
	}

	void OnResize() {
		int surface_width, surface_height;
		if (!SDL_GetWindowSize(s_main_surface, &surface_width, &surface_height)) {
			throw std::runtime_error(std::format("Calling SDL_GetWindowSize(), SDL reports {}", SDL_GetError()));
		}
		s_app->surface_width = static_cast<std::uint32_t>(surface_width);
		s_app->surface_height = static_cast<std::uint32_t>(surface_height);
	}

}

namespace fyuu_engine::application {

	export bool Initialize(int argc, char** argv, Fyuu_App* app) {

		log::Initialize();

		CLI::App cli_app(app->description, app->name);

		fs::path conf_path;
		std::string graphics_api;

		cli_app.add_option(
			"--API", graphics_api,
			"Graphics API, can be Vulkan, D3D12, Metal, WebGPU, OpenGL"
		)->default_val("PlatformDefault");

		cli_app.add_option(
			"--conf", conf_path,
			"Configuration file path (YAML or JSON)"
		)->default_val("./conf.yaml");

		try {

			cli_app.parse(argc, argv);
			std::transform(
				graphics_api.begin(),
				graphics_api.end(),
				graphics_api.begin(),
				[](unsigned char c) -> unsigned char {
					return static_cast<unsigned char>(std::tolower(c));
				}
			);

			s_app = app;
			CreateMainSurface();
			rendering::Initialize(s_main_surface, s_app, graphics_api);

			log::Info(std::format("Engine initialized with graphics API: '{}', configuration file: '{}'", graphics_api, conf_path.string()));

			if (s_app->Init) {
				s_app->Init(s_app);
			}

			return true;
		}
		catch (CLI::CallForHelp const&) {
			s_show_help = true;
			std::println("{}", cli_app.help());
			return true;
		}
		catch (std::exception const& ex) {
			log::Fatal(std::format("Initialize() error occurred: {}\n", ex.what()));
			return false;
		}

	}

	export bool ShowHelp() {
		return s_show_help;
	}

	export void Tick() {
		static SDL_Event event;
		static bool should_tick = true;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EventType::SDL_EVENT_QUIT:
				s_is_running = false;
				break;
			case SDL_EventType::SDL_EVENT_WINDOW_RESIZED:
				OnResize();
				break;
			case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED:
				OnResize();
				should_tick = false;
				break;
			case SDL_EventType::SDL_EVENT_WINDOW_RESTORED:
				OnResize();
				should_tick = true;
				break;
			default:
				break;
			}
		}

		if (should_tick) {
			s_app->Tick(s_app);
		}
		else {

		}

	}

	export bool IsRunning() {
		return s_is_running;
	}

	export void Shutdown() {
		if (s_app->Shutdown) {
			s_app->Shutdown(s_app);
		}
		DestroyMainSurface();
		log::Info("Engine shutdown successfully");
		log::Shutdown();
	}
}