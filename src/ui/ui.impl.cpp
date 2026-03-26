module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <stdexcept>
#include <functional>
#endif // !defined(__cpp_lib_modules)
module fyuu_engine:ui_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :ui;
import :application;
import plastic.sealed_polymorphism;
import :imgui;

namespace fyuu_engine::ui {

	UI::UI(UIInit const& init)
		: m_impl(
			[=]() {
				switch (init.ui_backend) {
				case UIBackend::PlatformDefault:
				case UIBackend::ImGUI:
					return plastic::utility::MakeUnique<imgui::ImGUI>(init);
				
				default:
					throw std::runtime_error("Not implemented UI backend or unsupported");
				}
			}()) {
	}

}