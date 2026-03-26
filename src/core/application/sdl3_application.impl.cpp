
module;
#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <mutex>
#include <format>
#endif // defined(__cpp_lib_modules)

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#endif // defined(_WIN32)
#include <SDL3/SDL.h>
#include "fyuu_declare_pool.hpp"

#if !defined(ENGINE_VER_VARIANT)
#define ENGINE_VER_VARIANT 0
#endif // !defined(ENGINE_VER_VARIANT)

#if !defined(ENGINE_VER_MAJOR)
#define ENGINE_VER_MAJOR 1
#endif // !defined(ENGINE_VER_MAJOR)

#if !defined(ENGINE_VER_MINOR)
#define ENGINE_VER_MINOR 0
#endif // !defined(ENGINE_VER_MINOR)

#if !defined(ENGINE_VER_PATCH)
#define ENGINE_VER_PATCH 0
#endif // !defined(ENGINE_VER_PATCH)

module fyuu_engine:sdl3_application_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :sdl3_application;
import plastic.other;
import :application;
import :application_option_parser;
import :logger_types;
import :logger;
import :ui;

static constexpr char const* s_default_app_name = "TouhouIBunKitan";
static constexpr char const* s_default_identifier = "com.Touhou.IBunKitan";
static constexpr char const* s_ui_style = "Dark";

static constexpr std::uint8_t engine_ver_variant = ENGINE_VER_VARIANT;
static constexpr std::uint8_t engine_ver_major = ENGINE_VER_MAJOR;
static constexpr std::uint8_t engine_ver_minor = ENGINE_VER_MINOR;
static constexpr std::uint8_t engine_ver_patch = ENGINE_VER_PATCH;


namespace {

	using namespace fyuu_engine;
	using namespace fyuu_engine::application;

	struct NativeHandle {

		// This variable is used for Wayland's wl_display* and X11's Display*.
		void* native_display = nullptr;

		// This variable is used for Windows HWND, Wayland wl_surface*, Android ANativeWindow*, and Cocoa UIWindow*.
		void* native_window = nullptr;

		// This variable is used for X11 Windows (32-bit or 64-bit integer).
		std::uint64_t native_window_id = 0;

	};

	NativeHandle GetNativeHandle(SDL_Window* window, logger::Logger& logger) {
		SDL_PropertiesID props = SDL_GetWindowProperties(window);
		if (!props) {
			char const* error = SDL_GetError();
			logger.Fatal(error);
			throw std::runtime_error(error);
		}

		// This variable is used for Wayland's wl_display* and X11's Display*.
		void* native_display = nullptr;

		// This variable is used for Windows HWND, Wayland wl_surface*, Android ANativeWindow*, and Cocoa UIWindow*.
		void* native_window = nullptr;

		// This variable is used for X11 Windows (32-bit or 64-bit integer).
		std::uint64_t native_window_id = 0;

#if defined(_WIN32)
		native_window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
		if (!native_window) {
			char const* error = SDL_GetError();
			logger.Fatal(error);
			throw std::runtime_error(error);
		}

#elif defined(__linux__)
		native_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
		if (native_display) {
			native_window_id = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
			if (native_window_id == 0) {
				char const* error = SDL_GetError();
				logger.Fatal(error);
				throw std::runtime_error(error);
			}
		}
		else {
			native_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
			native_window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
			if (!native_display || !native_window) {
				char const* error = SDL_GetError();
				logger.Fatal(error);
				throw std::runtime_error("Failed to get Wayland display/surface");
			}
		}

#elif defined(__ANDROID__)
		native_window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
		if (!native_window) {
			char const* error = SDL_GetError();
			logger.Fatal(error);
			throw std::runtime_error(error);
		}

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
		native_window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif TARGET_OS_IOS
		native_window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
#else
#error "Unsupported Apple platform"
#endif
		if (!native_window) {
			char const* error = SDL_GetError();
			logger.Fatal(error);
			throw std::runtime_error(error);
		}

#else
#error "Unsupported platform"
#endif

		return { native_display, native_window, native_window_id };

	}

