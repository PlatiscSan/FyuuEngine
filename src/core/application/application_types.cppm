module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#include <exception>
#endif
export module fyuu_engine:application_types;
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.sealed_polymorphism;

namespace fyuu_engine::application {

	export enum class ApplicationBackend : std::uint32_t {
		PlatformDefault,
		SDL3,
		GLFW,
		Count
	};

	namespace sdl3 {
		export class SDL3Application;
	} // namespace sdl3

	namespace glfw {
		export class GLFWApplication;
	} // namespace glfw

	using PolymorphicApplicationBase = plastic::utility::PolymorphicBase<
		std::size_t,
		sdl3::SDL3Application,
		glfw::GLFWApplication
	>;

	export class ShowHelp
		: public std::exception {
	public:
		char const* what() const noexcept override {
			return "show help";
		}
	};

	using UniqueApplication = plastic::utility::UniqueBase<PolymorphicApplicationBase>;

}
