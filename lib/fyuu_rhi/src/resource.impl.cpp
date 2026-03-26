/* resource.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif
module fyuu_rhi:resource_impl;
#if defined(__cpp_lib_modules)
import std;			  // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :resource;
import plastic.sealed_polymorphism;
import :types;
import :enums;
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

	Resource::Resource(LogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags) 
		: m_impl(
			[&logical_device, buffer_size, type, flags]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[buffer_size, type, flags](vulkan::VulkanLogicalDevice* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<vulkan::VulkanResource>(*logical_device, buffer_size, type, flags);
					},
					[buffer_size, type, flags](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<opengl::OpenGLResource>(*logical_device, buffer_size, type, flags);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[buffer_size, type, flags](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<d3d12::D3D12Resource>(*logical_device, buffer_size, type, flags);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[buffer_size, type, flags](metal::MetalLogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<metal::MetalResource>(*logical_device, buffer_size, type, flags);
					}
#endif // defined(__APPLE__)
					[buffer_size, type, flags](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<webgpu::WebGPUResource>(*logical_device, buffer_size, type, flags);
					}
				};
				return logical_device.GetHandle()->Visit(visitor);
			}()) {

	}

	Resource::Resource(LogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags) 
		: m_impl(
			[&logical_device, &texture_size, type, flags]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[&texture_size, type, flags](vulkan::VulkanLogicalDevice* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<vulkan::VulkanResource>(*logical_device, texture_size, type, flags);
					},
					[&texture_size, type, flags](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<opengl::OpenGLResource>(*logical_device, texture_size, type, flags);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[&texture_size, type, flags](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<d3d12::D3D12Resource>(*logical_device, texture_size, type, flags);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[&texture_size, type, flags](metal::MetalLogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<metal::MetalResource>(*logical_device, texture_size, type, flags);
					}
#endif // defined(__APPLE__)
					[&texture_size, type, flags](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueResource {
						return plastic::utility::MakeUnique<webgpu::WebGPUResource>(*logical_device, texture_size, type, flags);
					}
				};
				return logical_device.GetHandle()->Visit(visitor);
			}()) {

	}

	Resource::operator PolymorphicResourceBase* () const noexcept {
		return m_impl.get();
	}

	PolymorphicResourceBase* Resource::GetHandle() const noexcept {
		return m_impl.get();
	}

} // namespace fyuu_rhi
