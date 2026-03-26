/* shader_data_segment.impl.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:shader_data_segment_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :shader_data_segment;
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
	
	ShaderDataSegment::ShaderDataSegment(LogicalDevice const& logical_device)
		: m_impl(
			[&logical_device]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[](vulkan::VulkanLogicalDevice* logical_device) -> UniqueShaderDataSegment {
						return plastic::utility::MakeUnique<vulkan::VulkanShaderDataSegment>(*logical_device);
					},
					[](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueShaderDataSegment {
						return plastic::utility::MakeUnique<opengl::OpenGLShaderDataSegment>(*logical_device);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueShaderDataSegment {
						return plastic::utility::MakeUnique<d3d12::D3D12ShaderDataSegment>(*logical_device);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[](metal::MetalLogicalDevice const* logical_device) -> UniqueShaderDataSegment {
						return plastic::utility::MakeUnique<metal::MetalShaderDataSegment>(*logical_device);
					}
#endif // defined(__APPLE__)
					[](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueShaderDataSegment {
						return plastic::utility::MakeUnique<webgpu::WebGPUShaderDataSegment>(*logical_device);
					}
				};				
				return logical_device.GetHandle()->Visit(visitor);
			}()) {

	}

	ShaderDataSegment& ShaderDataSegment::Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		m_impl->Visit(
			[=](auto* derived) {
				derived->Declare(where, count, visible, ns);
			}
		);
		return *this;
	}

	ShaderDataSegment& ShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns) {
		m_impl->Visit(
			[=](auto* derived) {
				derived->Declare(flags, where, visible, ns);
			}
		);
		return *this;
	}

	ShaderDataSegment& ShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		m_impl->Visit(
			[=](auto* derived) {
				derived->Declare(flags, where, count, visible, ns);
			}
		);
		return *this;
	}

	ShaderDataSegment& ShaderDataSegment::Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns) {
		m_impl->Visit(
			[flags, &info, where, visible, ns](auto* derived) {
				derived->Declare(flags, info, where, visible, ns);
			}
		);
		return *this;
	}

	ShaderDataSegment& ShaderDataSegment::Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		m_impl->Visit(
			[=](auto* derived) {
				derived->Declare(flags, where, count, visible, ns);
			}
		);
		return *this;
	}

	void ShaderDataSegment::Instantiate() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Instantiate();
			}
		);
	}

	void ShaderDataSegment::Reset() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Reset();
			}
		);
	}

	PolymorphicShaderDataSegmentBase* ShaderDataSegment::GetHandle() const noexcept {
		return m_impl.get();
	}

} // namespace fyuu_rhi
