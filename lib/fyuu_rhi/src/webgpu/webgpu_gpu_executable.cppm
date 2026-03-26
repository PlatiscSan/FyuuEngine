module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_gpu_executable;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

    export class WebGPUGPUExecutable
		: public PolymorphicGPUExecutableBase,
		public WebGPUCommon<WebGPUGPUExecutable> {
    private:

    public:
		template <class... Args>
		WebGPUGPUExecutable(Args&&... args)
			: PolymorphicGPUExecutableBase(this),
			WebGPUCommon(this)  {
			// placeholder constructor
		}

		WebGPUGPUExecutable& SetShader(ShaderStage stage, WebGPUShader const& shader);
		WebGPUGPUExecutable& SetShaderDataSegment(WebGPUShaderDataSegment const& shader_data_segment);
		WebGPUGPUExecutable& SetFlags(GPUExecutableFlags const& flags);
		WebGPUGPUExecutable& SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info);
		WebGPUGPUExecutable& SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks);
		WebGPUGPUExecutable& SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks);
		WebGPUGPUExecutable& SetDepthBias(DepthBias const& depth_bias);
		WebGPUGPUExecutable& SetDepthStencil(DepthStencilInfo const& depth_stencil);
		WebGPUGPUExecutable& SetMultiSample(MultiSample const& multi_sample);
		WebGPUGPUExecutable& SetRenderTargetFormats(std::span<ResourceFlags const> format);
		WebGPUGPUExecutable& SetVertexInputLayout(VertexInputLayout const& vertex_input_layout);
		WebGPUGPUExecutable& SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes);
		WebGPUGPUExecutable& AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute);
		WebGPUGPUExecutable& AddVertexBinding(VertexBinding const& binding);
		WebGPUGPUExecutable& AddVertexAttribute(VertexAttribute const& attribute);

		void Compile();
		void Reset();
	


    };

} // namespace fyuu_rhi::webgpu