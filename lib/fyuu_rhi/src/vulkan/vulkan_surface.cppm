/* vulkan_surface.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:vulkan_surface;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;

namespace fyuu_rhi::vulkan {
        
    export class VulkanSurface
        : public PolymorphicSurfaceBase,
		public VulkanCommon<VulkanSurface> {
	private:
        PlatformHandle m_handle;
		vk::UniqueSurfaceKHR m_impl;

	public:
		VulkanSurface(
			VulkanPhysicalDevice const& physical_device,
			PlatformHandle const& handle
		);

		std::pair<std::size_t, std::size_t> GetSize() const noexcept;

		PlatformHandle const& GetPlatformHandle() const noexcept;

		vk::SurfaceKHR GetNative() const noexcept;

	};

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)