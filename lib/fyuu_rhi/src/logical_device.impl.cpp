/* logical_device.impl.cpp */
module;
#include <memory>
#include <string_view>
module fyuu_rhi:logical_device_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.sealed_polymorphism;
import :types;
import :physical_device;
import :logical_device;
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
	LogicalDevice::LogicalDevice(PhysicalDevice const& physical_device)
		: m_impl(
			[&physical_device]() {

				auto overload = plastic::utility::Overload{
#if !defined(__APPLE__)
					[](vulkan::VulkanPhysicalDevice const* physical_device) -> UniqueLogicalDevice {
						return plastic::utility::MakeUnique<vulkan::VulkanLogicalDevice>(*physical_device);
					},
					[](opengl::OpenGLPhysicalDevice const* physical_device) -> UniqueLogicalDevice {
						return plastic::utility::MakeUnique<opengl::OpenGLLogicalDevice>(*physical_device);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[](d3d12::D3D12PhysicalDevice const* physical_device) -> UniqueLogicalDevice {
						return plastic::utility::MakeUnique<d3d12::D3D12LogicalDevice>(*physical_device);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[](metal::MetalPhysicalDevice const* physical_device) -> UniqueLogicalDevice {
						return plastic::utility::MakeUnique<metal::MetalLogicalDevice>(*physical_device);
					}
#endif // defined(__APPLE__)
					[](webgpu::WebGPUPhysicalDevice const* physical_device) -> UniqueLogicalDevice {
						return plastic::utility::MakeUnique<webgpu::WebGPULogicalDevice>(*physical_device);
					}
				};

				return physical_device.GetHandle()->Visit(overload);

			}()) {
	}

	PolymorphicLogicalDeviceBase* LogicalDevice::GetHandle() const noexcept {
		return m_impl.get();
	}

    API LogicalDevice::GetAPI() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPI();
			}
		);
    }

	std::string_view LogicalDevice::GetAPIString() const noexcept {
        return m_impl->Visit(
			[](auto const* derived) {
				return derived->GetAPIString();
			}
		);
	}	


}
