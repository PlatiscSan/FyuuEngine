module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <variant>
#include <print>
#endif // !defined(__cpp_lib_modules)
module fyuu_engine:application_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :application;
import :application_option_parser;

import :sdl3_application;

namespace fyuu_engine::application {


	Application::Application(int argc, char** argv)
		: m_impl(
			[argc, argv]() {
				
				ApplicationOptionParser parser(argc, argv);

				if (parser.ShowHelp()) {
					throw ShowHelp();
				}
		
				ApplicationBackend backend = parser.ParseBackend();
		
				switch (backend) {
				case ApplicationBackend::PlatformDefault:
				case ApplicationBackend::SDL3:
					return plastic::utility::MakeUnique<sdl3::SDL3Application>(parser);
				case ApplicationBackend::GLFW:
				default:
					throw std::runtime_error("Not implemented backend or unsupported");
				}

			}()) {

	}

	int Application::Run() noexcept {

		auto overload = plastic::utility::Overload{
			[](sdl3::SDL3Application* app) -> int {
				return app->Run();
			},
			[](glfw::GLFWApplication* app) -> int {
				return 0;
			}
		};

		return m_impl->Visit(overload);

	}

	float Application::GetDisplayContentScale() const noexcept {
		auto overload = plastic::utility::Overload{
			[](sdl3::SDL3Application const* app) -> float {
				return app->GetDisplayContentScale();
			},
			[](glfw::GLFWApplication const* app) -> float {
				return 0.0f;
			}
		};

		return m_impl->Visit(overload);
	}

	rendering::RHIContext* Application::GetRHIContext() noexcept {
		auto overload = plastic::utility::Overload{
			[](sdl3::SDL3Application* app) -> rendering::RHIContext* {
				return app->GetRHIContext();
			},
			[](glfw::GLFWApplication* app) -> rendering::RHIContext* {
				return nullptr;
			}
		};

		return m_impl->Visit(overload);
	}

	ApplicationBackend Application::GetBackEnd() const noexcept {
		auto overload = plastic::utility::Overload{
			[](sdl3::SDL3Application const*) -> ApplicationBackend {
				return ApplicationBackend::SDL3;
			},
			[](glfw::GLFWApplication const*) -> ApplicationBackend {
				return ApplicationBackend::GLFW;
			}
		};

		return m_impl->Visit(overload);
	}

	void* Application::GetSurfaceHandle() noexcept {
		auto overload = plastic::utility::Overload{
			[](sdl3::SDL3Application* derived) -> void* {
				return derived->GetSurfaceHandle();
			},
			[](glfw::GLFWApplication* derived) -> void* {
				return nullptr;
			}
		};

		return m_impl->Visit(overload);
	}

}