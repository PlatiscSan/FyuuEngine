module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <variant>
#include <span>
#endif
#include "webgpu.hpp"

export module fyuu_rhi:webgpu_sampler;

#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	export class WebGPUSampler
		: public PolymorphicSamplerBase,
		public WebGPUCommon<WebGPUSampler> {
	private:

	public:
		template <class... Args>
		WebGPUSampler(Args&&... args)
			: PolymorphicSamplerBase(this),
			WebGPUCommon(this)  {
			// placeholder constructor
		}
	};
	
} // namespace fyuu_rhi::webgpu