module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <memory>
#include <atomic>
#include <functional>
#include <variant>
#include <optional>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "fyuu_platform.hpp"
export module fyuu_engine:imgui;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :application;
import :ui_types;

namespace fyuu_engine::ui::imgui {

	export class ImGUIResource {

	};

	export class ImGUI 
		: public PolymorphicUIBase {
	private:
		std::atomic<std::shared_ptr<ImGUIResource>> m_immutable_resource;

	public:
		ImGUI(UIInit const& init);
		~ImGUI() noexcept = default;


	};


} // namespace fyuu_engine::ui::imgui
