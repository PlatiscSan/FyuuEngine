/* physical_device.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <string>
#include <string_view>
#endif
module fyuu_rhi:physical_device_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :enums;
import :physical_device;
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
	PhysicalDevice::PhysicalDevice(API api, InitOptions const& init)
		: m_impl(
			[api, &init]() -> UniquePhysicalDevice {
				switch (api) {
				case fyuu_rhi::API::PlatformDefault:
#if defined(_WIN32)
				case fyuu_rhi::API::DirectX12:
					return plastic::utility::MakeUnique<d3d12::D3D12PhysicalDevice>(init);
#endif // defined(_WIN32)
#if !defined(__APPLE__)
				case fyuu_rhi::API::Vulkan:
					return plastic::utility::MakeUnique<vulkan::VulkanPhysicalDevice>(init);
				case fyuu_rhi::API::OpenGL:
					return plastic::utility::MakeUnique<opengl::OpenGLPhysicalDevice>(init);
#endif // !defined(__APPLE__)
#if defined(__APPL__)
				case fyuu_rhi::API::Metal:
					return plastic::utility::MakeUnique<metal::MetalPhysicalDevice>(init);
#endif // defined(__APPL__)
				case fyuu_rhi::API::WebGPU:
					return plastic::utility::MakeUnique<webgpu::WebGPUPhysicalDevice>(init);

				default:
					throw std::runtime_error("Unsupported API");
				}
			}()) {

	}

	std::uint32_t PhysicalDevice::GetVendorID() const noexcept {
		return m_impl->Visit(
			[](auto* derived) {
				return derived->GetVendorID();
			}
		);
	}

	std::uint32_t PhysicalDevice::GetID() const noexcept {
		return m_impl->Visit(
			[](auto* derived) {
				return derived->GetID();
			}
		);
	}

	std::string PhysicalDevice::GetDescription() const {
		return m_impl->Visit(
			[](auto* derived) {
				return derived->GetDescription();
			}
		);
	}

	PolymorphicPhysicalDeviceBase* PhysicalDevice::GetHandle() const noexcept {
		return m_impl.get();
	}

    API PhysicalDevice::GetAPI() const noexcept {
        return m_impl->Visit(
			[](auto* derived) {
				return derived->GetAPI();
			}
		);
    }

    std::string_view PhysicalDevice::GetAPIString() const noexcept {
        return m_impl->Visit(
			[](auto* derived) {
				return derived->GetAPIString();
			}
		);
    }

}
