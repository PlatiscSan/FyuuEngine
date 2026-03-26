/* vulkan_sampler.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#include <vector>
#include <optional>
#include <variant>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
module fyuu_rhi:vulkan_sampler_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_sampler;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_logical_device;

namespace fyuu_rhi::vulkan {
        

	VulkanSampler::VulkanSampler(VulkanLogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment)
		: PolymorphicSamplerBase(this),
		VulkanCommon(this),
		m_impl(
			[&logical_device, flags, &attachment]() {
				if (HasConflictingFlags(flags, SamplerFlags::FilterModes))
					throw std::invalid_argument("Multiple filter modes in sampler flags");
				if (HasConflictingFlags(flags, SamplerFlags::AddressModes))
					throw std::invalid_argument("Multiple address modes in sampler flags");
				if (HasConflictingFlags(flags, SamplerFlags::CompareFunctions))
					throw std::invalid_argument("Multiple compare functions in sampler flags");
				if (HasConflictingFlags(flags, SamplerFlags::BorderColors))
					throw std::invalid_argument("Multiple border colors in sampler flags");
				if (HasConflictingFlags(flags, SamplerFlags::ReductionModes))
					throw std::invalid_argument("Multiple reduction modes in sampler flags");
				if (HasConflictingFlags(flags, SamplerFlags::AnisotropyModes))
					throw std::invalid_argument("Multiple anisotropy levels in sampler flags");
			
				vk::SamplerCreateInfo info;
			
				info.magFilter = ((flags & SamplerFlags::MagNearest) != SamplerFlags::Unknown) ? vk::Filter::eNearest : vk::Filter::eLinear;
				info.minFilter = ((flags & SamplerFlags::MinNearest) != SamplerFlags::Unknown) ? vk::Filter::eNearest : vk::Filter::eLinear;
				info.mipmapMode = ((flags & SamplerFlags::MipNearest) != SamplerFlags::Unknown) ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear;
			
				if ((flags & SamplerFlags::Repeat) != SamplerFlags::Unknown)
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eRepeat;
				else if ((flags & SamplerFlags::MirroredRepeat) != SamplerFlags::Unknown)
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eMirroredRepeat;
				else if ((flags & SamplerFlags::ClampToEdge) != SamplerFlags::Unknown)
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
				else if ((flags & SamplerFlags::ClampToBorder) != SamplerFlags::Unknown)
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eClampToBorder;
				else if ((flags & SamplerFlags::MirrorClampToEdge) != SamplerFlags::Unknown)
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eMirrorClampToEdge;
				else
					info.addressModeU = info.addressModeV = info.addressModeW = vk::SamplerAddressMode::eRepeat;
			
				if ((flags & SamplerFlags::Never) != SamplerFlags::Unknown)          info.compareOp = vk::CompareOp::eNever;
				else if ((flags & SamplerFlags::Less) != SamplerFlags::Unknown)      info.compareOp = vk::CompareOp::eLess;
				else if ((flags & SamplerFlags::Equal) != SamplerFlags::Unknown)     info.compareOp = vk::CompareOp::eEqual;
				else if ((flags & SamplerFlags::LessOrEqual) != SamplerFlags::Unknown) info.compareOp = vk::CompareOp::eLessOrEqual;
				else if ((flags & SamplerFlags::Greater) != SamplerFlags::Unknown)   info.compareOp = vk::CompareOp::eGreater;
				else if ((flags & SamplerFlags::NotEqual) != SamplerFlags::Unknown)  info.compareOp = vk::CompareOp::eNotEqual;
				else if ((flags & SamplerFlags::GreaterOrEqual) != SamplerFlags::Unknown) info.compareOp = vk::CompareOp::eGreaterOrEqual;
				else if ((flags & SamplerFlags::Always) != SamplerFlags::Unknown)    info.compareOp = vk::CompareOp::eAlways;
				else info.compareOp = vk::CompareOp::eAlways;
			
				if ((flags & SamplerFlags::FloatTransparentBlack) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eFloatTransparentBlack;
				else if ((flags & SamplerFlags::FloatOpaqueBlack) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
				else if ((flags & SamplerFlags::FloatOpaqueWhite) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
				else if ((flags & SamplerFlags::IntTransparentBlack) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eIntTransparentBlack;
				else if ((flags & SamplerFlags::IntOpaqueBlack) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eIntOpaqueBlack;
				else if ((flags & SamplerFlags::IntOpaqueWhite) != SamplerFlags::Unknown)
					info.borderColor = vk::BorderColor::eIntOpaqueWhite;
				else
					info.borderColor = vk::BorderColor::eFloatTransparentBlack;
			
				if ((flags & SamplerFlags::AnisotropyOff) != SamplerFlags::Unknown) {
					info.anisotropyEnable = false;
					info.maxAnisotropy = 1.0f;
				} else if ((flags & SamplerFlags::Anisotropy2x) != SamplerFlags::Unknown) {
					info.anisotropyEnable = true;
					info.maxAnisotropy = 2.0f;
				} else if ((flags & SamplerFlags::Anisotropy4x) != SamplerFlags::Unknown) {
					info.anisotropyEnable = true;
					info.maxAnisotropy = 4.0f;
				} else if ((flags & SamplerFlags::Anisotropy8x) != SamplerFlags::Unknown) {
					info.anisotropyEnable = true;
					info.maxAnisotropy = 8.0f;
				} else if ((flags & SamplerFlags::Anisotropy16x) != SamplerFlags::Unknown) {
					info.anisotropyEnable = true;
					info.maxAnisotropy = 16.0f;
				} else {
					info.anisotropyEnable = false;
					info.maxAnisotropy = 1.0f;
				}
			
				info.mipLodBias = attachment.mip_lod_bias;
				info.minLod = attachment.min_lod;
				info.maxLod = attachment.max_lod;
			
				vk::SamplerReductionModeCreateInfoEXT reduction_info;
				vk::SamplerReductionModeEXT reductionMode = vk::SamplerReductionModeEXT::eWeightedAverage;
				if ((flags & SamplerFlags::Min) != SamplerFlags::Unknown) {
					reductionMode = vk::SamplerReductionModeEXT::eMin;
				} else if ((flags & SamplerFlags::Max) != SamplerFlags::Unknown) {
					reductionMode = vk::SamplerReductionModeEXT::eMax;
				}
				if (reductionMode != vk::SamplerReductionModeEXT::eWeightedAverage) {
					reduction_info.reductionMode = reductionMode;
					info.setPNext(&reduction_info);
				}
			
				return logical_device.CreateSampler(info);

			}()) {

	}

	vk::Sampler VulkanSampler::GetNative() const noexcept {
		return *m_impl;
	}

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)