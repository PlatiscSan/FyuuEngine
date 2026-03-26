/* sampler.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif
module fyuu_rhi:sampler_impl;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :sampler;
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

	Sampler::Sampler(LogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment)
		: m_impl(
			[&logical_device, flags, &attachment]() {
				
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[flags, &attachment](vulkan::VulkanLogicalDevice const* logical_device) -> UniqueSampler {
						return plastic::utility::MakeUnique<vulkan::VulkanSampler>(*logical_device, flags, attachment);
					},
					[flags, &attachment](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueSampler {
						return  plastic::utility::MakeUnique<opengl::OpenGLSampler>(*logical_device, flags, attachment);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[flags, &attachment](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueSampler {
						return plastic::utility::MakeUnique<d3d12::D3D12Sampler>(*logical_device, flags, attachment);
					},
#endif // defined(_WIN32)
				#if defined(__APPLE__)
					[flags, &attachment](metal::MetalLogicalDevice const* logical_device) -> UniqueSampler {
						return plastic::utility::MakeUnique<metal::MetalSampler>(*logical_device, flags, attachment);
					}
#endif // defined(__APPLE__)
					[flags, &attachment](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueSampler {
						return plastic::utility::MakeUnique<webgpu::WebGPUSampler>(*logical_device, flags, attachment);
					}
				};

				return logical_device.GetHandle()->Visit(visitor);
			}()) {

	}

	PolymorphicSamplerBase* Sampler::GetHandle() const noexcept {
		return m_impl.get();
	}

} // namespace fyuu_rhi
