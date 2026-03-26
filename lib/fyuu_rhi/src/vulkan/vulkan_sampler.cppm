/* vulkan_sampler.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <optional>
#include <variant>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:vulkan_sampler;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;

namespace fyuu_rhi::vulkan {
        
    export class VulkanSampler
        : public PolymorphicSamplerBase,
		public VulkanCommon<VulkanSampler> {
	private:
		vk::UniqueSampler m_impl;

	public:
		VulkanSampler(VulkanLogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment);

		vk::Sampler GetNative() const noexcept;

	};

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)