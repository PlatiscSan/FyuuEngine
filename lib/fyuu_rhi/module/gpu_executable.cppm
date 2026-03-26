/* gpu_executable.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <span>
#endif
export module fyuu_rhi:gpu_executable;
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :enums;
import :logical_device;
import :shader;
import :shader_data_segment;

namespace fyuu_rhi {

	export class GPUExecutable {
	private:
		UniqueGPUExecutable m_impl;

	public:
		GPUExecutable(LogicalDevice const& logical_device);

		GPUExecutable& SetShader(ShaderStage stage, Shader const& shader);
		GPUExecutable& SetShaderDataSegment(ShaderDataSegment const& segment);
		GPUExecutable& SetFlags(GPUExecutableFlags const& flags);
		GPUExecutable& SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info);
		GPUExecutable& SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks);
		GPUExecutable& SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks);
		GPUExecutable& SetDepthBias(DepthBias const& depth_bias);
		GPUExecutable& SetDepthStencil(DepthStencilInfo const& depth_stencil);
		GPUExecutable& SetMultiSample(MultiSample const& multi_sample);
		GPUExecutable& SetRenderTargetFormats(std::span<ResourceFlags const> format);
		GPUExecutable& SetVertexInputLayout(VertexInputLayout const& vertex_input_layout);
		GPUExecutable& SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes);
		GPUExecutable& AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute);
		GPUExecutable& AddVertexBinding(VertexBinding const& binding);
		GPUExecutable& AddVertexAttribute(VertexAttribute const& attribute);
		void Compile();
		void Reset();

		PolymorphicGPUExecutableBase* GetHandle() const noexcept;

	};


} // namespace fyuu_rhi
