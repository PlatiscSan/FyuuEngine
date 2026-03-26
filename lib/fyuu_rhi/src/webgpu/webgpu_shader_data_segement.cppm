module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_shader_data_segment;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

    export class WebGPUShaderDataSegment
		: public PolymorphicShaderDataSegmentBase,
		public WebGPUCommon<WebGPUShaderDataSegment> {
    private:

    public:
		template <class... Args>
		WebGPUShaderDataSegment(Args&&... args)
			: PolymorphicShaderDataSegmentBase(this),
			WebGPUCommon(this)  {
			// placeholder constructor
		}

		WebGPUShaderDataSegment& Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		WebGPUShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		WebGPUShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		WebGPUShaderDataSegment& Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		WebGPUShaderDataSegment& Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		
		void Instantiate();
		void Reset();
    };

} // namespace fyuu_rhi::webgpu