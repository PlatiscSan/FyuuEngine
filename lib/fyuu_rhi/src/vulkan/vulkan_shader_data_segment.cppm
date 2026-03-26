/* vulkan_shader_data_segment.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:vulkan_shader_data_segment;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_sampler;

namespace fyuu_rhi::vulkan {
        
    export class VulkanShaderDataSegment
        : public PolymorphicShaderDataSegmentBase,
		public VulkanCommon<VulkanShaderDataSegment> {
	private:
		struct Declaration {
			struct PushConstantInfo {
				std::size_t count;
			};

			struct DescriptorInfo {
				std::size_t count;
				std::size_t ns;
				vk::DescriptorType type;
			};

			struct StaticSamplerInfo {
				SamplerFlags flags;
				SamplerAttachmentInfo attachment;	
				std::size_t ns;
			};

			std::size_t where;
			std::variant<PushConstantInfo, DescriptorInfo, StaticSamplerInfo> info;
			vk::ShaderStageFlags stages;
			
		};

		struct InstantiatedData {
			struct SetLayout {
				std::vector<vk::DescriptorSetLayoutBinding> bindings;
				std::unordered_map<std::size_t, VulkanSampler> immutable_samplers;
				vk::UniqueDescriptorSetLayout impl;
			};
			std::unordered_map<std::size_t, SetLayout> set_layouts;
            vk::UniquePipelineLayout pipeline_layout;
		};

		std::optional<std::size_t> m_logical_device_id;
		std::variant<std::vector<Declaration>, InstantiatedData> m_handle;

	public:
		struct ImmutableSamplerInfo {
			std::uint32_t set;
			std::uint32_t binding;
			VulkanSampler const* sampler;
		};

		VulkanShaderDataSegment(VulkanLogicalDevice const& logical_device);
		VulkanShaderDataSegment& Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		VulkanShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		VulkanShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		VulkanShaderDataSegment& Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		VulkanShaderDataSegment& Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);

		void Instantiate();
		void Reset();

		vk::PipelineLayout GetPipelineLayout() const noexcept;

		std::pair<vk::DescriptorSetLayoutBinding const*, VulkanSampler const*> GetLayoutBinding(std::size_t ns, std::size_t where) const noexcept;

		std::vector<ImmutableSamplerInfo> GetImmutableSamplers() const;

	};

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)