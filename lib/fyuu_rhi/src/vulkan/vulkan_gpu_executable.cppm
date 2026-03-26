/* vulkan_gpu_executable.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:vulkan_gpu_executable;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;

namespace fyuu_rhi::vulkan {
        
    export class VulkanGPUExecutable
        : public PolymorphicGPUExecutableBase,
		public VulkanCommon<VulkanGPUExecutable> {
	private:
		struct Declaration {
			std::unordered_map<ShaderStage, std::optional<std::size_t>> shaders;
			std::optional<std::size_t> shader_data_segment_id;
			GPUExecutableFlags flags;
			RenderTargetAttachmentsInfo rt_att;
			std::optional<DepthBias> depth_bias;
			std::optional<DepthStencilInfo> depth_stencil;
			std::optional<MultiSample> multi_sample;
			VertexInputLayout vertex_input_layout;
			std::vector<ResourceFlags> rt_formats;
		};

		struct Compilation {
			std::optional<std::size_t> shader_data_segment_id;
			ShaderStage pso_type;
			vk::UniquePipeline pso;
		};

		std::optional<std::size_t> m_logical_device_id;
		std::variant<Declaration, Compilation> m_handle;

		void CompileGraphicsExecutable(Declaration* declaration);
		void CompileComputingExecutable(Declaration* declaration);
		void CompileMeshExecutable(Declaration* declaration);
		void CompileRayTracingExecutable(Declaration* declaration);

	public:
		VulkanGPUExecutable(VulkanLogicalDevice const& logical_device);

		VulkanGPUExecutable& SetShader(ShaderStage stage, VulkanShader const& shader);
		VulkanGPUExecutable& SetShaderDataSegment(VulkanShaderDataSegment const& shader_data_segment);
		VulkanGPUExecutable& SetFlags(GPUExecutableFlags const& flags);
		VulkanGPUExecutable& SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info);
		VulkanGPUExecutable& SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks);
		VulkanGPUExecutable& SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks);
		VulkanGPUExecutable& SetDepthBias(DepthBias const& depth_bias);
		VulkanGPUExecutable& SetDepthStencil(DepthStencilInfo const& depth_stencil);
		VulkanGPUExecutable& SetMultiSample(MultiSample const& multi_sample);
		VulkanGPUExecutable& SetRenderTargetFormats(std::span<ResourceFlags const> format);
		VulkanGPUExecutable& SetVertexInputLayout(VertexInputLayout const& vertex_input_layout);
		VulkanGPUExecutable& SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes);
		VulkanGPUExecutable& AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute);
		VulkanGPUExecutable& AddVertexBinding(VertexBinding const& binding);
		VulkanGPUExecutable& AddVertexAttribute(VertexAttribute const& attribute);

		std::optional<std::size_t> GetAssociatedShaderDataSegmentID() const noexcept;

		std::pair<ShaderStage, vk::Pipeline> GetPSO() const noexcept;

		void Compile();

		void Reset();

	};

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)