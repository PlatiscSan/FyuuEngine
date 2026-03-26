/* vulkan_surface.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
module fyuu_rhi:vulkan_surface_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_surface;
import vulkan;
import :types;
import :platform_handle;
import :vulkan_physical_device;
import :vulkan_common;

namespace fyuu_rhi::vulkan {

	VulkanSurface::VulkanSurface(
		VulkanPhysicalDevice const& physical_device,
		PlatformHandle const& handle
	) : PolymorphicSurfaceBase(this),
        VulkanCommon(this),
        m_handle(handle),
        m_impl(
            [&physical_device, &handle]() {
#if defined(_WIN32)
                vk::Win32SurfaceCreateInfoKHR info({}, GetModuleHandle(nullptr), handle.impl, nullptr);
                return physical_device.CreateSurface(info);
#elif defined(__linux__)
                return std::visit(
                    [](auto&& arg) -> vk::UniqueSurface {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, Xlib>) {
                            vk::XlibSurfaceCreateInfoKHR info({}, arg.display, arg.window);
                            return physical_device.CreateSurface(info);
                        } 
                        else if constexpr (std::is_same_v<T, Wayland>) {
                            vk::WaylandSurfaceCreateInfoKHR info({}, arg.display, arg.surface);
                            return physical_device.CreateSurface(info);
                        } else {
                            throw std::runtime_error("Invalid platform handle");
                        }
                    },
                    handle.impl
                );
#elif defined(__ANDROID__)
                vk::AndroidSurfaceCreateInfoKHR info({}, handle.impl);
                return physical_device.CreateSurface(info);
#endif // defined(_WIN32)
            }()) {

    }

    std::pair<std::size_t, std::size_t> VulkanSurface::GetSize() const noexcept {
        return ::fyuu_rhi::GetSize(m_handle);
    }

    PlatformHandle const &VulkanSurface::GetPlatformHandle() const noexcept {
        return m_handle;
    }

    vk::SurfaceKHR VulkanSurface::GetNative() const noexcept {
        return *m_impl;
    }

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)