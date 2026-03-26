module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string_view>
#include <span>
#include <print>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:command_buffer_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :command_buffer;
import :logical_device;
import :resource;
import :view;
import :gpu_executable;
import :sampler;
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

	CommandBuffer::CommandBuffer(LogicalDevice const& logical_device, CommandObjectType buffer_type)
		: m_impl(
			[&logical_device, buffer_type]() {
				auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[buffer_type](vulkan::VulkanLogicalDevice* logical_device) -> UniqueCommandBuffer {
						return plastic::utility::MakeUnique<vulkan::VulkanCommandBuffer>(*logical_device, buffer_type);
					},
					[buffer_type](opengl::OpenGLLogicalDevice const* logical_device) -> UniqueCommandBuffer {
						return plastic::utility::MakeUnique<opengl::OpenGLCommandBuffer>(*logical_device, buffer_type);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[buffer_type](d3d12::D3D12LogicalDevice const* logical_device) -> UniqueCommandBuffer {
						return plastic::utility::MakeUnique<d3d12::D3D12CommandBuffer>(*logical_device, buffer_type);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[buffer_type](metal::MetalLogicalDevice const* logical_device) -> UniqueCommandBuffer {
						return plastic::utility::MakeUnique<metal::MetalCommandBuffer>(*logical_device, buffer_type);
					}
#endif // defined(__APPLE__)
					[buffer_type](webgpu::WebGPULogicalDevice const* logical_device) -> UniqueCommandBuffer {
						return plastic::utility::MakeUnique<webgpu::WebGPUCommandBuffer>(*logical_device, buffer_type);
					}
				};

				return logical_device.GetHandle()->Visit(visitor);

			}()) {

	}

	void CommandBuffer::BeginRecording() {
		m_impl->Visit(
			[](auto* derived) {
				derived->BeginRecording();
			}
		);
	}

	void CommandBuffer::EndRecording() {
		m_impl->Visit(
			[](auto* derived) {
				derived->EndRecording();
			}
		);
	}

	void CommandBuffer::Reset() {
		m_impl->Visit(
			[](auto* derived) {
				derived->Reset();
			}
		);
	}

	CommandBuffer& CommandBuffer::SetFrame(PolymorphicResourceBase const* frame, PolymorphicViewBase const* frame_view) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* frame, vulkan::VulkanView const* frame_view) {
				command_buffer->SetFrame(*frame, *frame_view);
			},
			[](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* frame, opengl::OpenGLView const* frame_view) {
				command_buffer->SetFrame(*frame, *frame_view);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* frame, d3d12::D3D12View const* frame_view) {
				command_buffer->SetFrame(*frame, *frame_view);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* frame, metal::MetalView const* frame_view) {
				command_buffer->SetFrame(*frame, *frame_view);
			},
