/* gpu_executable.impl.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <string>
#include <optional>
#include <span>
#endif
module fyuu_rhi:gpu_executable_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :gpu_executable;
import plastic.sealed_polymorphism;
import :types;
import :enums;
import :logical_device;
import :shader;
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

	GPUExecutable::GPUExecutable(LogicalDevice const& logical_device)
		: m_impl(
			[&logical_device]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[](vulkan::VulkanLogicalDevice const* logical_device) -> UniqueGPUExecutable {
						return plastic::utility::MakeUnique<vulkan::VulkanGPUExecutable>(*logical_device);
					},
					[](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueGPUExecutable {
						return plastic::utility::MakeUnique<opengl::OpenGLGPUExecutable>(*logical_device);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueGPUExecutable {
						return plastic::utility::MakeUnique<d3d12::D3D12GPUExecutable>(*logical_device);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[](metal::MetalLogicalDevice const* logical_device) -> UniqueGPUExecutable {
						return plastic::utility::MakeUnique<metal::MetalGPUExecutable>(*logical_device);
					}
#endif // defined(__APPLE__)
					[](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueGPUExecutable {
							return plastic::utility::MakeUnique<webgpu::WebGPUGPUExecutable>(*logical_device);
					}
				};

				return logical_device.GetHandle()->Visit(visitor);

			}()) {
	}

	GPUExecutable& GPUExecutable::SetShader(ShaderStage stage, Shader const& shader) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stage](vulkan::VulkanGPUExecutable* rendering_gpu_executable, vulkan::VulkanShader const* shader) {
				rendering_gpu_executable->SetShader(stage, *shader);
			},
			[stage](opengl::OpenGLGPUExecutable* rendering_gpu_executable, opengl::OpenGLShader const* shader) {
				rendering_gpu_executable->SetShader(stage, *shader);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stage](d3d12::D3D12GPUExecutable* rendering_gpu_executable, d3d12::D3D12Shader const* shader) {
				rendering_gpu_executable->SetShader(stage, *shader);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stage](metal::MetalGPUExecutable* rendering_gpu_executable, metal::MetalShader const* shader) {
				rendering_gpu_executable->SetShader(stage, *shader);
			}
#endif // defined(__APPLE__)
			[stage](webgpu::WebGPUGPUExecutable* rendering_gpu_executable, webgpu::WebGPUShader const* shader) {
				rendering_gpu_executable->SetShader(stage, *shader);
			},
			[](auto* rendering_gpu_executable, auto const* shader) {

			}
		};

		plastic::utility::Visit(
			visitor,
			m_impl.get(),
			shader.GetHandle()
		);

		return *this;

	}

	GPUExecutable& GPUExecutable::SetShaderDataSegment(ShaderDataSegment const& segment) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[](vulkan::VulkanGPUExecutable* rendering_gpu_executable, vulkan::VulkanShaderDataSegment const* shader_data_segment) {
				rendering_gpu_executable->SetShaderDataSegment(*shader_data_segment);
			},
			[](opengl::OpenGLGPUExecutable* rendering_gpu_executable, opengl::OpenGLShaderDataSegment const* shader_data_segment) {
				rendering_gpu_executable->SetShaderDataSegment(*shader_data_segment);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[](d3d12::D3D12GPUExecutable* rendering_gpu_executable, d3d12::D3D12ShaderDataSegment const* shader_data_segment) {
				rendering_gpu_executable->SetShaderDataSegment(*shader_data_segment);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[](metal::MetalGPUExecutable* rendering_gpu_executable, metal::MetalShaderDataSegment const* shader_data_segment) {
				rendering_gpu_executable->SetShaderDataSegment(*shader_data_segment);
			}
#endif // defined(__APPLE__)
			[](webgpu::WebGPUGPUExecutable* rendering_gpu_executable, webgpu::WebGPUShaderDataSegment const* shader_data_segment) {
				rendering_gpu_executable->SetShaderDataSegment(*shader_data_segment);
			},
			[](auto* rendering_gpu_executable, auto const* shader_data_segment) {

			}
		};

		plastic::utility::Visit(
			visitor,
			m_impl.get(),
			segment.GetHandle()
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetFlags(GPUExecutableFlags const& flags) {
		
		m_impl->Visit(
			[&flags](auto* derived) {
				derived->SetFlags(flags);
			}
		);

		return *this;

	}

	GPUExecutable& GPUExecutable::SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info) {
		
		m_impl->Visit(
			[&info](auto* derived) {
				derived->SetRenderTargetAttachments(info);
			}
		);

		return *this;

	}

	GPUExecutable& GPUExecutable::SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks) {
		m_impl->Visit(
			[blend_info, color_write_masks](auto* derived) {
				derived->SetRenderTargetAttachments(blend_info, color_write_masks);
			}
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks) {
		m_impl->Visit(
			[logic_op, color_write_masks](auto* derived) {
				derived->SetRenderTargetAttachments(logic_op, color_write_masks);
			}
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetDepthBias(DepthBias const& depth_bias) {
		m_impl->Visit(
			[&depth_bias](auto* derived) {
				derived->SetDepthBias(depth_bias);
			}
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetDepthStencil(DepthStencilInfo const& depth_stencil) {
		m_impl->Visit(
			[&depth_stencil](auto* derived) {
				derived->SetDepthStencil(depth_stencil);
			}
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetMultiSample(MultiSample const& multi_sample) {
		m_impl->Visit(
			[&multi_sample](auto* derived) {
				derived->SetMultiSample(multi_sample);
			}
		);

		return *this;
	}

	GPUExecutable& GPUExecutable::SetRenderTargetFormats(std::span<ResourceFlags const> format) {
		m_impl->Visit(
			[format](auto* derived) {
				derived->SetRenderTargetFormats(format);
			}
		);
		return *this;
	}

	GPUExecutable& GPUExecutable::SetVertexInputLayout(VertexInputLayout const& vertex_input_layout) {
		m_impl->Visit(
			[&vertex_input_layout](auto* derived) {
				derived->SetVertexInputLayout(vertex_input_layout);
			}
		);
		return *this;
	}

	GPUExecutable& GPUExecutable::SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes) {
		m_impl->Visit(
			[bindings, attributes](auto* derived) {
				derived->SetVertexInputLayout(bindings, attributes);
			}
		);
		return *this;
	}

	GPUExecutable& GPUExecutable::AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute) {
		m_impl->Visit(
			[&binding, &attribute](auto* derived) {
				derived->AddVertexInputLayout(binding, attribute);
			}
		);
		return *this;
	}

	GPUExecutable& GPUExecutable::AddVertexBinding(VertexBinding const& binding) {
		m_impl->Visit(
			[&binding](auto* derived) {
				derived->AddVertexBinding(binding);
			}
		);
		return *this;
	}

	GPUExecutable& GPUExecutable::AddVertexAttribute(VertexAttribute const& attribute) {
		m_impl->Visit(
			[&attribute](auto* derived) {
				derived->AddVertexAttribute(attribute);
			}
		);
		return *this;
	}

	void GPUExecutable::Compile() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Compile();
			}
		);
	}

	void GPUExecutable::Reset() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Reset();
			}
		);
	}

	PolymorphicGPUExecutableBase* GPUExecutable::GetHandle() const noexcept {
		return m_impl.get();
	}

} // namespace fyuu_rhi
