/* sdl3_application.cpp */
module;
#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <string>
#include <memory>
#endif

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
#include <boost/thread/tss.hpp>

export module fyuu_engine:sdl3_application;
#if defined(__cpp_lib_modules)
import std;
#endif
import :application;
import :logger_types;
import :logger;
import :application_option_parser;
import :rhi_context;
import :ui;

namespace fyuu_engine::application::sdl3 {
	
	export using SDLSurface = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

	export class SDL3Application 
		: public PolymorphicApplicationBase {
	private:
		logger::Logger m_logger;
		SDLSurface m_surface;
		rendering::RHIContext m_rhi;
		ui::UI m_ui;

		void Tick();

		void RHILog(fyuu_rhi::LogSeverity severity, std::string_view msg) noexcept;

	public:
		SDL3Application(ApplicationOptionParser& parser);

		int Run();

		float GetDisplayContentScale() const noexcept;

		rendering::RHIContext* GetRHIContext() noexcept;

		SDL_Window* GetSurfaceHandle() noexcept;

	};

} // namespace fyuu_engine::application

