export module fyuu_rhi:vulkan_pipeline_layout;
import std;
import vulkan_hpp;
import plastic.any_pointer;
import plastic.registrable;
import :pipeline;
import :vulkan_declaration;

namespace fyuu_rhi::vulkan {

	export class VulkanPipelineLayout
		: public IPipelineLayout {
	private:
		vk::UniqueDescriptorSetLayout m_descriptor_set_layout;
		vk::UniquePipelineLayout m_impl;

	public:
		VulkanPipelineLayout(
			VulkanLogicalDevice const& device,
			PipelineLayoutDescriptor const& descriptor
		);

		VulkanPipelineLayout(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& device,
			PipelineLayoutDescriptor const& descriptor
		);
	};

}