#endif // defined(__APPLE__)
			[](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* frame, webgpu::WebGPUView const* frame_view) {
				command_buffer->SetFrame(*frame, *frame_view);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("various API");
			}
		};

		plastic::utility::Visit(visitor, m_impl.get(), frame, frame_view);

		return *this;		
	}

	CommandBuffer& CommandBuffer::BeginRendering(
		std::span<PolymorphicResourceBase const* const> rts, 
		std::span<PolymorphicViewBase const* const> rtvs, 
		std::span<LoadOp const> rt_loads, 
		std::span<StoreOp const> rt_stores, 
		std::span<std::array<float, 4u> const> rt_clears
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[rt_loads, rt_stores, rt_clears](vulkan::VulkanCommandBuffer* command_buffer, std::span<vulkan::VulkanResource const* const> rts, std::span<vulkan::VulkanView const* const> rtvs) {
				command_buffer->BeginRendering(rts, rtvs, rt_loads, rt_stores, rt_clears);
			},
			[rt_loads, rt_stores, rt_clears](opengl::OpenGLCommandBuffer* command_buffer, std::span<opengl::OpenGLResource const* const> rts, std::span<opengl::OpenGLView const* const> rtvs) {
				command_buffer->BeginRendering(rts, rtvs, rt_loads, rt_stores, rt_clears);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[rt_loads, rt_stores, rt_clears](d3d12::D3D12CommandBuffer* command_buffer, std::span<d3d12::D3D12Resource const* const> rts, std::span<d3d12::D3D12View const* const> rtvs) {
				command_buffer->BeginRendering(rts, rtvs, rt_loads, rt_stores, rt_clears);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[rt_loads, rt_stores, rt_clears](metal::MetalCommandBuffer* command_buffer, std::span<metal::MetalResource const* const> rts, std::span<metal::MetalView const* const> rtvs) {
				command_buffer->BeginRendering(rts, rtvs, rt_loads, rt_stores, rt_clears);
			},
#endif // defined(__APPLE__)
			[rt_loads, rt_stores, rt_clears](webgpu::WebGPUCommandBuffer* command_buffer, std::span<webgpu::WebGPUResource const* const> rts, std::span<webgpu::WebGPUView const* const> rtvs) {
				command_buffer->BeginRendering(rts, rtvs, rt_loads, rt_stores, rt_clears);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("various API");
			}
		};

		plastic::utility::Visit(visitor, m_impl.get(), rts, rtvs);

		return *this;
	}


	// -------------------------------------------------------------------------
	// EndRendering
	CommandBuffer& CommandBuffer::EndRendering() {
		m_impl->Visit([](auto* derived) {
			derived->EndRendering();
		});
		return *this;
	}

	// -------------------------------------------------------------------------
	// SetViewports
	CommandBuffer& CommandBuffer::SetViewports(std::span<CmdSetViewports::Viewport const> viewports) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[viewports](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->SetViewports(viewports);
			},
			[viewports](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->SetViewports(viewports);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[viewports](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->SetViewports(viewports);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[viewports](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->SetViewports(viewports);
			},
#endif // defined(__APPLE__)
			[viewports](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->SetViewports(viewports);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in SetViewports");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// SetScissors
	CommandBuffer& CommandBuffer::SetScissors(std::span<CmdSetScissors::Scissor const> scissors) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[scissors](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->SetScissors(scissors);
			},
			[scissors](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->SetScissors(scissors);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[scissors](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->SetScissors(scissors);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[scissors](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->SetScissors(scissors);
			},
#endif // defined(__APPLE__)
			[scissors](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->SetScissors(scissors);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in SetScissors");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// Execute
	CommandBuffer& CommandBuffer::Execute(GPUExecutable const& gpu_exec) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanGPUExecutable const* exec) {
				command_buffer->Execute(*exec);
			},
			[](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLGPUExecutable const* exec) {
				command_buffer->Execute(*exec);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12GPUExecutable const* exec) {
				command_buffer->Execute(*exec);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[](metal::MetalCommandBuffer* command_buffer, metal::MetalGPUExecutable const* exec) {
				command_buffer->Execute(*exec);
			},
#endif // defined(__APPLE__)
			[](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUGPUExecutable const* exec) {
				command_buffer->Execute(*exec);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in Execute");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), gpu_exec.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// DispatchComputingTask
	CommandBuffer& CommandBuffer::DispatchComputingTask(
		std::size_t threads_x,
		std::size_t threads_y,
		std::size_t threads_z,
		std::size_t threads_per_thread_group_x,
		std::size_t threads_per_thread_group_y,
		std::size_t threads_per_thread_group_z
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[threads_x, threads_y, threads_z, threads_per_thread_group_x, threads_per_thread_group_y, threads_per_thread_group_z](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->DispatchComputingTask(
					threads_x, threads_y, threads_z,
					threads_per_thread_group_x,
					threads_per_thread_group_y,
					threads_per_thread_group_z
				);
			},
			[threads_x, threads_y, threads_z,threads_per_thread_group_x, threads_per_thread_group_y, threads_per_thread_group_z](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->DispatchComputingTask(
					threads_x, threads_y, threads_z,
					threads_per_thread_group_x,
					threads_per_thread_group_y,
					threads_per_thread_group_z
				);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[threads_x, threads_y, threads_z, threads_per_thread_group_x, threads_per_thread_group_y, threads_per_thread_group_z](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->DispatchComputingTask(
					threads_x, threads_y, threads_z,
					threads_per_thread_group_x,
					threads_per_thread_group_y,
					threads_per_thread_group_z
				);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[threads_x, threads_y, threads_z, threads_per_thread_group_x, threads_per_thread_group_y, threads_per_thread_group_z](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->DispatchComputingTask(
					threads_x, threads_y, threads_z,
					threads_per_thread_group_x,
					threads_per_thread_group_y,
					threads_per_thread_group_z
				);
			},
#endif // defined(__APPLE__)
			[threads_x, threads_y, threads_z,threads_per_thread_group_x, threads_per_thread_group_y, threads_per_thread_group_z](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->DispatchComputingTask(
					threads_x, threads_y, threads_z,
					threads_per_thread_group_x,
					threads_per_thread_group_y,
					threads_per_thread_group_z
				);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in DispatchComputingTask");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindVertexBuffers
	CommandBuffer& CommandBuffer::BindVertexBuffers(
		std::span<PolymorphicResourceBase const* const> vertex_buffers,
		std::span<std::size_t const> wheres,
		std::span<std::size_t const> offsets,
		std::span<std::size_t const> strides
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[wheres, offsets, strides](vulkan::VulkanCommandBuffer* command_buffer, std::span<vulkan::VulkanResource const* const> buffers) {
				command_buffer->BindVertexBuffers(buffers, wheres, offsets, strides);
			},
			[wheres, offsets, strides](opengl::OpenGLCommandBuffer* command_buffer, std::span<opengl::OpenGLResource const* const> buffers) {
				command_buffer->BindVertexBuffers(buffers, wheres, offsets, strides);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[wheres, offsets, strides](d3d12::D3D12CommandBuffer* command_buffer, std::span<d3d12::D3D12Resource const* const> buffers) {
				command_buffer->BindVertexBuffers(buffers, wheres, offsets, strides);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[wheres, offsets, strides](metal::MetalCommandBuffer* command_buffer, std::span<metal::MetalResource const* const> buffers) {
				command_buffer->BindVertexBuffers(buffers, wheres, offsets, strides);
			},
#endif // defined(__APPLE__)
			[wheres, offsets, strides](webgpu::WebGPUCommandBuffer* command_buffer, std::span<webgpu::WebGPUResource const* const> buffers) {
				command_buffer->BindVertexBuffers(buffers, wheres, offsets, strides);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindVertexBuffers");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), vertex_buffers);
		return *this;
	}

	// -------------------------------------------------------------------------
	// DrawInstanced
	CommandBuffer& CommandBuffer::DrawInstanced(
		std::size_t vertices,
		std::size_t instances,
		std::size_t start_vertex,
		std::size_t first_instance
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[vertices, instances, start_vertex, first_instance](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->DrawInstanced(vertices, instances, start_vertex, first_instance);
			},
			[vertices, instances, start_vertex, first_instance](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->DrawInstanced(vertices, instances, start_vertex, first_instance);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[vertices, instances, start_vertex, first_instance](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->DrawInstanced(vertices, instances, start_vertex, first_instance);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[vertices, instances, start_vertex, first_instance](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->DrawInstanced(vertices, instances, start_vertex, first_instance);
			},
#endif // defined(__APPLE__)
			[vertices, instances, start_vertex, first_instance](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->DrawInstanced(vertices, instances, start_vertex, first_instance);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in DrawInstanced");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// DrawIndexed
	CommandBuffer& CommandBuffer::DrawIndexed(
		std::size_t indices,
		std::size_t instances,
		std::size_t first_index,
		std::size_t start_vertex,
		std::size_t first_instance
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[indices, instances, first_index, start_vertex, first_instance](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->DrawIndexed(indices, instances, first_index, start_vertex, first_instance);
			},
			[indices, instances, first_index, start_vertex, first_instance](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->DrawIndexed(indices, instances, first_index, start_vertex, first_instance);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[indices, instances, first_index, start_vertex, first_instance](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->DrawIndexed(indices, instances, first_index, start_vertex, first_instance);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[indices, instances, first_index, start_vertex, first_instance](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->DrawIndexed(indices, instances, first_index, start_vertex, first_instance);
			},
#endif // defined(__APPLE__)
			[indices, instances, first_index, start_vertex, first_instance](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->DrawIndexed(indices, instances, first_index, start_vertex, first_instance);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in DrawIndexed");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindBuffer (Resource only)
	CommandBuffer& CommandBuffer::BindBuffer(
		ShaderStage stages,
		std::size_t where,
		Resource const& buffer,
		std::size_t offset,
		std::size_t ns
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stages, where, offset, ns](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf) {
				command_buffer->BindBuffer(stages, where, *buf, offset, ns);
			},
			[stages, where, offset, ns](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf) {
				command_buffer->BindBuffer(stages, where, *buf, offset, ns);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stages, where, offset, ns](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* buf) {
				command_buffer->BindBuffer(stages, where, *buf, offset, ns);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stages, where, offset, ns](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf) {
				command_buffer->BindBuffer(stages, where, *buf, offset, ns);
			},
#endif // defined(__APPLE__)
			[stages, where, offset, ns](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf) {
				command_buffer->BindBuffer(stages, where, *buf, offset, ns);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindBuffer (no view)");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindBuffer (with texel view)
	CommandBuffer& CommandBuffer::BindBuffer(
		ShaderStage stages,
		std::size_t where,
		Resource const& buffer,
		View const& texel_view,
		std::size_t offset,
		std::size_t ns
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stages, where, offset, ns](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf,vulkan::VulkanView const* view) {
				command_buffer->BindBuffer(stages, where, *buf, *view, offset, ns);
			},
			[stages, where, offset, ns](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf, opengl::OpenGLView const* view) {
				command_buffer->BindBuffer(stages, where, *buf, *view, offset, ns);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stages, where, offset, ns](d3d12::D3D12CommandBuffer* command_buffer,d3d12::D3D12Resource const* buf, d3d12::D3D12View const* view) {
				command_buffer->BindBuffer(stages, where, *buf, *view, offset, ns);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stages, where, offset, ns](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf, metal::MetalView const* view) {
				command_buffer->BindBuffer(stages, where, *buf, *view, offset, ns);
			},
#endif // defined(__APPLE__)
			[stages, where, offset, ns](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf, webgpu::WebGPUView const* view) {
				command_buffer->BindBuffer(stages, where, *buf, *view, offset, ns);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindBuffer (with view)");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), buffer.GetHandle(), texel_view.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// PushData
	CommandBuffer& CommandBuffer::PushData(
		ShaderStage stages,
		std::size_t where,
		std::span<std::byte const> bytes,
		std::size_t offset,
		std::size_t ns
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stages, where, bytes, offset, ns](vulkan::VulkanCommandBuffer* command_buffer) {
				command_buffer->PushData(stages, where, bytes, offset, ns);
			},
			[stages, where, bytes, offset, ns](opengl::OpenGLCommandBuffer* command_buffer) {
				command_buffer->PushData(stages, where, bytes, offset, ns);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stages, where, bytes, offset, ns](d3d12::D3D12CommandBuffer* command_buffer) {
				command_buffer->PushData(stages, where, bytes, offset, ns);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stages, where, bytes, offset, ns](metal::MetalCommandBuffer* command_buffer) {
				command_buffer->PushData(stages, where, bytes, offset, ns);
			},
#endif // defined(__APPLE__)
			[stages, where, bytes, offset, ns](webgpu::WebGPUCommandBuffer* command_buffer) {
				command_buffer->PushData(stages, where, bytes, offset, ns);
			},
			[](auto&&) {
				throw std::logic_error("Unsupported backend in PushData");
			}
		};
		m_impl->Visit(visitor);
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindSampler
	CommandBuffer& CommandBuffer::BindSampler(
		ShaderStage stages,
		std::size_t where,
		Sampler const& sampler,
		std::size_t ns
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stages, where, ns](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanSampler const* samp) {
				command_buffer->BindSampler(stages, where, *samp, ns);
			},
			[stages, where, ns](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLSampler const* samp) {
				command_buffer->BindSampler(stages, where, *samp, ns);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stages, where, ns](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Sampler const* samp) {
				command_buffer->BindSampler(stages, where, *samp, ns);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stages, where, ns](metal::MetalCommandBuffer* command_buffer, metal::MetalSampler const* samp) {
				command_buffer->BindSampler(stages, where, *samp, ns);
			},
#endif // defined(__APPLE__)
			[stages, where, ns](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUSampler const* samp) {
				command_buffer->BindSampler(stages, where, *samp, ns);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindSampler");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), sampler.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindTexture
	CommandBuffer& CommandBuffer::BindTexture(
		ShaderStage stages,
		std::size_t where,
		Resource const& texture,
		View const& view,
		std::size_t ns
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[stages, where, ns](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* tex, vulkan::VulkanView const* view) {
				command_buffer->BindTexture(stages, where, *tex, *view, ns);
			},
			[stages, where, ns](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* tex, opengl::OpenGLView const* view) {
				command_buffer->BindTexture(stages, where, *tex, *view, ns);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[stages, where, ns](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* tex, d3d12::D3D12View const* view) {
				command_buffer->BindTexture(stages, where, *tex, *view, ns);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[stages, where, ns](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* tex, metal::MetalView const* view) {
				command_buffer->BindTexture(stages, where, *tex, *view, ns);
			},
#endif // defined(__APPLE__)
			[stages, where, ns](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* tex, webgpu::WebGPUView const* view) {
				command_buffer->BindTexture(stages, where, *tex, *view, ns);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindTexture");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), texture.GetHandle(), view.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// CopyTextureToBuffer
	CommandBuffer& CommandBuffer::CopyTextureToBuffer(
		Resource const& src_texture,
		Resource const& dest_buffer,
		CmdCopyTextureToBuffer::Subresource const& src_subresource,
		CmdCopyTextureToBuffer::Offset3D const& src_offset,
		std::size_t dest_offset,
		CmdCopyTextureToBuffer::Extent3D const& extent
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[&src_subresource, &src_offset, dest_offset, &extent](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* src_tex, vulkan::VulkanResource const* dst_buf) {
				command_buffer->CopyTextureToBuffer(*src_tex, *dst_buf, src_subresource, src_offset, dest_offset, extent);
			},
			[&src_subresource, &src_offset, dest_offset, &extent](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* src_tex, opengl::OpenGLResource const* dst_buf) {
				command_buffer->CopyTextureToBuffer(*src_tex, *dst_buf, src_subresource, src_offset, dest_offset, extent);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[&src_subresource, &src_offset, dest_offset, &extent](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* src_tex, d3d12::D3D12Resource const* dst_buf) {
				command_buffer->CopyTextureToBuffer(*src_tex, *dst_buf, src_subresource, src_offset, dest_offset, extent);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[&src_subresource, &src_offset, dest_offset, &extent](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* src_tex, metal::MetalResource const* dst_buf) {
				command_buffer->CopyTextureToBuffer(*src_tex, *dst_buf, src_subresource, src_offset, dest_offset, extent);
			},
#endif // defined(__APPLE__)
			[&src_subresource, &src_offset, dest_offset, &extent](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* src_tex, webgpu::WebGPUResource const* dst_buf) {
				command_buffer->CopyTextureToBuffer(*src_tex, *dst_buf, src_subresource, src_offset, dest_offset, extent);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in CopyTextureToBuffer");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), src_texture.GetHandle(), dest_buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// CopyBufferToTexture
	CommandBuffer& CommandBuffer::CopyBufferToTexture(
		Resource const& src_buffer,
		Resource const& dest_texture,
		std::size_t src_offset,
		CmdCopyBufferToTexture::Subresource const& dest_subresource,
		CmdCopyBufferToTexture::Offset3D const& dest_offset,
		CmdCopyBufferToTexture::Extent3D const& extent
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[src_offset, &dest_subresource, &dest_offset, &extent](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* src_buf, vulkan::VulkanResource const* dst_tex) {
				command_buffer->CopyBufferToTexture(*src_buf, *dst_tex, src_offset, dest_subresource, dest_offset, extent);
			},
			[src_offset, &dest_subresource, &dest_offset, &extent](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* src_buf, opengl::OpenGLResource const* dst_tex) {
				command_buffer->CopyBufferToTexture(*src_buf, *dst_tex, src_offset, dest_subresource, dest_offset, extent);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[src_offset, &dest_subresource, &dest_offset, &extent](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* src_buf, d3d12::D3D12Resource const* dst_tex) {
				command_buffer->CopyBufferToTexture(*src_buf, *dst_tex, src_offset, dest_subresource, dest_offset, extent);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[src_offset, &dest_subresource, &dest_offset, &extent](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* src_buf, metal::MetalResource const* dst_tex) {
				command_buffer->CopyBufferToTexture(*src_buf, *dst_tex, src_offset, dest_subresource, dest_offset, extent);
			},
#endif // defined(__APPLE__)
			[src_offset, &dest_subresource, &dest_offset, &extent](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* src_buf, webgpu::WebGPUResource const* dst_tex) {
				command_buffer->CopyBufferToTexture(*src_buf, *dst_tex, src_offset, dest_subresource, dest_offset, extent);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in CopyBufferToTexture");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), src_buffer.GetHandle(), dest_texture.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// CopyBufferToBuffer
	CommandBuffer& CommandBuffer::CopyBufferToBuffer(
		Resource const& src_buffer,
		Resource const& dest_buffer,
		std::size_t src_offset,
		std::size_t dest_offset,
		std::size_t size
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[src_offset, dest_offset, size](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* src_buf, vulkan::VulkanResource const* dst_buf) {
				command_buffer->CopyBufferToBuffer(*src_buf, *dst_buf, src_offset, dest_offset, size);
			},
			[src_offset, dest_offset, size](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* src_buf, opengl::OpenGLResource const* dst_buf) {
				command_buffer->CopyBufferToBuffer(*src_buf, *dst_buf, src_offset, dest_offset, size);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[src_offset, dest_offset, size](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* src_buf, d3d12::D3D12Resource const* dst_buf) {
				command_buffer->CopyBufferToBuffer(*src_buf, *dst_buf, src_offset, dest_offset, size);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[src_offset, dest_offset, size](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* src_buf, metal::MetalResource const* dst_buf) {
				command_buffer->CopyBufferToBuffer(*src_buf, *dst_buf, src_offset, dest_offset, size);
			},
#endif // defined(__APPLE__)
			[src_offset, dest_offset, size](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* src_buf, webgpu::WebGPUResource const* dst_buf) {
				command_buffer->CopyBufferToBuffer(*src_buf, *dst_buf, src_offset, dest_offset, size);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in CopyBufferToBuffer");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), src_buffer.GetHandle(), dest_buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// CopyTextureToTexture
	CommandBuffer& CommandBuffer::CopyTextureToTexture(
		Resource const& src_texture,
		Resource const& dest_texture,
		CmdCopyTextureToTexture::Subresource const& src_subresource,
		CmdCopyTextureToTexture::Subresource const& dest_subresource,
		CmdCopyTextureToTexture::Offset3D const& src_offset,
		CmdCopyTextureToTexture::Offset3D const& dest_offset,
		CmdCopyTextureToTexture::Extent3D const& extent
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[&src_subresource, &dest_subresource, &src_offset, &dest_offset, &extent](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* src_tex, vulkan::VulkanResource const* dst_tex) {
				command_buffer->CopyTextureToTexture(
					*src_tex, *dst_tex,
					src_subresource, dest_subresource,
					src_offset, dest_offset, extent
				);
			},
			[&src_subresource, &dest_subresource, &src_offset, &dest_offset, &extent](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* src_tex, opengl::OpenGLResource const* dst_tex) {
				command_buffer->CopyTextureToTexture(
					*src_tex, *dst_tex,
					src_subresource, dest_subresource,
					src_offset, dest_offset, extent
				);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[&src_subresource, &dest_subresource, &src_offset, &dest_offset, &extent](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* src_tex, d3d12::D3D12Resource const* dst_tex) {
				command_buffer->CopyTextureToTexture(
					*src_tex, *dst_tex,
					src_subresource, dest_subresource,
					src_offset, dest_offset, extent
				);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[&src_subresource, &dest_subresource, &src_offset, &dest_offset, &extent](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* src_tex, metal::MetalResource const* dst_tex) {
				command_buffer->CopyTextureToTexture(
					*src_tex, *dst_tex,
					src_subresource, dest_subresource,
					src_offset, dest_offset, extent
				);
			},
#endif // defined(__APPLE__)
			[&src_subresource, &dest_subresource, &src_offset, &dest_offset, &extent](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* src_tex, webgpu::WebGPUResource const* dst_tex) {
				command_buffer->CopyTextureToTexture(
					*src_tex, *dst_tex,
					src_subresource, dest_subresource,
					src_offset, dest_offset, extent
				);
			},
			[](auto&&, auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in CopyTextureToTexture");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), src_texture.GetHandle(), dest_texture.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// BindIndexBuffer
	CommandBuffer& CommandBuffer::BindIndexBuffer(
		Resource const& index_buffer
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf) {
				command_buffer->BindIndexBuffer(*buf);
			},
			[](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf) {
				command_buffer->BindIndexBuffer(*buf);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* buf) {
				command_buffer->BindIndexBuffer(*buf);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf) {
				command_buffer->BindIndexBuffer(*buf);
			},
#endif // defined(__APPLE__)
			[](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf) {
				command_buffer->BindIndexBuffer(*buf);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in BindIndexBuffer");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), index_buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// ExecuteIndirect
	CommandBuffer& CommandBuffer::ExecuteIndirect(
		Resource const& argument_buffer,
		std::size_t offset_into_buffer,
		std::size_t draw_count
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[offset_into_buffer, draw_count](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf) {
				command_buffer->ExecuteIndirect(*buf, offset_into_buffer, draw_count);
			},
			[offset_into_buffer, draw_count](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf) {
				command_buffer->ExecuteIndirect(*buf, offset_into_buffer, draw_count);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[offset_into_buffer, draw_count](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* buf) {
				command_buffer->ExecuteIndirect(*buf, offset_into_buffer, draw_count);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[offset_into_buffer, draw_count](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf) {
				command_buffer->ExecuteIndirect(*buf, offset_into_buffer, draw_count);
			},
#endif // defined(__APPLE__)
			[offset_into_buffer, draw_count](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf) {
				command_buffer->ExecuteIndirect(*buf, offset_into_buffer, draw_count);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in ExecuteIndirect");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), argument_buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// ExecuteIndirectIndexed
	CommandBuffer& CommandBuffer::ExecuteIndirectIndexed(
		Resource const& argument_buffer,
		std::size_t offset_into_buffer,
		std::size_t draw_count
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[offset_into_buffer, draw_count](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf) {
				command_buffer->ExecuteIndirectIndexed(*buf, offset_into_buffer, draw_count);
			},
			[offset_into_buffer, draw_count](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf) {
				command_buffer->ExecuteIndirectIndexed(*buf, offset_into_buffer, draw_count);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[offset_into_buffer, draw_count](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* buf) {
				command_buffer->ExecuteIndirectIndexed(*buf, offset_into_buffer, draw_count);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[offset_into_buffer, draw_count]
			(metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf) {
				command_buffer->ExecuteIndirectIndexed(*buf, offset_into_buffer, draw_count);
			},
#endif // defined(__APPLE__)
			[offset_into_buffer, draw_count](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf) {
				command_buffer->ExecuteIndirectIndexed(*buf, offset_into_buffer, draw_count);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in ExecuteIndirectIndexed");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), argument_buffer.GetHandle());
		return *this;
	}

	// -------------------------------------------------------------------------
	// DispatchIndirect
	CommandBuffer& CommandBuffer::DispatchIndirect(
		Resource const& argument_buffer,
		std::size_t offset_into_buffer,
		std::size_t block_size_x,
		std::size_t block_size_y,
		std::size_t block_size_z
	) {
		auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
			[offset_into_buffer, block_size_x, block_size_y, block_size_z](vulkan::VulkanCommandBuffer* command_buffer, vulkan::VulkanResource const* buf) {
				command_buffer->DispatchIndirect(*buf, offset_into_buffer, block_size_x, block_size_y, block_size_z);
			},
			[offset_into_buffer, block_size_x, block_size_y, block_size_z](opengl::OpenGLCommandBuffer* command_buffer, opengl::OpenGLResource const* buf) {
				command_buffer->DispatchIndirect(*buf, offset_into_buffer, block_size_x, block_size_y, block_size_z);
			},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
			[offset_into_buffer, block_size_x, block_size_y, block_size_z](d3d12::D3D12CommandBuffer* command_buffer, d3d12::D3D12Resource const* buf) {
				command_buffer->DispatchIndirect(*buf, offset_into_buffer, block_size_x, block_size_y, block_size_z);
			},
#endif // defined(_WIN32)
#if defined(__APPLE__)
			[offset_into_buffer, block_size_x, block_size_y, block_size_z](metal::MetalCommandBuffer* command_buffer, metal::MetalResource const* buf) {
				command_buffer->DispatchIndirect(*buf, offset_into_buffer, block_size_x, block_size_y, block_size_z);
			},
#endif // defined(__APPLE__)
			[offset_into_buffer, block_size_x, block_size_y, block_size_z](webgpu::WebGPUCommandBuffer* command_buffer, webgpu::WebGPUResource const* buf) {
				command_buffer->DispatchIndirect(*buf, offset_into_buffer, block_size_x, block_size_y, block_size_z);
			},
			[](auto&&, auto&&) {
				throw std::logic_error("Unsupported backend in DispatchIndirect");
			}
		};
		plastic::utility::Visit(visitor, m_impl.get(), argument_buffer.GetHandle());
		return *this;
	}
	
}