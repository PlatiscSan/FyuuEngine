module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <variant>
#endif
#include "glad.hpp"
export module fyuu_rhi:webgpu_view;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.resource;
import :types;
import :webgpu_common;
import :webgpu_resource;

namespace fyuu_rhi::webgpu {

	export class WebGPUView
		: public PolymorphicViewBase,
		public WebGPUCommon<WebGPUView> {
	private:

		
	public:
		template <class... Args>
		WebGPUView(Args&&... args)
			: PolymorphicViewBase(this),
			WebGPUCommon(this)  {
			// placeholder constructor
		}

	};

}

#endif // !defined(__APPLE__)