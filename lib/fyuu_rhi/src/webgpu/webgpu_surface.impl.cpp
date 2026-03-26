module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
#include "platform.hpp"
module fyuu_rhi:webgpu_surface_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_surface;
import :types;
import :webgpu_common;
import :webgpu_physical_device;
import :platform_handle;

namespace fyuu_rhi::webgpu {

    fyuu_rhi::webgpu::WebGPUSurface::WebGPUSurface(WebGPUPhysicalDevice const& physical_device, PlatformHandle const& handle) 
        : PolymorphicSurfaceBase(this),
        WebGPUCommon(this),
        m_impl(
            [&physical_device, &handle]() {
#if defined(_WIN32)
                wgpu::SurfaceSourceWindowsHWND win32_desc = {};
                win32_desc.hinstance = GetModuleHandle(nullptr);
                win32_desc.hwnd = handle.impl;
                wgpu::SurfaceDescriptor surface_desc = {};
                surface_desc.nextInChain = &win32_desc;

                return physical_device.CreateSurface(surface_desc);
#elif defined(__linux__)
                std::visit(
                    [&](auto&& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, Xlib>) {
                            wgpu::SurfaceDescriptorFromXlibWindow xlib_desc = {};
                            xlib_desc.display = arg.display;
                            xlib_desc.window = arg.window;

                            wgpu::SurfaceDescriptor surface_desc = {};
                            surface_desc.nextInChain = &xlib_desc;

                            return physical_device.CreateSurface(surface_desc);
                        }
                        else if constexpr (std::is_same_v<T, Wayland>) {
                            wgpu::SurfaceDescriptorFromWaylandSurface wayland_desc = {};
                            wayland_desc.display = arg.display;
                            wayland_desc.surface = arg.surface;

                            wgpu::SurfaceDescriptor surface_desc = {};
                            surface_desc.nextInChain = &wayland_desc;

                            return physical_device.CreateSurface(surface_desc);
                        }
                        else {
                            throw std::runtime_error("Unsupported Linux platform handle for WebGPU surface");
                        }
                    },
                    handle.impl
                );
#elif defined(__ANDROID__)
                wgpu::SurfaceDescriptorFromAndroidNativeWindow android_desc = {};
                android_desc.window = handle.impl;

                wgpu::SurfaceDescriptor surface_desc = {};
                surface_desc.nextInChain = &android_desc;

                return physical_device.CreateSurface(surface_desc);
#else
                throw std::runtime_error("Unsupported platform for WebGPU surface");
#endif
            }()) {
    }

    std::pair<std::size_t, std::size_t> WebGPUSurface::GetSize() const noexcept {
        return ::fyuu_rhi::GetSize(m_handle);
    }

    ::fyuu_rhi::PlatformHandle const &WebGPUSurface::GetPlatformHandle() const noexcept {
        return m_handle;
    }

    wgpu::Surface WebGPUSurface::GetNative() const noexcept {
        return m_impl;
    }

} // namespace fyuu_rhi::webgpu