	fyuu_rhi::Version ParseApplicationVersion(ApplicationOptionParser& parser) {

		configuration::Configuration& conf = parser.ParseConfiguration();

		std::string version_str = conf["version"].AsOr<std::string>("1.0.0");
		fyuu_rhi::Version app_ver{};
		// Parse "major.minor.patch" – any extra parts are ignored, variant defaults to 0.
		size_t start = 0;
		size_t end = version_str.find('.');
		int part_index = 0;
		while (end != std::string::npos && part_index < 3) {
			std::string part = version_str.substr(start, end - start);
			if (!part.empty()) {
				auto val = static_cast<std::uint8_t>(std::stoi(part));
				if (part_index == 0) app_ver.major = val;
				else if (part_index == 1) app_ver.minor = val;
				else if (part_index == 2) app_ver.patch = val;
			}
			++part_index;
			start = end + 1;
			end = version_str.find('.', start);
		}
		// Handle the final part after the last dot (if any and under 3 parts)
		if (part_index < 3 && start < version_str.length()) {
			std::string part = version_str.substr(start);
			if (!part.empty()) {
				auto val = static_cast<std::uint8_t>(std::stoi(part));
				if (part_index == 0) app_ver.major = val;
				else if (part_index == 1) app_ver.minor = val;
				else if (part_index == 2) app_ver.patch = val;
			}
		}
		app_ver.variant = 0;  // Default variant (stable)
		return app_ver;

	}

	void InitializeSDL(ApplicationOptionParser& parser, logger::Logger& logger) {
		static std::once_flag flag;
		std::call_once(
			flag,
			[&]() {
				logger.Info("Initializing SDL3");

				configuration::Configuration& conf = parser.ParseConfiguration();

				using Pair = std::pair<std::string_view const, std::string>;
				DECLARE_POOL(pair, Pair, 8)
				std::pmr::unordered_map<std::string_view, std::string> sdl_metadata(pair_alloc);

				sdl_metadata[SDL_PROP_APP_METADATA_NAME_STRING] = conf["name"].AsOr<std::string>(s_default_app_name);
				sdl_metadata[SDL_PROP_APP_METADATA_VERSION_STRING] = conf["version"].AsOr<std::string>("1.0.0");
				sdl_metadata[SDL_PROP_APP_METADATA_IDENTIFIER_STRING] = conf["identifier"].AsOr<std::string>(s_default_identifier);

				auto creator = conf["creator"];
				auto copyright = conf["copyright"];
				auto url = conf["url"];
				auto type = conf["type"];

				if (creator.HasValue()) {
					sdl_metadata[SDL_PROP_APP_METADATA_CREATOR_STRING] = creator.As<std::string>();
				}

				if (copyright.HasValue()) {
					sdl_metadata[SDL_PROP_APP_METADATA_COPYRIGHT_STRING] = copyright.As<std::string>();
				}

				if (url.HasValue()) {
					sdl_metadata[SDL_PROP_APP_METADATA_URL_STRING] = url.As<std::string>();
				}

				if (type.HasValue()) {
					sdl_metadata[SDL_PROP_APP_METADATA_TYPE_STRING] = type.As<std::string>();
				}

				for (auto const& [key, value] : sdl_metadata) {
					if (!SDL_SetAppMetadataProperty(key.data(), value.data())) {
						char const* error = SDL_GetError();
						logger.Fatal(error);
						throw std::runtime_error(error);
					}
				}

				static constexpr std::uint64_t sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD;

				if (!SDL_Init(sdl_init_flags)) {
					char const* error = SDL_GetError();
					logger.Fatal(error);
					throw std::runtime_error(error);
				}
			
			}
		);
		static plastic::utility::Defer SDL_gc(
			[]() {
				SDL_Quit();
			}
		);

	}

	ui::Style ParseUIStyle(ApplicationOptionParser& parser) {

		configuration::Configuration& conf = parser.ParseConfiguration();
		std::string style_str = conf["ui"]["style"].AsOr<std::string>(s_default_identifier);
		std::transform(
			style_str.begin(), style_str.end(), style_str.begin(),
			[](unsigned char c) { 
				return std::tolower(c); 
			}
		);

		if (style_str == "dark") {
			return ui::Style::Dark;
		}

		if (style_str == "light") {
			return ui::Style::Light;
		}

		return ui::Style::Dark; 

	}

}

namespace fyuu_engine::application::sdl3 {

	void SDL3Application::Tick() {

	}

	void SDL3Application::RHILog(fyuu_rhi::LogSeverity severity, std::string_view msg) noexcept {
		switch (severity) {
		case fyuu_rhi::LogSeverity::Fatal:
			m_logger.Fatal(msg);
			break;
		case fyuu_rhi::LogSeverity::Error:
			m_logger.Error(msg);
			break;
		case fyuu_rhi::LogSeverity::Warning:
			m_logger.Warning(msg);
			break;
		case fyuu_rhi::LogSeverity::Info:
			m_logger.Info(msg);
			break;
		default:
			m_logger.Info(msg);
			break;
		}

	}

