module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#include <memory>
#endif // !defined(__cpp_lib_modules)
export module fyuu_engine:ui;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :ui_types;
import :application_types;
import :rhi_context;

namespace fyuu_engine::ui {

	export class UI {
	private:
		UniqueUI m_impl;

	public:
		UI(UIInit const& init);


	};

}