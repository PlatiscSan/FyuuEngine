module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_shader;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

    export class WebGPUShader
		: public PolymorphicShaderBase,
		public WebGPUCommon<WebGPUShader> {
    private:

    public:
		template <class... Args>
		WebGPUShader(Args&&... args)
			: PolymorphicShaderBase(this),
			WebGPUCommon(this)  {
			// placeholder constructor
		}

    };

} // namespace fyuu_rhi::webgpu