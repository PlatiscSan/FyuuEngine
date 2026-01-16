module;
#include "declare_pool.h"

module fyuu_rhi:vulkan_pipeline_layout;
import :vulkan_logical_device;

namespace fyuu_rhi::vulkan {

	static vk::DescriptorType ToVkDescriptorType(BindingType binding_type) noexcept {
		switch (binding_type) {
		case BindingType::Sampler:
			return vk::DescriptorType::eSampler;
		case BindingType::SampledImage:
			return vk::DescriptorType::eSampledImage;
		case BindingType::StorageImage:
			return vk::DescriptorType::eStorageImage;
		case BindingType::UniformTexelBuffer:
			return vk::DescriptorType::eUniformTexelBuffer;
		case BindingType::StorageTexelBuffer:
			return vk::DescriptorType::eStorageTexelBuffer;
		case BindingType::UniformBuffer:
			return vk::DescriptorType::eUniformBuffer;
		case BindingType::StorageBuffer:
			return vk::DescriptorType::eStorageBuffer;
		case BindingType::UniformBufferDynamic:
			return vk::DescriptorType::eUniformBufferDynamic;
		case BindingType::StorageBufferDynamic:
			return vk::DescriptorType::eStorageBufferDynamic;
		case BindingType::InputAttachment:
			return vk::DescriptorType::eInputAttachment;
		case BindingType::Unknown:
		default:
			return vk::DescriptorType::eSampler;
		}
	}

	static vk::ShaderStageFlags ToVkShaderStageFlags(BindingVisibility visibility) noexcept {
		vk::ShaderStageFlags flags{};

		if (static_cast<uint32_t>(visibility) & static_cast<uint32_t>(BindingVisibility::Vertex)) {
			flags |= vk::ShaderStageFlagBits::eVertex;
		}
		if (static_cast<uint32_t>(visibility) & static_cast<uint32_t>(BindingVisibility::Fragment)) {
			flags |= vk::ShaderStageFlagBits::eFragment;
		}
		if (static_cast<uint32_t>(visibility) & static_cast<uint32_t>(BindingVisibility::Compute)) {
			flags |= vk::ShaderStageFlagBits::eCompute;
		}

		return flags;
	}

	static vk::UniqueDescriptorSetLayout CreateDescriptorLayout(
		VulkanLogicalDevice const& device,
		PipelineLayoutDescriptor const& descriptor
	) {

		DECLARE_POOL(layout_bindings_pool, vk::DescriptorSetLayoutBinding, 256ull)

		std::pmr::vector<vk::DescriptorSetLayoutBinding> layout_bindings(layout_bindings_pool_alloc);

		for (auto const& item : descriptor.bindings) {
			auto type = ToVkDescriptorType(item.type);
			auto stage_flags = ToVkShaderStageFlags(item.stage_flags);
			if (item.is_bindless) {
				continue;
			}
			layout_bindings.emplace_back(
				item.binding,   // see vertex shader
				type,
				1u,
				stage_flags,
				nullptr
			);
		}

		vk::DescriptorSetLayoutCreateInfo layout_info(
			vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR, // Setting this flag tells the descriptor set layouts that no actual descriptor sets are allocated but instead pushed at command buffer creation time
			static_cast<std::uint32_t>(layout_bindings.size()),
			layout_bindings.data(),
			nullptr
		);

		// create the descriptor set layout
		return device.GetNative().createDescriptorSetLayoutUnique(layout_info, nullptr, *device.GetRawDispatcher());

	}

	static vk::UniquePipelineLayout CreatePipelineLayout(
		VulkanLogicalDevice const& device,
		PipelineLayoutDescriptor const& descriptor,
		vk::DescriptorSetLayout const& vk_desc_set_layout
	) {
		
		DECLARE_POOL(set_layout_pool, vk::DescriptorSetLayout, 256ull)

		std::pmr::vector<vk::DescriptorSetLayout> set_layouts(set_layout_pool_alloc);
		set_layouts.emplace_back(vk_desc_set_layout);

		for (auto const& item : descriptor.bindings) {
			auto type = ToVkDescriptorType(item.type);
			auto stage_flags = ToVkShaderStageFlags(item.stage_flags);
			if (item.is_bindless) {
				switch (item.type) {
				case BindingType::SampledImage:
					//set_layouts[item.binding] = (owningDevice->globalTextureDescriptorSetLayout);
					break;
				case BindingType::StorageBuffer:
					//set_layouts[item.binding] = owningDevice->globalBufferDescriptorSetLayout;
					break;
				}
			}
		}

		//// setup push constants
		//const auto nconstants = descriptor.constants.size();
		//std::vector<VkPushConstantRange> pushconstants;
		//pushconstants.resize(nconstants);

		//for (int i = 0; i < nconstants; i++) {
		//	const auto flags = rgl2vkstageflags(descriptor.constants[i].visibility);
		//	pushConstantBindingStageFlags[descriptor.constants[i].n_register] = flags;
		//	pushconstants[i].offset = descriptor.constants[i].n_register;
		//	pushconstants[i].size = descriptor.constants[i].size_bytes;
		//	pushconstants[i].stage_flags = flags;
		//}


		//VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		//	.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		//	.flags = 0,
		//	.setLayoutCount = uint32_t(set_layouts.size()),
		//	.pSetLayouts = set_layouts.data(),
		//	.pushConstantRangeCount = static_cast<uint32_t>(nconstants),
		//	.pPushConstantRanges = pushconstants.data()
		//};
		//VK_CHECK(vkCreatePipelineLayout(owningDevice->device, &pipelineLayoutInfo, nullptr, &layout));

	}

	VulkanPipelineLayout::VulkanPipelineLayout(
		VulkanLogicalDevice const& device, 
		PipelineLayoutDescriptor const& descriptor
	) : m_descriptor_set_layout(CreateDescriptorLayout(device, descriptor)) {
	}

	VulkanPipelineLayout::VulkanPipelineLayout(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& device,
		PipelineLayoutDescriptor const& descriptor
	) : m_descriptor_set_layout(CreateDescriptorLayout(*device, descriptor)) {

	}

}