	SDL3Application::SDL3Application(ApplicationOptionParser& parser)
		: PolymorphicApplicationBase(this),
		m_logger(
			logger::LoggerBackend::PlatformDefault,
			"main logger",
			"./logs/app.log",
			5 * 1024 * 1024, // 5MB
			5
		),
		m_surface(
			[this, &parser]() {

				InitializeSDL(parser, m_logger);

				configuration::Configuration& conf = parser.ParseConfiguration();
				std::string app_name = conf["name"].AsOr<std::string>(s_default_app_name);

				static constexpr SDL_WindowFlags window_flags =
					SDL_WINDOW_RESIZABLE
					| SDL_WINDOW_HIGH_PIXEL_DENSITY
#if defined(__APPLE__)
					| SDL_WINDOW_METAL
#endif // defined(__APPLE__)
					;

				SDL_Window* window = SDL_CreateWindow(
					app_name.data(),
					1280u,	// width, in pixels
					800u,		// height, in pixels
					window_flags
				);

				if (!window) {
					char const* error = SDL_GetError();
					m_logger.Fatal(error);
					throw std::runtime_error(error);
				}

				return SDLSurface(window, SDL_DestroyWindow);

			}()),
		m_rhi(
			[this, &parser]() {
				configuration::Configuration& conf = parser.ParseConfiguration();
				std::string app_name = conf["name"].AsOr<std::string>(s_default_app_name);

				auto [native_display, native_window, native_window_id] = GetNativeHandle(m_surface.get(), m_logger);

				fyuu_engine::rendering::RHIContext context(
					parser.ParseAPI(), 
					{
						app_name,
						"Fyuu Engine",
						ParseApplicationVersion(parser),
						{
							engine_ver_variant,
							engine_ver_major,
							engine_ver_minor,
							engine_ver_patch
						},
						{
							[](fyuu_rhi::LogSeverity severity, std::string_view msg, void* user_data) {
								static_cast<SDL3Application*>(user_data)->RHILog(severity, msg);
							},
							this
						},
						{
#if defined(_WIN32)
							static_cast<HWND>(native_window),
#elif defined(__linux__)
							[&]() -> std::variant<std::monostate, fyuu_rhi::Wayland, fyuu_rhi::Xlib> {
								return native_window_id ?
									fyuu_rhi::Xlib{static_cast<Display*>(native_display), native_window_id} :
									fyuu_rhi::Wayland{ static_cast<wl_display*>(native_display), static_cast<wl_surface*>(native_window) };
							}()
#elif defined(__ANDROID__)
							static_cast<ANativeWindow*>(native_window),
#endif // defined(_WIN32)
						},
						parser.IsSoftwareFallbackAllowed()
					}
				);

				m_logger.Info(std::format("Current graphics API: {}", context.GetAPIString()));
				m_logger.Info(std::format("Device vendor {}, ID {}, {}", context.GetVendorID(), context.GetID(), context.GetDescription()));

				return context;

			}()),
		m_ui(ui::UIBackend::ImGUI, ApplicationBackend::SDL3, ParseUIStyle(parser), &m_rhi) {
		
		

	}

	int SDL3Application::Run() {

		m_logger.Info("Application is now running");

		try {
			
			SDL_Event e;
			bool quit = 0;
			while (!quit) {

				while (SDL_PollEvent(&e)) {
					/*
					*	process event
					*/
					switch (e.type) {
					case SDL_EventType::SDL_EVENT_QUIT:
						quit = true;
						break;
					case SDL_EventType::SDL_EVENT_WINDOW_RESIZED:
						m_rhi.Resize();
						break;
					default:
						break;
					}
				}

				Tick();

			}

			return 0;
		}
		catch (std::exception const& ex) {
			m_logger.Fatal(ex.what());
			return -1;
		}

	}

	float SDL3Application::GetDisplayContentScale() const noexcept {
		return SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	}

	rendering::RHIContext* SDL3Application::GetRHIContext() noexcept {
		return &m_rhi;
	}

	SDL_Window* SDL3Application::GetSurfaceHandle() noexcept {
		return m_surface.get();
	}
	
}