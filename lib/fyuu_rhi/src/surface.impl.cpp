module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <string_view>
#include <print>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:surface_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :surface;
import :physical_device;
import :types;
import plastic.sealed_polymorphism;
#if !defined(__APPLE__)
import :vulkan;
import :opengl;
#elif 
import :metal;
#endif // !defined(__APPLE__)
#if defined(_WIN32)
import :d3d12;
#endif // defined(_WIN32)
import :webgpu;
namespace fyuu_rhi {

    Surface::Surface(PhysicalDevice const& physical_device, PlatformHandle const& handle)
        : m_impl(
            [&physical_device, &handle]() {
		        auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			        [&handle](vulkan::VulkanPhysicalDevice const* dev) -> UniqueSurface {
                        return plastic::utility::MakeUnique<vulkan::VulkanSurface>(*dev, handle);
                    },
                    [&handle](opengl::OpenGLPhysicalDevice const* dev) -> UniqueSurface {
                        return plastic::utility::MakeUnique<opengl::OpenGLSurface>(*dev, handle);
                    },
#endif // !defined(__APPLE__)
#if defined(_WIN32)
                    [&handle](d3d12::D3D12PhysicalDevice const* dev) -> UniqueSurface {
                        return plastic::utility::MakeUnique<d3d12::D3D12Surface>(*dev, handle);
                    },
#endif // defined(_WIN32)
#if defined(__APPLE__)
                    [&handle](metal::MetalPhysicalDevice const* dev) -> UniqueSurface {
                        return plastic::utility::MakeUnique<metal::MetalSurface>(*dev, handle);
                    },
#endif
                    [&handle](webgpu::WebGPUPhysicalDevice const* dev) -> UniqueSurface {
                        return plastic::utility::MakeUnique<webgpu::WebGPUSurface>(*dev, handle);
                    },
		        };

		        return physical_device.GetHandle()->Visit(visitor);

            }()) {

    }

    std::pair<std::size_t, std::size_t> Surface::GetSize() const noexcept {
        return m_impl->Visit(
            [](auto const* derived) {
                return derived->GetSize();
            }
        );
    }

    ::fyuu_rhi::PlatformHandle const& Surface::GetPlatformHandle() const noexcept {
        return m_impl->Visit(
            [](auto const* derived) -> ::fyuu_rhi::PlatformHandle const& {
                return derived->GetPlatformHandle();
            }
        );
    }

    PolymorphicSurfaceBase* Surface::GetHandle() const noexcept {
        return m_impl.get();
    }

    API Surface::GetAPI() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPI();
			}
		);
    }

	std::string_view Surface::GetAPIString() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPIString();
			}
		);
	}

}