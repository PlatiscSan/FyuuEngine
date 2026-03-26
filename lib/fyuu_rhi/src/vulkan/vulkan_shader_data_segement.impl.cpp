/* vulkan_shader_data_segment.impl.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "declare_pool.hpp"
module fyuu_rhi:vulkan_shader_data_segment_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_shader_data_segment;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_logical_device;
import :vulkan_sampler;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	std::optional<vk::DescriptorType> DescriptorTypeFromResourceFlags(ResourceFlags flags) noexcept {

		if (HasConflictingFlags(flags, ResourceFlags::Buffer, ResourceFlags::Texture)) {
			return std::nullopt;
		}

		if ((flags & ResourceFlags::UniformTexelBuffer) != ResourceFlags::Unknown)
			return vk::DescriptorType::eUniformTexelBuffer;

		if ((flags & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown)
			return vk::DescriptorType::eStorageTexelBuffer;

		if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown)
			return vk::DescriptorType::eUniformBuffer;

		if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown)
			return vk::DescriptorType::eStorageBuffer;

		if ((flags & ResourceFlags::SampledTexture) != ResourceFlags::Unknown)
			return vk::DescriptorType::eSampledImage;

		if ((flags & ResourceFlags::StorageTexture) != ResourceFlags::Unknown)
			return vk::DescriptorType::eStorageImage;

		if ((flags & ResourceFlags::Input) != ResourceFlags::Unknown)
			return vk::DescriptorType::eInputAttachment;		   
	   
		return std::nullopt;

	}

}

namespace fyuu_rhi::vulkan {
	VulkanShaderDataSegment::VulkanShaderDataSegment(VulkanLogicalDevice const& logical_device)
		: PolymorphicShaderDataSegmentBase(this),
		VulkanCommon(this),
		m_logical_device_id(logical_device.GetID()),
		m_handle() {

		
	}

#define VALIDATE_STATE_AND_STAGE \
		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle);	\
		if (!declarations) {	\
			throw std::logic_error("Cannot declare after instantiation");	\
		}	\
		if (visible == ShaderStage::Unknown) {	\
			throw std::invalid_argument("Visible stage cannot be Unknown");	\
		}

	VulkanShaderDataSegment& VulkanShaderDataSegment::Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		vk::ShaderStageFlags stages = ShaderStageToVk(visible);
		declarations->emplace_back(where, Declaration::PushConstantInfo{ count }, stages);

		return *this;

	}

	VulkanShaderDataSegment& VulkanShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		vk::ShaderStageFlags stages = ShaderStageToVk(visible);
		auto desc_type = DescriptorTypeFromResourceFlags(flags);

		if (!desc_type) {
			throw std::invalid_argument("Invalid resource flags");
		}

		declarations->emplace_back(where, Declaration::DescriptorInfo{ 1u, ns, *desc_type }, stages);

		return *this;

	}

	VulkanShaderDataSegment& VulkanShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		vk::ShaderStageFlags stages = ShaderStageToVk(visible);
		auto desc_type = DescriptorTypeFromResourceFlags(flags);

		if (!desc_type) {
			throw std::invalid_argument("Invalid resource flags");
		}

		declarations->emplace_back(where, Declaration::DescriptorInfo{ count, ns, *desc_type }, stages);

		return *this;

	}

	VulkanShaderDataSegment& VulkanShaderDataSegment::Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		vk::ShaderStageFlags stages = ShaderStageToVk(visible);
		declarations->emplace_back(where, Declaration::StaticSamplerInfo{ flags, info, ns }, stages);

		return *this;

	}

	VulkanShaderDataSegment& VulkanShaderDataSegment::Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {

		VALIDATE_STATE_AND_STAGE

		vk::ShaderStageFlags stages = ShaderStageToVk(visible);
		declarations->emplace_back(where, Declaration::DescriptorInfo{ count, ns, vk::DescriptorType::eSampler }, stages);

		return *this;

	}

	void VulkanShaderDataSegment::Instantiate() {
			
		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle);
		if (!declarations) {
			throw std::logic_error("Cannot instantiate again after instantiation");
		}

		if (!m_logical_device_id) {
			throw std::runtime_error("Invalid logical device id");
		}
		auto logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);
		if (!logical_device) {
			throw std::runtime_error("Logical device lost");
		}

		using SetMapPair = std::pair<std::size_t const, SmallVector<Declaration, 32u>>;
		DECLARE_POOL(set_map, SetMapPair, 64u)

		std::pmr::unordered_map<std::remove_const_t<SetMapPair::first_type>, SetMapPair::second_type> set_map(set_map_alloc);
		SetMapPair::second_type push_constants;

		for (auto const& declaration : *declarations) {
			std::visit(
				[&declaration, &push_constants, &set_map](auto&& info) {
					using Info = std::decay_t<decltype(info)>;
					if constexpr (std::same_as<Info, Declaration::PushConstantInfo>) {
						push_constants.emplace_back(declaration);
					}
					else {
						auto&& [iter, _] = set_map.try_emplace(info.ns);
						iter->second.emplace_back(declaration);
					}
					
				},
				declaration.info
			);
		}

		std::unordered_map<std::size_t, InstantiatedData::SetLayout> set_layouts;

		SmallVector<vk::DescriptorSetLayout, 512> raw_layouts;
		SmallVector<vk::Sampler, 512> raw_samplers;

		for (auto& [set, decls] : set_map) {

			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			std::unordered_map<std::size_t, VulkanSampler> immutable_samplers;

			for (auto& decl : decls) {
				std::visit(
					[logical_device, &decl, &bindings, &raw_samplers, &immutable_samplers](auto&& info) {
						using Info = std::decay_t<decltype(info)>;
						if constexpr (std::same_as<Info, Declaration::DescriptorInfo>) {
							bindings.emplace_back(
								static_cast<std::uint32_t>(decl.where),
								info.type,
								static_cast<std::uint32_t>(info.count),
								decl.stages,
								nullptr
							);
						}
						else if constexpr (std::same_as<Info, Declaration::StaticSamplerInfo>) {
							auto&& [iter, _] = immutable_samplers.try_emplace(
								static_cast<std::uint32_t>(decl.where), 
								*logical_device, 
								info.flags, 
								info.attachment
							);
							vk::Sampler& raw_sampler = raw_samplers.emplace_back(iter->second.GetNative());
							bindings.emplace_back(
								static_cast<std::uint32_t>(decl.where),
								vk::DescriptorType::eSampler,
								1u,
								decl.stages,
								&raw_sampler
							);
						}
						else {

						}
					},
					decl.info
				);
			}

			vk::DescriptorSetLayoutCreateInfo layout_info(
				{},
				bindings
			);

			vk::UniqueDescriptorSetLayout layout = logical_device->CreateDescriptorSetLayout(layout_info);
			raw_layouts.emplace_back(*layout);

			for (auto& binding : bindings) {
				binding.pImmutableSamplers = nullptr;
			}

			set_layouts.try_emplace(set, std::move(bindings), std::move(immutable_samplers), std::move(layout));

		}

		SmallVector<vk::PushConstantRange, 32u> push_constant_ranges;
		for (auto& decl : push_constants) {
			auto& push_info = std::get<Declaration::PushConstantInfo>(decl.info);
			push_constant_ranges.emplace_back(decl.stages, static_cast<uint32_t>(decl.where), static_cast<uint32_t>(push_info.count * 4));
		}

		vk::PipelineLayoutCreateInfo pl_info(
			{},
			raw_layouts,
			push_constant_ranges,
			nullptr
		);

		vk::UniquePipelineLayout pipeline_layout = logical_device->CreatePipelineLayout(pl_info);

		m_handle.emplace<InstantiatedData>(std::move(set_layouts), std::move(pipeline_layout));

	}

	void VulkanShaderDataSegment::Reset() {
		m_handle.emplace<std::vector<Declaration>>();
	}

	vk::PipelineLayout VulkanShaderDataSegment::GetPipelineLayout() const noexcept {
		auto* instantiated_data = std::get_if<InstantiatedData>(&m_handle);
		if (!instantiated_data) {
			return {};
		}
		return *instantiated_data->pipeline_layout;
		
	}
	
	std::pair<vk::DescriptorSetLayoutBinding const*, VulkanSampler const*> VulkanShaderDataSegment::GetLayoutBinding(std::size_t ns, std::size_t where) const noexcept {
		
		auto* instantiated_data = std::get_if<InstantiatedData>(&m_handle);
		if (!instantiated_data) {
			return {nullptr, nullptr};
		}
	
		auto set_it = instantiated_data->set_layouts.find(ns);
		if (set_it == instantiated_data->set_layouts.end()) {
			return {nullptr, nullptr};
		}
	
		auto const& set_layout = set_it->second;
		size_t sampler_index = 0;
	
		for (auto const& binding : set_layout.bindings) {
			if (binding.binding == where) {
				VulkanSampler const* sampler = nullptr;
				if (binding.descriptorType == vk::DescriptorType::eSampler) {
					auto sampler_it = set_layout.immutable_samplers.find(where);
					if (sampler_it != set_layout.immutable_samplers.end()) {
						sampler = &sampler_it->second;
					}
				}
				return { &binding, sampler };
			}
		}

		return { nullptr, nullptr };

	}

	std::vector<VulkanShaderDataSegment::ImmutableSamplerInfo> VulkanShaderDataSegment::GetImmutableSamplers() const {
		auto* instantiated_data = std::get_if<InstantiatedData>(&m_handle);
		if (!instantiated_data) {
			return {};
		}
	
		std::vector<ImmutableSamplerInfo> result;
		for (auto const& [set, set_layout] : instantiated_data->set_layouts) {
			for (auto const& binding : set_layout.bindings) {
				if (binding.descriptorType == vk::DescriptorType::eSampler) {
					auto it = set_layout.immutable_samplers.find(binding.binding);
					if (it != set_layout.immutable_samplers.end()) {
						result.emplace_back(							
						static_cast<std::uint32_t>(set),
						binding.binding,
						&it->second
					);
					}
				}
			}
		}
		return result;
	}

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)