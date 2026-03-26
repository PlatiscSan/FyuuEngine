/* d3d12_command_buffer.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <ranges>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <coroutine>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "declare_pool.hpp"
module fyuu_rhi:d3d12_command_buffer_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_command_buffer;
import plastic.sealed_polymorphism;
import plastic.serial_task;
import :types;
import :synchronization;
import :d3d12_future;
import :d3d12_common;
import :d3d12_logical_device;
import :d3d12_types;
import :d3d12_throw;
import :d3d12_resource;
import :d3d12_view;
import :d3d12_gpu_executable;
import :d3d12_resource;
import :d3d12_sampler;
import :d3d12_shader_data_segment;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::d3d12;

	D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToD3D12LoadOp(LoadOp op) noexcept {
		switch (op) {
		case LoadOp::Load:      return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		case LoadOp::Clear:     return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		case LoadOp::DontCare:  return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		default: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		}
	}
	
	D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ToD3D12StoreOp(StoreOp op) noexcept {
		switch (op) {
		case StoreOp::Store:    return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		case StoreOp::DontCare: return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		default: return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		}
	}

	std::uint32_t GetBytesPerPixel(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16u;
		case DXGI_FORMAT_R32G32B32_FLOAT:    return 12u;
		case DXGI_FORMAT_R16G16B16A16_FLOAT: return 8u;
		case DXGI_FORMAT_R32G32_FLOAT:       return 8u;
		case DXGI_FORMAT_R8G8B8A8_UNORM:     return 4u;
		case DXGI_FORMAT_R16G16_FLOAT:       return 4u;
		case DXGI_FORMAT_R32_FLOAT:          return 4u;
		case DXGI_FORMAT_R8_UNORM:           return 1u;
		default: throw std::invalid_argument("Unsupported texture format for copy");
		}
	}

    std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

	void AddBarrier(D3D12_RESOURCE_STATES dest_state, D3D12Resource* res, auto&& barriers) {
		auto original_state = res->GetState();
		if (original_state != dest_state) {
			barriers.emplace_back(
				CD3DX12_RESOURCE_BARRIER::Transition(
					res->GetRawNative(), 
					original_state, 
					dest_state
				)
			);
			res->SetState(dest_state);
		}
	}


	enum class WaitFor : std::uint8_t {
		Frame,
		RenderingStart,
		RenderingEnd,
		GraphicsPipeline,
		ComputePipeline,
		RayTracingPipeline,
		IndexType,
		Count
	};

	enum class ContextType : std::uint8_t { 
		Graphics, 
		Compute, 
		RayTracing 
	};

	class StateMachine {
	private:
		D3D12GPUExecutable* m_graphics_exec = nullptr;
		D3D12GPUExecutable* m_compute_exec = nullptr;
		D3D12GPUExecutable* m_ray_tracing_exec = nullptr;
	
		D3D12Resource* m_frame_texture = nullptr;
		D3D12View* m_frame_view = nullptr;
		D3D12Resource* m_depth_stencil = nullptr;
		std::vector<D3D12Resource*> m_render_targets;
	
		std::array<std::deque<std::coroutine_handle<>>, static_cast<std::size_t>(WaitFor::Count)> m_pending_queues;

		DXGI_FORMAT m_index_type;

		bool m_is_rendering = false;

		bool IsConditionSatisfied(WaitFor type) const {
			switch (type) {
			case WaitFor::Frame:				return m_frame_texture && m_frame_view;
			case WaitFor::RenderingStart:		return m_is_rendering;
			case WaitFor::RenderingEnd:			return !m_is_rendering;
			case WaitFor::GraphicsPipeline:		return m_graphics_exec != nullptr;
			case WaitFor::ComputePipeline:		return m_compute_exec != nullptr;
			case WaitFor::RayTracingPipeline:	return m_ray_tracing_exec != nullptr;
			case WaitFor::IndexType:			return m_index_type != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			default:
				throw std::invalid_argument("Invalid wait type");
			}
			return false;
		}
	
		void AddPending(std::coroutine_handle<> h, WaitFor type) {
			m_pending_queues[static_cast<std::size_t>(type)].emplace_back(h);
		}
	
		void ResumeIfReady(WaitFor type) {
			auto& queue = m_pending_queues[static_cast<std::size_t>(type)];
			std::size_t queue_size = queue.size();
			for (std::size_t i = 0; i < queue_size; ++i) {
				auto h = queue.front();
				queue.pop_front();
				if (IsConditionSatisfied(type)) {
					h();
				} else {
					queue.emplace_back(h);
				}
			}
		}
	
	public:
		std::span<D3D12Resource* const> GetRenderTargets() const { 
			return m_render_targets;
		}
		
		D3D12Resource* GetDepthStencil() const { 
			return m_depth_stencil; 
		}
	
		void SetFrame(D3D12Resource* frame_texture, D3D12View* frame_view) {
			m_frame_texture = frame_texture;
			m_frame_view = frame_view;
			ResumeIfReady(WaitFor::Frame);
		}

		void BeginRendering(std::span<D3D12Resource* const> rt, D3D12Resource* ds) {
			m_render_targets.assign(rt.begin(), rt.end());
			m_depth_stencil = ds;
			m_is_rendering = true;
			ResumeIfReady(WaitFor::RenderingStart);
		}
	
		void EndRendering() {
			m_is_rendering = false;
			m_render_targets.clear();
			m_depth_stencil = nullptr;
			m_index_type = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			ResumeIfReady(WaitFor::RenderingEnd);
		}
	
		void SetGraphicsExecutable(D3D12GPUExecutable* exec) {
			if (!m_is_rendering) throw std::logic_error("Graphics executable must be set inside a rendering instance");
			m_graphics_exec = exec;
			ResumeIfReady(WaitFor::GraphicsPipeline);
		}
	
		void SetComputeExecutable(D3D12GPUExecutable* exec) {
			if (m_is_rendering) throw std::logic_error("Compute executable cannot be set inside a rendering instance");
			m_compute_exec = exec;
			ResumeIfReady(WaitFor::ComputePipeline);
		}

		void SetRayTracingExecutable(D3D12GPUExecutable* exec) {
			if (m_is_rendering) throw std::logic_error("Ray tracing executable cannot be set inside a rendering instance");
			m_ray_tracing_exec = exec;
			ResumeIfReady(WaitFor::RayTracingPipeline);
		}

		void SetIndexType(DXGI_FORMAT format) {
			m_index_type = format;
			ResumeIfReady(WaitFor::IndexType);
		}
	
		auto WaitForFrame() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_frame_texture && sm.m_frame_view; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::Frame); }
				std::pair<D3D12Resource*, D3D12View*> await_resume() const {
					return { sm.m_frame_texture, sm.m_frame_view }; 
				}
			};
			return Awaiter{*this};
		}

		auto WaitForRenderingStart() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_is_rendering; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::RenderingStart); }
				void await_resume() const {}
			};
			return Awaiter{*this};
		}
	
		auto WaitForRenderingEnd() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return !sm.m_is_rendering; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::RenderingEnd); }
				void await_resume() const {}
			};
			return Awaiter{*this};
		}
	
		auto WaitForIndexType() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_index_type != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::IndexType); }
				DXGI_FORMAT await_resume() const {
					return sm.m_index_type;
				}
			};
			return Awaiter{*this};		
		}

		auto WaitForExecutable(ContextType type) {
			struct Awaiter {
				StateMachine& sm;
				ContextType type;
				bool await_ready() const {
					switch (type) {
					case ContextType::Graphics:
						return sm.m_graphics_exec;
					case ContextType::Compute:
						return sm.m_compute_exec;
					case ContextType::RayTracing:
						return sm.m_ray_tracing_exec;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					switch (type) {
					case ContextType::Graphics:
						sm.AddPending(h, WaitFor::GraphicsPipeline);
						break;
					case ContextType::Compute:
						sm.AddPending(h, WaitFor::ComputePipeline);
						break;
					case ContextType::RayTracing:
						sm.AddPending(h, WaitFor::RayTracingPipeline);
						break;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				D3D12GPUExecutable* await_resume() const {
					switch (type) {
					case ContextType::Graphics:
						return sm.m_graphics_exec;
					case ContextType::Compute:
						return sm.m_compute_exec;
					case ContextType::RayTracing:
						return sm.m_ray_tracing_exec;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}			
				}
			};
			return Awaiter{*this, type};
		}

		auto WaitForContext(ContextType type) {
			struct Awaiter {
				StateMachine& sm;
				ContextType type;
				bool await_ready() const {
					switch (type) {
					case ContextType::Graphics:
						return sm.m_is_rendering;
					case ContextType::Compute:
						return !sm.m_is_rendering;
					case ContextType::RayTracing:
						return !sm.m_is_rendering;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					switch (type) {
					case ContextType::Graphics:
						if (!sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingStart);
						break;
					case ContextType::Compute:
						if (sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingEnd);
						break;
					case ContextType::RayTracing:
						if (sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingEnd);
						break;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_resume() const {}
			};
			return Awaiter{*this, type};
		}
	
		void Finalize() {
			for (std::size_t i = 0; i < static_cast<std::size_t>(WaitFor::Count); ++i) {
				auto& queue = m_pending_queues[i];
				while (!queue.empty()) {
					auto h = queue.front();
					queue.pop_front();
					if (IsConditionSatisfied(static_cast<WaitFor>(i))) {
						h();
					} else {
						h.destroy();
					}
				}
			}
		}
	};

	ContextType GetContextType(ShaderStage stages) {
		if (HasCrossGroupConflicts(stages, ShaderStage::Graphics, ShaderStage::Compute, ShaderStage::RayTracing)) {
			throw std::invalid_argument("Invalid parameter: Conflicting shader stage");
		}
		if ((stages & ShaderStage::Graphics) != ShaderStage::Unknown) {
			return ContextType::Graphics;
		}
		if ((stages & ShaderStage::Compute) != ShaderStage::Unknown) {
			return ContextType::Compute;
		}
		if ((stages & ShaderStage::RayTracing) != ShaderStage::Unknown) {
			return ContextType::RayTracing;
		}
		throw std::invalid_argument("Invalid parameter: No shader stage specified.");
	}

}

namespace fyuu_rhi::d3d12 {

	D3D12CommandBuffer::D3D12CommandBuffer(
		D3D12LogicalDevice const& logical_device,
		CommandObjectType type,
		std::wstring_view debug_name
	) : PolymorphicCommandBufferBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_impl(logical_device.CreateCommandList(ToD3D12CommandListType(type), debug_name)),
		m_commands(),
		m_sync(),
		m_recording_flag(false) {

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BeginRecording() {

		m_sync.Acquire();

		if (!std::exchange(m_recording_flag, true)) {
			ID3D12CommandAllocator* command_allocator;
			UINT data_size = sizeof(command_allocator);
			ThrowIfFailed(m_impl->GetPrivateData(__uuidof(ID3D12CommandAllocator), &data_size, &command_allocator));
			ThrowIfFailed(command_allocator->Reset());	// gotta reset this too, otherwise we leak
			ThrowIfFailed(m_impl->Reset(command_allocator, nullptr));
			m_commands.clear();
		}

		return *this;

	}

	void D3D12CommandBuffer::EndRecording() {

		m_sync.ThrowIfNotOwned();

		if (std::exchange(m_recording_flag, false) && !m_commands.empty()) {

			StateMachine state_machine;

			using GPUTask = plastic::concurrency::SerialTask<void>;

			auto visitor = plastic::utility::Overload{
				[&state_machine](CmdSetFrame const* set_frame) -> GPUTask {

					auto const& frame_id = set_frame->texture_id;
					if (!frame_id) {
						throw std::invalid_argument("Invalid frame");
					}

					D3D12Resource* frame = plastic::utility::QueryObject<D3D12Resource>(!frame_id);
					if (!frame) {
						throw std::runtime_error("Frame lost");
					}

					auto const& view_id = set_frame->view_id;
					if (!view_id) {
						throw std::invalid_argument("Invalid frame view");
					}

					D3D12View* frame_view = plastic::utility::QueryObject<D3D12View>(*view_id);
					if (!frame_view) {
						throw std::runtime_error("Frame view lost");
					}

					state_machine.SetFrame(frame, frame_view);

					co_return;

				},
				[this, &state_machine](CmdBeginRendering const* begin_rendering) -> GPUTask {

					std::vector<D3D12_RESOURCE_BARRIER> barriers;
					std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> rt_attachments;
					std::vector<D3D12Resource*> rts;

					std::size_t rt_descs_size = begin_rendering->render_targets.size();
					for (std::size_t i = 0; i < rt_descs_size; ++i) {
						
						auto const& rt_desc = begin_rendering->render_targets[i];

						auto const& rt_id = rt_desc.texture_id;
						if (!rt_id) {
							throw std::invalid_argument("Invalid render target");
						}

						D3D12Resource* rt = plastic::utility::QueryObject<D3D12Resource>(*rt_id);
						if (!rt) {
							throw std::runtime_error("Render target lost");
						}

						auto const& rtv_id = rt_desc.view_id;
						if (!rtv_id) {
							throw std::invalid_argument("Invalid render target view");
						}

						D3D12View* rtv = plastic::utility::QueryObject<D3D12View>(*rtv_id);
						if (!rtv) {
							throw std::runtime_error("Render target view lost");
						}

						AddBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, rt, barriers);

						D3D12_RENDER_PASS_RENDER_TARGET_DESC& desc = rt_attachments.emplace_back();
						desc.cpuDescriptor = rtv->GetNative().first;
						if (rt_desc.load == LoadOp::Clear) {
							desc.BeginningAccess.Clear.ClearValue.Format = rt->GetDescription().Format;
							desc.BeginningAccess.Clear.ClearValue.Color[0] = rt_desc.clear[0];
							desc.BeginningAccess.Clear.ClearValue.Color[1] = rt_desc.clear[1];
							desc.BeginningAccess.Clear.ClearValue.Color[2] = rt_desc.clear[2];
							desc.BeginningAccess.Clear.ClearValue.Color[3] = rt_desc.clear[3];
						}

						desc.BeginningAccess.Type = ToD3D12LoadOp(rt_desc.load);
						desc.EndingAccess.Type = ToD3D12StoreOp(rt_desc.store);

						rts.emplace_back(rt);

					}

					std::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> ds_attachment;
					D3D12Resource* depth_stencil = nullptr;
					auto& ds_desc = begin_rendering->depth_stencil;
					if (ds_desc) {

						D3D12_RENDER_PASS_DEPTH_STENCIL_DESC& emplaced = ds_attachment.emplace();

						auto const& ds_id = ds_desc->texture_id;
						if (!ds_id) {
							throw std::invalid_argument("Invalid depth stencil");
						}

						depth_stencil = plastic::utility::QueryObject<D3D12Resource>(*ds_id);
						if (!depth_stencil) {
							throw std::runtime_error("Depth stencil lost");
						}

						auto& dsv_id = ds_desc->view_id;
						if (!dsv_id) {
							throw std::invalid_argument("Invalid depth stencil view");
						}

						D3D12View* dsv = plastic::utility::QueryObject<D3D12View>(*dsv_id);
						if (!dsv) {
							throw std::runtime_error("Depth stencil view lost");
						}

						AddBarrier(D3D12_RESOURCE_STATE_DEPTH_WRITE, depth_stencil, barriers);

						emplaced.cpuDescriptor = dsv->GetNative().first;

						emplaced.DepthBeginningAccess.Type = ToD3D12LoadOp(ds_desc->d_load);
						if (ds_desc->d_load == LoadOp::Clear) {
							emplaced.DepthBeginningAccess.Clear.ClearValue.Format = depth_stencil->GetDescription().Format;
							emplaced.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = ds_desc->depth_clear;
							emplaced.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = ds_desc->stencil_clear;
						}
						emplaced.DepthEndingAccess.Type = ToD3D12StoreOp(ds_desc->d_store);
	
						emplaced.StencilBeginningAccess.Type = ToD3D12LoadOp(ds_desc->s_load);
						if (ds_desc->s_load == LoadOp::Clear) {
							emplaced.StencilBeginningAccess.Clear.ClearValue.Format = depth_stencil->GetDescription().Format;
							emplaced.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = ds_desc->stencil_clear;
						}
				
						emplaced.StencilEndingAccess.Type = ToD3D12StoreOp(ds_desc->s_store);						

					}

					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}

					m_impl->BeginRenderPass(
						static_cast<UINT>(rt_attachments.size()),
						rt_attachments.data(),
						ds_attachment ? &*ds_attachment : nullptr,
						D3D12_RENDER_PASS_FLAG_NONE
					);

					state_machine.BeginRendering(rts, depth_stencil);

					co_return;

				},
				[this, &state_machine](CmdEndRendering const*) -> GPUTask {
					
					co_await state_machine.WaitForRenderingStart();

					std::vector<D3D12_RESOURCE_BARRIER> barriers;

					for (auto& rt : state_machine.GetRenderTargets()) {
						AddBarrier(D3D12_RESOURCE_STATE_PRESENT, rt, barriers);
					}			

					auto* depth_stencil = state_machine.GetDepthStencil();
					if (depth_stencil) {
						AddBarrier(D3D12_RESOURCE_STATE_DEPTH_READ, depth_stencil, barriers);
					}

					m_impl->EndRenderPass();

					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}

					state_machine.EndRendering();

					co_return;

				},
				[this, &state_machine](CmdSetViewports const* set_viewport) -> GPUTask {
					
					co_await state_machine.WaitForRenderingStart();

					std::vector<D3D12_VIEWPORT> viewports;

					std::ranges::transform(
						set_viewport->viewports, 
						std::back_inserter(viewports), 
						[](CmdSetViewports::Viewport const& v) -> D3D12_VIEWPORT {
							return {
								v.x,
								v.y,
								v.width,
								v.height,
								v.min_depth,
								v.max_depth
							};
						}
					);

					m_impl->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());

					co_return;

				},
				[this, &state_machine](CmdSetScissors const* set_scissors) -> GPUTask {
					
					co_await state_machine.WaitForRenderingStart();

					std::vector<D3D12_RECT> scissors;

					std::ranges::transform(
						set_scissors->scissors, 
						std::back_inserter(scissors), 
						[](CmdSetScissors::Scissor const& s) -> D3D12_RECT {
							return {
								static_cast<LONG>(s.x),
								static_cast<LONG>(s.y),
								static_cast<LONG>(s.width),
								static_cast<LONG>(s.height)
							};
						}
					);

					m_impl->RSSetScissorRects(static_cast<UINT>(scissors.size()), scissors.data());
					
					co_return;

				},
				[this, &state_machine](CmdExecute const* execute) -> GPUTask {

					auto const& gpu_exec_id = execute->gpu_exec_id;
					if (!gpu_exec_id) {
						throw std::invalid_argument("Invalid GPU executable program");
					}

					D3D12GPUExecutable* gpu_exec = plastic::utility::QueryObject<D3D12GPUExecutable>(*gpu_exec_id);
					if (!gpu_exec) {
						throw std::runtime_error("GPU executable program lost");
					}

					m_impl->SetGraphicsRootSignature(gpu_exec->GetRawAssociatedRootSignature());
					auto visitor = plastic::utility::Overload{
						[this, &state_machine, &gpu_exec](D3D12GPUExecutable::GraphicsPSO const& pso) -> GPUTask {
							co_await state_machine.WaitForRenderingStart();
							DXGI_FORMAT format = co_await state_machine.WaitForIndexType();
							switch (format) {
							case DXGI_FORMAT::DXGI_FORMAT_R16_UINT:
								m_impl->SetPipelineState(pso.impl16.Get());
								break;
							case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
								m_impl->SetPipelineState(pso.impl32.Get());
								break;
							default:
								throw std::invalid_argument("Invalid index type");
							}

							auto topo = gpu_exec->GetPrimitiveTopology();
							if (topo != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
								m_impl->IASetPrimitiveTopology(topo);
							}

							state_machine.SetGraphicsExecutable(gpu_exec);

							co_return;

						},
						[this, &state_machine, &gpu_exec](D3D12GPUExecutable::ComputePSO const& pso) -> GPUTask {
							co_await state_machine.WaitForRenderingEnd();
							m_impl->SetPipelineState(pso.impl.Get());
							state_machine.SetComputeExecutable(gpu_exec);
							co_return;
						},
						[this, &state_machine, &gpu_exec](D3D12GPUExecutable::RayTracingPSO const& pso) -> GPUTask {

							co_await state_machine.WaitForRenderingEnd();
							m_impl->SetPipelineState1(pso.impl.Get());

							auto topo = gpu_exec->GetPrimitiveTopology();
							if (topo != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED) {
								m_impl->IASetPrimitiveTopology(topo);
							}

							state_machine.SetRayTracingExecutable(gpu_exec);

							co_return;

						}
					};

					return gpu_exec->VisitPSO(visitor);

				},
				[this, &state_machine](CmdDispatchComputingTask const* dispatch_computing_task) -> GPUTask {
					co_await state_machine.WaitForContext(ContextType::Compute);
					m_impl->Dispatch(
						dispatch_computing_task->threads_x, 
						dispatch_computing_task->threads_y, 
						dispatch_computing_task->threads_z
					);
					co_return;
				},
				[this, &state_machine](CmdBindVertexBuffers const* bind_vertex_buffers) -> GPUTask {

					co_await state_machine.WaitForContext(ContextType::Graphics);

					std::vector<D3D12_RESOURCE_BARRIER> barriers;
					std::map<UINT, D3D12_VERTEX_BUFFER_VIEW> slot_views;

					for (auto const& binding : bind_vertex_buffers->bindings) {
						
						auto const& id = binding.id;
						if (!id) {
							throw std::invalid_argument("Invalid vertex buffer");
						}

						D3D12Resource* vertex_buffer = plastic::utility::QueryObject<D3D12Resource>(*id);
						if (!vertex_buffer) {
							throw std::runtime_error("Vertex buffer lost");
						}

						AddBarrier(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_buffer, barriers);

						ID3D12Resource2* resource = vertex_buffer->GetRawNative();
						D3D12_RESOURCE_DESC desc = resource->GetDesc();

						slot_views[binding.where] = D3D12_VERTEX_BUFFER_VIEW{
							.BufferLocation = resource->GetGPUVirtualAddress() + binding.offset,
							.SizeInBytes = static_cast<UINT>(desc.Width - binding.offset),
							.StrideInBytes = static_cast<UINT>(binding.stride)
						};

					}

					if (!slot_views.empty()) {

						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

						std::vector<D3D12_VERTEX_BUFFER_VIEW> views;

						auto const first = slot_views.begin()->first;
						auto const last = slot_views.rbegin()->first;
						std::size_t size = last - first + 1;
						std::ranges::generate_n(
							std::back_inserter(views),
							size,
							[slot = first, &slot_views]() mutable {
								auto it = slot_views.find(slot++);
								return it != slot_views.end() ? it->second : D3D12_VERTEX_BUFFER_VIEW{};
							}
						);

						m_impl->IASetVertexBuffers(first, static_cast<UINT>(views.size()), views.data());

					}

				},
				[this, &state_machine](CmdBindIndexBuffer const* bind_index_buffer) -> GPUTask {

					co_await state_machine.WaitForRenderingStart();

					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					auto const& id = bind_index_buffer->id;
					if (!id) {
						throw std::invalid_argument("Invalid index buffer");
					}

					D3D12Resource* index_buffer = plastic::utility::QueryObject<D3D12Resource>(*id);
					if (!index_buffer) {
						throw std::runtime_error("Index buffer lost");
					}

					AddBarrier(D3D12_RESOURCE_STATE_INDEX_BUFFER, index_buffer, barriers);

					ID3D12Resource2* resource = index_buffer->GetRawNative();
					D3D12_RESOURCE_DESC desc = resource->GetDesc();

					D3D12_INDEX_BUFFER_VIEW view{
						resource->GetGPUVirtualAddress(),
						static_cast<UINT>(desc.Width),
						desc.Format
					};

					state_machine.SetIndexType(desc.Format);

					m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					m_impl->IASetIndexBuffer(&view);

				},
				[this, &state_machine](CmdDrawInstanced const* draw_instanced) -> GPUTask {
					co_await state_machine.WaitForContext(ContextType::Graphics);
					m_impl->DrawInstanced(draw_instanced->vertices, draw_instanced->instances, draw_instanced->start_vertex, draw_instanced->first_instance);
				},
				[this, &state_machine](CmdDrawIndexedInstanced const* draw_indexed_instanced) -> GPUTask {
					co_await state_machine.WaitForContext(ContextType::Graphics);
					m_impl->DrawIndexedInstanced(draw_indexed_instanced->indices, draw_indexed_instanced->instances, draw_indexed_instanced->first_index, draw_indexed_instanced->start_vertex, draw_indexed_instanced->first_instance);
				},
				[this, &state_machine](CmdBindBuffer const* bind_buffer) -> GPUTask {

					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					ContextType type = GetContextType(bind_buffer->stages);
					D3D12GPUExecutable* exec = co_await state_machine.WaitForExecutable(type);

					auto const& id = bind_buffer->id;
					if (!id) {
						throw std::invalid_argument("Invalid buffer");
					}

					D3D12Resource* buffer = plastic::utility::QueryObject<D3D12Resource>(*id);
					if (!buffer) {
						throw std::runtime_error("Buffer lost");
					}

					std::optional<std::size_t> shader_data_segment_id = exec->GetAssociatedShaderDataSegmentID();
					if (!shader_data_segment_id) {
						throw std::invalid_argument("Invalid shader data segment");
					}
					auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
					if (!shader_data_segment) {
						throw std::runtime_error("Shader data segment lost");
					}

					auto reflector = shader_data_segment->GetReflector();
					if (!reflector) {
						throw std::logic_error("Shader data segment is not instantiated");
					}

					auto& view_id = bind_buffer->texel_view_id;
					D3D12View* texel_view = nullptr;
					if (view_id) {
						texel_view = plastic::utility::QueryObject<D3D12View>(*view_id);
						if (!texel_view) {
							throw std::runtime_error("View lost");
						}
					}

					auto Barrier = [&](D3D12_RESOURCE_STATES dest) mutable {
						SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;
						auto original = buffer->GetState();
						if (dest != original) {
							barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(buffer->GetRawNative(), original, dest));
							m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
							buffer->SetState(dest);
						}
						};
				
					ID3D12Resource2* native = buffer->GetRawNative();
					D3D12_GPU_VIRTUAL_ADDRESS addr = native->GetGPUVirtualAddress() + bind_buffer->offset;
				
					bool graphics_rt = type == ContextType::Graphics || type == ContextType::RayTracing;
					bool compute = type == ContextType::Compute;
				
					switch (buffer->GetViewType()) {
					case D3D12Resource::ViewType::CBV:
						if (auto idx = reflector->IndexOf(bind_buffer->ns, bind_buffer->where, RootSignatureReflector::Type::CBV)) {
							Barrier(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
							if (texel_view) {
								throw std::logic_error("Constant buffer cannot be texel buffer");
							}
							if (graphics_rt) {
								m_impl->SetGraphicsRootConstantBufferView(static_cast<UINT>(*idx), addr);
								co_return;
							}
							if (compute) {
								m_impl->SetComputeRootConstantBufferView(static_cast<UINT>(*idx), addr);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					case D3D12Resource::ViewType::UAV:
						if (auto idx = reflector->IndexOf(bind_buffer->ns, bind_buffer->where, RootSignatureReflector::Type::UAV)) {
							Barrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
							if (graphics_rt) {
								if (texel_view) m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), texel_view->GetNative().second);
								else m_impl->SetGraphicsRootUnorderedAccessView(static_cast<UINT>(*idx), addr);
								co_return;
							} 
							if (compute) {
								if (texel_view) m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), texel_view->GetNative().second);
								else m_impl->SetComputeRootUnorderedAccessView(static_cast<UINT>(*idx), addr);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					case D3D12Resource::ViewType::SRV:
						if (auto idx = reflector->IndexOf(bind_buffer->ns, bind_buffer->where, RootSignatureReflector::Type::SRV)) {
							Barrier((bind_buffer->stages & ShaderStage::Pixel) != ShaderStage::Unknown ? 
								D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : 
								D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
							);
							if (graphics_rt) {
								if (texel_view) m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), texel_view->GetNative().second);
								else m_impl->SetGraphicsRootShaderResourceView(static_cast<UINT>(*idx), addr);
								co_return;
							} 
							if (compute) {
								if (texel_view) m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), texel_view->GetNative().second);
								else m_impl->SetComputeRootShaderResourceView(static_cast<UINT>(*idx), addr);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					default:
						throw std::logic_error("Incompatible buffer");
					}

					throw std::logic_error("Binding variable mismatch");

				},
				[this, &state_machine](CmdPushData const* push_data) -> GPUTask {
	
					auto& bytes = push_data->bytes;
					std::size_t push_size = bytes.size() / 4u;

					if (push_size % 4u != 0) {
						throw std::logic_error("Push data must be 4-byte aligned");
					}

					ContextType type = GetContextType(push_data->stages);
					D3D12GPUExecutable* exec = co_await state_machine.WaitForExecutable(type);

					std::optional<std::size_t> shader_data_segment_id = exec->GetAssociatedShaderDataSegmentID();
					if (!shader_data_segment_id) {
						throw std::invalid_argument("Invalid shader data segment");
					}
					auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
					if (!shader_data_segment) {
						throw std::runtime_error("Shader data segment lost");
					}

					auto reflector = shader_data_segment->GetReflector();
					if (!reflector) {
						throw std::runtime_error("Shader data segment is not instantiated");
					}

					bool graphics_rt = type == ContextType::Graphics || type == ContextType::RayTracing;
					bool compute = type == ContextType::Compute;

					auto idx = reflector->IndexOf(push_data->ns, push_data->where, RootSignatureReflector::Type::RootConstant);
					if (!idx) {
						throw std::logic_error("Binding variable mismatch");
					}

					std::size_t offset = push_data->offset / 4u;

					if (graphics_rt) {
						m_impl->SetGraphicsRoot32BitConstants(static_cast<UINT>(*idx), push_size, bytes.data(), static_cast<UINT>(offset));
					}

					if (compute) {
						m_impl->SetComputeRoot32BitConstants(static_cast<UINT>(*idx), push_size, bytes.data(), static_cast<UINT>(offset));
					}

					co_return;

				},
				[this, &state_machine](CmdBindSampler const* bind_sampler) -> GPUTask {

					ContextType type = GetContextType(bind_sampler->stages);
					D3D12GPUExecutable* exec = co_await state_machine.WaitForExecutable(type);

					std::optional<std::size_t> shader_data_segment_id = exec->GetAssociatedShaderDataSegmentID();
					if (!shader_data_segment_id) {
						throw std::invalid_argument("Invalid shader data segment");
					}
					auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
					if (!shader_data_segment) {
						throw std::runtime_error("Shader data segment lost");
					}

					auto reflector = shader_data_segment->GetReflector();
					if (!reflector) {
						throw std::runtime_error("Shader data segment is not instantiated");
					}

					bool graphics_rt = type == ContextType::Graphics || type == ContextType::RayTracing;
					bool compute = type == ContextType::Compute;

					auto idx = reflector->IndexOf(bind_sampler->ns, bind_sampler->where, RootSignatureReflector::Type::Sampler);
					if (!idx) {
						throw std::logic_error("Binding variable mismatch");
					}

					auto const& id = bind_sampler->id;
					if (!id) {
						throw std::invalid_argument("Invalid sampler");
					}

					D3D12Sampler* sampler = plastic::utility::QueryObject<D3D12Sampler>(*id);
					if (!sampler) {
						throw std::runtime_error("Sampler lost");
					}

					if (graphics_rt) {
						m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), sampler->GetNative().second);
					}

					if (compute) {
						m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), sampler->GetNative().second);
					}	

					co_return;

				},
				[this, &state_machine](CmdBindTexture const* bind_texture) -> GPUTask {
					
					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					ContextType type = GetContextType(bind_texture->stages);
					D3D12GPUExecutable* exec = co_await state_machine.WaitForExecutable(type);

					auto& texture_id = bind_texture->texture_id;
					if (!texture_id) {
						throw std::invalid_argument("Invalid texture");
					}

					D3D12Resource* texture = plastic::utility::QueryObject<D3D12Resource>(*texture_id);
					if (!texture) {
						throw std::runtime_error("Texture lost");
					}
					
					auto& view_id = bind_texture->view_id;
					if (!view_id) {
						throw std::invalid_argument("Invalid view");
					}

					D3D12View* view = plastic::utility::QueryObject<D3D12View>(*view_id);
					if (!view) {
						throw std::runtime_error("View lost");
					}

					std::optional<std::size_t> shader_data_segment_id = exec->GetAssociatedShaderDataSegmentID();
					if (!shader_data_segment_id) {
						throw std::invalid_argument("Invalid shader data segment");
					}
					auto* shader_data_segment = plastic::utility::QueryObject<D3D12ShaderDataSegment>(*shader_data_segment_id);
					if (!shader_data_segment) {
						throw std::runtime_error("Shader data segment lost");
					}

					auto reflector = shader_data_segment->GetReflector();
					if (!reflector) {
						throw std::runtime_error("Shader data segment is not instantiated");
					}

					auto Barrier = [&](D3D12_RESOURCE_STATES dest) mutable {
						SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;
						auto original = texture->GetState();
						if (dest != original) {
							barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetRawNative(), original, dest));
							m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
							texture->SetState(dest);
						}
						};

					bool graphics_rt = type == ContextType::Graphics || type == ContextType::RayTracing;
					bool compute = type == ContextType::Compute;
				
					switch (texture->GetViewType()) {
					case D3D12Resource::ViewType::CBV:
						if (auto idx = reflector->IndexOf(bind_texture->ns, bind_texture->where, RootSignatureReflector::Type::CBV)) {
							Barrier(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
							if (graphics_rt) {
								m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							if (compute) {
								m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					case D3D12Resource::ViewType::UAV:
						if (auto idx = reflector->IndexOf(bind_texture->ns, bind_texture->where, RootSignatureReflector::Type::UAV)) {
							Barrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
							if (graphics_rt) {
								m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							if (compute) {
								m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					case D3D12Resource::ViewType::SRV:
						if (auto idx = reflector->IndexOf(bind_texture->ns, bind_texture->where, RootSignatureReflector::Type::SRV)) {
							Barrier((bind_texture->stages & ShaderStage::Pixel) != ShaderStage::Unknown ? 
								D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : 
								D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
							);
							if (graphics_rt) {
								m_impl->SetGraphicsRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							if (compute) {
								m_impl->SetComputeRootDescriptorTable(static_cast<UINT>(*idx), view->GetNative().second);
								co_return;
							}
							std::unreachable();
						}
						break;
				
					default:
						throw std::logic_error("Incompatible texture");
					}

					throw std::logic_error("Binding variable mismatch");
					
				},
				[this, &state_machine](CmdCopyTextureToBuffer const* copy_cmd) -> GPUTask {

					co_await state_machine.WaitForRenderingEnd();

					SmallVector<D3D12_RESOURCE_BARRIER, 2u> barriers;

					auto src_id = copy_cmd->src_id;
					auto dest_id = copy_cmd->dest_id;
					if (!src_id || !dest_id) {
						throw std::invalid_argument("Invalid copy source/destination");
					}

					D3D12Resource* src_texture = plastic::utility::QueryObject<D3D12Resource>(*src_id);
					D3D12Resource* dest_buffer = plastic::utility::QueryObject<D3D12Resource>(*dest_id);
					if (!src_texture || !dest_buffer) {
						throw std::runtime_error("Copy source or destination resource lost");
					}

					// Transition source texture to COPY_SOURCE
					AddBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, src_texture, barriers);

					// Transition destination buffer to COPY_DEST (was incorrectly using src_texture)
					AddBarrier(D3D12_RESOURCE_STATE_COPY_DEST, dest_buffer, barriers);

					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}

					ID3D12Resource2* src_res = src_texture->GetRawNative();
					ID3D12Resource2* dest_res = dest_buffer->GetRawNative();
				
					D3D12_RESOURCE_DESC const src_desc = src_res->GetDesc();
					if (src_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
						src_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
						throw std::runtime_error("Source resource is not a texture");
					}
				
					std::uint32_t const bytes_per_pixel = GetBytesPerPixel(src_desc.Format);
					std::uint32_t const mip_levels = src_desc.MipLevels;
					std::uint32_t const array_start = static_cast<std::uint32_t>(copy_cmd->src_subresource.array_layer);
					std::uint32_t const layer_count = static_cast<std::uint32_t>(copy_cmd->src_subresource.layer_count);
				
					std::uint64_t const row_pitch = AlignUp(
						static_cast<std::uint64_t>(copy_cmd->extent.width) * bytes_per_pixel,
						D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT
					);

					std::uint64_t const slice_pitch = row_pitch * copy_cmd->extent.height;
					std::uint64_t const layer_size = slice_pitch * copy_cmd->extent.depth;
				
					std::uint64_t dest_offset = copy_cmd->dest_offset;
				
					for (std::uint32_t layer = 0; layer < layer_count; ++layer) {

						std::uint32_t const subresource = static_cast<std::uint32_t>(copy_cmd->src_subresource.mip_level) + (array_start + layer) * mip_levels;
				
						D3D12_TEXTURE_COPY_LOCATION const src_loc = {
							.pResource = src_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
							.SubresourceIndex = subresource
						};
				
						D3D12_TEXTURE_COPY_LOCATION const dest_loc = {
							.pResource = dest_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
							.PlacedFootprint = {
								.Offset = dest_offset,
								.Footprint = {
									.Format = src_desc.Format,
									.Width = static_cast<UINT>(copy_cmd->extent.width),
									.Height = static_cast<UINT>(copy_cmd->extent.height),
									.Depth = static_cast<UINT>(copy_cmd->extent.depth),
									.RowPitch = static_cast<UINT>(row_pitch)
								}
							}
						};
				
						D3D12_BOX const src_box = {
							.left = static_cast<UINT>(copy_cmd->src_offset.x),
							.top = static_cast<UINT>(copy_cmd->src_offset.y),
							.front = static_cast<UINT>(copy_cmd->src_offset.z),
							.right = static_cast<UINT>(copy_cmd->src_offset.x + copy_cmd->extent.width),
							.bottom = static_cast<UINT>(copy_cmd->src_offset.y + copy_cmd->extent.height),
							.back = static_cast<UINT>(copy_cmd->src_offset.z + copy_cmd->extent.depth)
						};
				
						m_impl->CopyTextureRegion(&dest_loc, 0, 0, 0, &src_loc, &src_box);
				
						dest_offset += layer_size;
					}
				
					co_return;

				},
				[this, &state_machine](CmdCopyBufferToTexture const* copy_cmd) -> GPUTask {

					co_await state_machine.WaitForRenderingEnd();

					SmallVector<D3D12_RESOURCE_BARRIER, 2u> barriers;

					auto src_id = copy_cmd->src_id;
					auto dest_id = copy_cmd->dest_id;
					if (!src_id || !dest_id) {
						throw std::invalid_argument("Invalid copy source/destination");
					}
				
					D3D12Resource* src_buffer = plastic::utility::QueryObject<D3D12Resource>(*src_id);
					D3D12Resource* dest_texture = plastic::utility::QueryObject<D3D12Resource>(*dest_id);
					if (!src_buffer || !dest_texture) {
						throw std::runtime_error("Copy source or destination resource lost");
					}
				
					// Transition source buffer to COPY_SOURCE
					AddBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, src_buffer, barriers);
				
					// Transition destination texture to COPY_DEST
					AddBarrier(D3D12_RESOURCE_STATE_COPY_DEST, dest_texture, barriers);
				
					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}
				
					ID3D12Resource2* src_res = src_buffer->GetRawNative();
					ID3D12Resource2* dest_res = dest_texture->GetRawNative();
				
					D3D12_RESOURCE_DESC const dest_desc = dest_res->GetDesc();
					if (dest_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
						dest_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
						throw std::runtime_error("Destination resource is not a texture");
					}
				
					std::uint32_t const bytes_per_pixel = GetBytesPerPixel(dest_desc.Format);
					std::uint32_t const mip_levels = dest_desc.MipLevels;
					std::uint32_t const array_start = static_cast<std::uint32_t>(copy_cmd->dest_subresource.array_layer);
					std::uint32_t const layer_count = static_cast<std::uint32_t>(copy_cmd->dest_subresource.layer_count);
				
					std::uint64_t const row_pitch = AlignUp(
						static_cast<std::uint64_t>(copy_cmd->extent.width) * bytes_per_pixel,
						D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT
					);
					std::uint64_t const slice_pitch = row_pitch * copy_cmd->extent.height;
					std::uint64_t const layer_size = slice_pitch * copy_cmd->extent.depth;
				
					std::uint64_t src_offset = copy_cmd->src_offset;
				
					for (std::uint32_t layer = 0; layer < layer_count; ++layer) {
						std::uint32_t const subresource = static_cast<std::uint32_t>(copy_cmd->dest_subresource.mip_level) + (array_start + layer) * mip_levels;
				
						D3D12_TEXTURE_COPY_LOCATION const src_loc = {
							.pResource = src_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
							.PlacedFootprint = {
								.Offset = src_offset,
								.Footprint = {
									.Format = dest_desc.Format,
									.Width = static_cast<UINT>(copy_cmd->extent.width),
									.Height = static_cast<UINT>(copy_cmd->extent.height),
									.Depth = static_cast<UINT>(copy_cmd->extent.depth),
									.RowPitch = static_cast<UINT>(row_pitch)
								}
							}
						};
				
						D3D12_TEXTURE_COPY_LOCATION const dest_loc = {
							.pResource = dest_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
							.SubresourceIndex = subresource
						};
				
						m_impl->CopyTextureRegion(
							&dest_loc,
							static_cast<UINT>(copy_cmd->dest_offset.x),
							static_cast<UINT>(copy_cmd->dest_offset.y),
							static_cast<UINT>(copy_cmd->dest_offset.z),
							&src_loc, nullptr
						);
				
						src_offset += layer_size;
					}
				
					co_return;

				},
				[this, &state_machine](CmdCopyBufferToBuffer const* copy_cmd) -> GPUTask {
					
					co_await state_machine.WaitForRenderingEnd();

					SmallVector<D3D12_RESOURCE_BARRIER, 2u> barriers;
				
					auto src_id = copy_cmd->src_id;
					auto dest_id = copy_cmd->dest_id;
					if (!src_id || !dest_id) {
						throw std::invalid_argument("Invalid copy source/destination");
					}
				
					D3D12Resource* src_buffer = plastic::utility::QueryObject<D3D12Resource>(*src_id);
					D3D12Resource* dest_buffer = plastic::utility::QueryObject<D3D12Resource>(*dest_id);
					if (!src_buffer || !dest_buffer) {
						throw std::runtime_error("Copy source or destination resource lost");
					}
				
					// Transition source buffer to COPY_SOURCE
					AddBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, src_buffer, barriers);
				
					// Transition destination buffer to COPY_DEST
					AddBarrier(D3D12_RESOURCE_STATE_COPY_DEST, dest_buffer, barriers);
				
					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}

					m_impl->CopyBufferRegion(
						dest_buffer->GetRawNative(), copy_cmd->dest_offset,
						src_buffer->GetRawNative(), copy_cmd->src_offset,
						copy_cmd->size
					);
				
					co_return;
				},
				[this, &state_machine](CmdCopyTextureToTexture const* copy_cmd) -> GPUTask {
					
					co_await state_machine.WaitForRenderingEnd();

					SmallVector<D3D12_RESOURCE_BARRIER, 2u> barriers;
				
					auto src_id = copy_cmd->src_id;
					auto dest_id = copy_cmd->dest_id;
					if (!src_id || !dest_id) {
						throw std::invalid_argument("Invalid copy source/destination");
					}
				
					D3D12Resource* src_texture = plastic::utility::QueryObject<D3D12Resource>(*src_id);
					D3D12Resource* dest_texture = plastic::utility::QueryObject<D3D12Resource>(*dest_id);
					if (!src_texture || !dest_texture) {
						throw std::runtime_error("Copy source or destination resource lost");
					}
				
					// Transition source texture to COPY_SOURCE
					AddBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, src_texture, barriers);
				
					// Transition destination texture to COPY_DEST
					AddBarrier(D3D12_RESOURCE_STATE_COPY_DEST, dest_texture, barriers);
				
					if (!barriers.empty()) {
						m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
					}

					ID3D12Resource* src_res = src_texture->GetRawNative();
					ID3D12Resource* dest_res = dest_texture->GetRawNative();
				
					D3D12_RESOURCE_DESC const src_desc = src_res->GetDesc();
					D3D12_RESOURCE_DESC const dest_desc = dest_res->GetDesc();
				
					std::uint32_t const src_mip_levels = src_desc.MipLevels;
					std::uint32_t const dest_mip_levels = dest_desc.MipLevels;
				
					std::uint32_t const src_array_start = static_cast<std::uint32_t>(copy_cmd->src_subresource.array_layer);
					std::uint32_t const dest_array_start = static_cast<std::uint32_t>(copy_cmd->dest_subresource.array_layer);
					std::uint32_t const layer_count = static_cast<std::uint32_t>(copy_cmd->src_subresource.layer_count);
				
					for (std::uint32_t layer = 0; layer < layer_count; ++layer) {
						std::uint32_t const src_subresource = static_cast<std::uint32_t>(copy_cmd->src_subresource.mip_level) + (src_array_start + layer) * src_mip_levels;
						std::uint32_t const dest_subresource = static_cast<std::uint32_t>(copy_cmd->dest_subresource.mip_level) + (dest_array_start + layer) * dest_mip_levels;
				
						D3D12_TEXTURE_COPY_LOCATION const src_loc = {
							.pResource = src_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
							.SubresourceIndex = src_subresource
						};
				
						D3D12_TEXTURE_COPY_LOCATION const dest_loc = {
							.pResource = dest_res,
							.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
							.SubresourceIndex = dest_subresource
						};
				
						D3D12_BOX const src_box = {
							.left = static_cast<UINT>(copy_cmd->src_offset.x),
							.top = static_cast<UINT>(copy_cmd->src_offset.y),
							.front = static_cast<UINT>(copy_cmd->src_offset.z),
							.right = static_cast<UINT>(copy_cmd->src_offset.x + copy_cmd->extent.width),
							.bottom = static_cast<UINT>(copy_cmd->src_offset.y + copy_cmd->extent.height),
							.back = static_cast<UINT>(copy_cmd->src_offset.z + copy_cmd->extent.depth)
						};
				
						m_impl->CopyTextureRegion(
							&dest_loc,
							static_cast<UINT>(copy_cmd->dest_offset.x),
							static_cast<UINT>(copy_cmd->dest_offset.y),
							static_cast<UINT>(copy_cmd->dest_offset.z),
							&src_loc, &src_box
						);
					}
				
					co_return;

				},
				[this, &state_machine](CmdExecuteIndirect const* execute_indirect) -> GPUTask {
					
					co_await state_machine.WaitForContext(ContextType::Graphics);

					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					if (!m_logical_device_id) {
						throw std::invalid_argument("Invalid logical device");
					}

					D3D12LogicalDevice* logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id);
					if (!logical_device) {
						throw std::runtime_error("Logical device lost");
					}

					auto& buffer_id = execute_indirect->id;
					if (!buffer_id) {
						throw std::invalid_argument("Invalid buffer");
					}

					D3D12Resource* buffer = plastic::utility::QueryObject<D3D12Resource>(*buffer_id);
					if (!buffer) {
						throw std::runtime_error("Buffer lost");
					}

					AddBarrier(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, buffer, barriers);
					m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

					auto sig = logical_device->GetMultiDrawSignature();
					m_impl->ExecuteIndirect(
						sig.Get(),
						execute_indirect->draws,
						buffer->GetRawNative(),
						execute_indirect->offset_into_buffer,
						nullptr,
						0
					);

				},
				[this, &state_machine](CmdExecuteIndirectIndexed const* execute_indirect_indexed) -> GPUTask {
					
					co_await state_machine.WaitForContext(ContextType::Graphics);
					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					if (!m_logical_device_id) {
						throw std::invalid_argument("Invalid logical device");
					}

					D3D12LogicalDevice* logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id);
					if (!logical_device) {
						throw std::runtime_error("Logical device lost");
					}

					auto& buffer_id = execute_indirect_indexed->id;
					if (!buffer_id) {
						throw std::invalid_argument("Invalid buffer");
					}

					D3D12Resource* buffer = plastic::utility::QueryObject<D3D12Resource>(*buffer_id);
					if (!buffer) {
						throw std::runtime_error("Buffer lost");
					}

					AddBarrier(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, buffer, barriers);
					m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

					auto sig = logical_device->GetMultiDrawIndexedSignature();
					m_impl->ExecuteIndirect(
						sig.Get(),
						execute_indirect_indexed->draws,
						buffer->GetRawNative(),
						execute_indirect_indexed->offset_into_buffer,
						nullptr,
						0
					);

				},
				[this, &state_machine](CmdDispatchIndirect const* dispatch_indirect) -> GPUTask {

					co_await state_machine.WaitForContext(ContextType::Compute);

					SmallVector<D3D12_RESOURCE_BARRIER, 1u> barriers;

					if (!m_logical_device_id) {
						throw std::invalid_argument("Invalid logical device");
					}

					D3D12LogicalDevice* logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id);
					if (!logical_device) {
						throw std::runtime_error("Logical device lost");
					}

					auto& buffer_id = dispatch_indirect->id;
					if (!buffer_id) {
						throw std::invalid_argument("Invalid buffer");
					}

					D3D12Resource* buffer = plastic::utility::QueryObject<D3D12Resource>(*buffer_id);
					if (!buffer) {
						throw std::runtime_error("Buffer lost");
					}

					AddBarrier(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, buffer, barriers);
					m_impl->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

					auto sig = logical_device->GetDispatchIndirectSignature();
					m_impl->ExecuteIndirect(
						sig.Get(),
						1u,
						buffer->GetRawNative(),
						dispatch_indirect->offset_into_buffer,
						nullptr,
						0
					);

				},
				[](auto const*) -> GPUTask {
					co_return;
				}
			};

			for (auto const& cmd : m_commands) {
				cmd->Visit(visitor);
			}

			state_machine.Finalize();
			ThrowIfFailed(m_impl->Close());

		}

		m_sync.Release();

	}

	void D3D12CommandBuffer::Reset() {
		m_sync.ThrowIfNotOwned();
		m_commands.clear();
	}

	D3D12CommandBuffer& D3D12CommandBuffer::SetFrame(D3D12Resource const& frame, D3D12View const& frame_view) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetFrame>();
		command->texture_id = frame.GetID();
		command->view_id = frame_view.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BeginRendering(std::span<D3D12Resource const* const> rts, std::span<D3D12View const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBeginRendering>();

		std::size_t count = (std::min)({ rts.size(), rtvs.size(), rt_loads.size(), rt_stores.size(), rt_clears.size() });

		for (std::size_t i = 0; i < count; ++i) {
			command->render_targets.emplace_back(
				rts[i]->GetID(),	// texture_id
				rtvs[i]->GetID(),	// view_id
				rt_clears[i],		// rt_clear
				rt_loads[i],		// rt_load
				rt_stores[i]		// rt_store
			);
		}
	
		m_commands.emplace_back(std::move(command));
		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BeginRendering(std::span<D3D12Resource const* const> rts, std::span<D3D12View const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears, D3D12Resource const& ds, D3D12View const& dsv, LoadOp depth_load, StoreOp depth_store, float depth_clear, LoadOp stencil_load, StoreOp stencil_store, std::uint8_t stencil_clear) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBeginRendering>();

		std::size_t count = (std::min)({ rts.size(), rt_loads.size(), rt_stores.size(), rt_clears.size() });

		for (std::size_t i = 0; i < count; ++i) {
			command->render_targets.emplace_back(
				rts[i]->GetID(),	// texture_id
				rtvs[i]->GetID(),	// view_id
				rt_clears[i],		// rt_clear
				rt_loads[i],		// rt_load
				rt_stores[i]		// rt_store
			);
		}
		command->depth_stencil.emplace(
			ds.GetID(),				// texture_id
			dsv.GetID(),			// view_id
			depth_clear,			// depth_clear
			stencil_clear,			// stencil_clear
			depth_load,				// d_load
			depth_store,			// d_store
			stencil_load,			// s_load
			stencil_store			// s_store
		);
	
		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::EndRendering() {
		m_sync.ThrowIfNotOwned();
		m_commands.emplace_back(plastic::utility::MakeUnique<CmdEndRendering>());
		return *this;
	}

	
	D3D12CommandBuffer& D3D12CommandBuffer::SetViewports(std::span<CmdSetViewports::Viewport const> viewports) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetViewports>();

		command->viewports.assign(viewports.begin(), viewports.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::SetScissors(std::span<CmdSetScissors::Scissor const> scissors) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetScissors>();

		command->scissors.assign(scissors.begin(), scissors.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::Execute(D3D12GPUExecutable const& gpu_exec) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdExecute>();

		command->gpu_exec_id = gpu_exec.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::DispatchComputingTask(std::size_t threads_x, std::size_t threads_y, std::size_t threads_z, std::size_t threads_per_thread_group_x, std::size_t threads_per_thread_group_y, std::size_t threads_per_thread_group_z) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDispatchComputingTask>();

		command->threads_x = threads_x;
		command->threads_y = threads_y;
		command->threads_z = threads_z;

		command->threads_per_thread_group_x = threads_per_thread_group_x;
		command->threads_per_thread_group_y = threads_per_thread_group_y;
		command->threads_per_thread_group_z = threads_per_thread_group_z;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	
	D3D12CommandBuffer& D3D12CommandBuffer::BindVertexBuffers(std::span<D3D12Resource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindVertexBuffers>();
		
		std::size_t binding_count = (std::min)({ vertex_buffers.size(), wheres.size(), offsets.size(), strides.size() });
		for (std::size_t i = 0; i < binding_count; ++i) {
			command->bindings.emplace_back(vertex_buffers[i]->GetID(), wheres[i], offsets[i], strides[i]);
		}

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::DrawInstanced(std::size_t vertices, std::size_t instances, std::size_t start_vertex, std::size_t first_instance) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDrawInstanced>();
	
		command->vertices = vertices;
		command->instances = instances;
		command->start_vertex = start_vertex;
		command->first_instance = first_instance;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::DrawIndexed(std::size_t indices, std::size_t instances, std::size_t first_index, std::size_t start_vertex, std::size_t first_instance) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDrawIndexedInstanced>();
	
		command->indices = indices;
		command->instances = instances;
		command->first_index = first_index;
		command->start_vertex = start_vertex;
		command->first_instance = first_instance;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BindBuffer(
		ShaderStage stages,
		std::size_t where,
		D3D12Resource const& buffer,
		std::size_t offset,
		std::size_t ns
	) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindBuffer>();
		
		command->id = buffer.GetID();
		command->offset = offset;
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BindBuffer(ShaderStage stages, std::size_t where, D3D12Resource const& buffer, D3D12View const& texel_view, std::size_t offset, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindBuffer>();
		
		command->id = buffer.GetID();
		command->texel_view_id = texel_view.GetID();
		command->offset = offset;
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::PushData(ShaderStage stages, std::size_t where, std::span<std::byte const> bytes, std::size_t offset, std::size_t ns) {

		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdPushData>();

		command->where = where;
		command->ns = ns;
		command->offset = offset;
		command->bytes.assign(bytes.begin(), bytes.end());
		command->stages = stages;

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BindSampler(ShaderStage stages, std::size_t where, D3D12Sampler const& sampler, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindSampler>();

		command->id = sampler.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::BindTexture(ShaderStage stages, std::size_t where, D3D12Resource const& texture, D3D12View const& view, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindTexture>();

		command->texture_id = texture.GetID();
		command->view_id = view.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	D3D12CommandBuffer& D3D12CommandBuffer::CopyTextureToBuffer(
		D3D12Resource const& src_texture,
		D3D12Resource const& dest_buffer,
		CmdCopyTextureToBuffer::Subresource const& src_subresource,
		CmdCopyTextureToBuffer::Offset3D const& src_offset,
		std::size_t dest_offset,
		CmdCopyTextureToBuffer::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyTextureToBuffer>();
		command->src_id = src_texture.GetID();
		command->dest_id = dest_buffer.GetID();
		command->src_subresource = src_subresource;
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	D3D12CommandBuffer& D3D12CommandBuffer::CopyBufferToTexture(
		D3D12Resource const& src_buffer,
		D3D12Resource const& dest_texture,
		std::size_t src_offset,
		CmdCopyBufferToTexture::Subresource const& dest_subresource,
		CmdCopyBufferToTexture::Offset3D const& dest_offset,
		CmdCopyBufferToTexture::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyBufferToTexture>();
		command->src_id = src_buffer.GetID();
		command->dest_id = dest_texture.GetID();
		command->src_offset = src_offset;
		command->dest_subresource = dest_subresource;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	D3D12CommandBuffer& D3D12CommandBuffer::CopyBufferToBuffer(
		D3D12Resource const& src_buffer,
		D3D12Resource const& dest_buffer,
		std::size_t src_offset,
		std::size_t dest_offset,
		std::size_t size
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyBufferToBuffer>();
		command->src_id = src_buffer.GetID();
		command->dest_id = dest_buffer.GetID();
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->size = size;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	D3D12CommandBuffer& D3D12CommandBuffer::CopyTextureToTexture(
		D3D12Resource const& src_texture,
		D3D12Resource const& dest_texture,
		CmdCopyTextureToTexture::Subresource const& src_subresource,
		CmdCopyTextureToTexture::Subresource const& dest_subresource,
		CmdCopyTextureToTexture::Offset3D const& src_offset,
		CmdCopyTextureToTexture::Offset3D const& dest_offset,
		CmdCopyTextureToTexture::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyTextureToTexture>();
		command->src_id = src_texture.GetID();
		command->dest_id = dest_texture.GetID();
		command->src_subresource = src_subresource;
		command->dest_subresource = dest_subresource;
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}

	D3D12CommandBuffer& D3D12CommandBuffer::BindIndexBuffer(
		D3D12Resource const& buffer
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdBindIndexBuffer>();
		cmd->id = buffer.GetID();
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	D3D12CommandBuffer& D3D12CommandBuffer::ExecuteIndirect(
		D3D12Resource const& arg_buffer,
		std::size_t offset,
		std::size_t draw_count
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdExecuteIndirect>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->draws = draw_count;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	D3D12CommandBuffer& D3D12CommandBuffer::ExecuteIndirectIndexed(
		D3D12Resource const& arg_buffer,
		std::size_t offset,
		std::size_t draw_count
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdExecuteIndirectIndexed>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->draws = draw_count;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	D3D12CommandBuffer& D3D12CommandBuffer::DispatchIndirect(
		D3D12Resource const& arg_buffer,
		std::size_t offset,
		std::size_t block_x, std::size_t block_y, std::size_t block_z
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdDispatchIndirect>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->block_size_x = block_x;
		cmd->block_size_y = block_y;
		cmd->block_size_z = block_z;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}


	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> D3D12CommandBuffer::GetNative() const noexcept {
		return m_impl;
	}

	ID3D12GraphicsCommandList4* D3D12CommandBuffer::GetRawNative() const noexcept {
		return m_impl.Get();
	}
}

#endif // defined(_WIN32)