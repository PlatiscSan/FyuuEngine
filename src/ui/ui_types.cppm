module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#endif // !defined(__cpp_lib_modules)
export module fyuu_engine:ui_types;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :rhi_context;
import :application_types;

namespace fyuu_engine::ui {

	namespace imgui {
		export class ImGUI;
	} // namespace imgui
	
	export enum class UIBackend : std::uint8_t {
		PlatformDefault = 0,
		ImGUI
	};

	export enum class Style : std::uint8_t {
		Dark,
		Light
	};

	export using PolymorphicUIBase = plastic::utility::PolymorphicBase<
		std::size_t,
		imgui::ImGUI
	>;

	export using UniqueUI = plastic::utility::UniqueBase<PolymorphicUIBase>;

	export struct UIInit {
		UIBackend ui_backend; 
		application::ApplicationBackend app_backend;
		Style style; 
		float scale; 
		rendering::RHIContext* rhi;
		void* surface;
	};

}