/* vulkan_command_buffer.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cmath>
#include <functional>
#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <span>
#include <coroutine>
#include <format>
#endif // !defined(__cpp_lib_modules)
#include "declare_pool.hpp"
module fyuu_rhi:vulkan_command_buffer_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_command_buffer;
import plastic.registrable;
import plastic.serial_task;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_queue_allocator;
import :vulkan_logical_device;
import :vulkan_shader_data_segment;
import :vulkan_gpu_executable;
import :vulkan_sampler;
import :vulkan_resource;
import :vulkan_view;
import :synchronization;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	vk::AttachmentLoadOp ToVulkanLoadOp(LoadOp op) noexcept {
		switch (op) {
		case LoadOp::Load:		return vk::AttachmentLoadOp::eLoad;
		case LoadOp::Clear:		return vk::AttachmentLoadOp::eClear;
		case LoadOp::DontCare:	return vk::AttachmentLoadOp::eDontCare;
		default:				return vk::AttachmentLoadOp::eDontCare;
		}
	}
	
	vk::AttachmentStoreOp ToVulkanStoreOp(StoreOp op) noexcept {
		switch (op) {
		case StoreOp::Store:	return vk::AttachmentStoreOp::eStore;
		case StoreOp::DontCare:	return vk::AttachmentStoreOp::eDontCare;
		default:				return vk::AttachmentStoreOp::eDontCare;
		}
	}

	std::size_t GetLayerCount(
		std::span<VulkanResource const* const> render_targets,
		VulkanResource const* depth_stencil
	) {
		std::optional<std::size_t> common_layers;
	
		// Helper to validate and record layer count.
		auto CheckLayers = [&](std::size_t layers) {
			if (!common_layers) {
				common_layers = layers;
			} 
			else {
				if (*common_layers != layers) {
					throw std::invalid_argument("Layer count mismatch among framebuffer attachments");
				}
			}
			};
	
		// Check all render targets.
		for (auto const* rt : render_targets) {
			if (!rt) continue;
			auto const* desc = rt->GetTextureDescription();
			if (desc) {
				CheckLayers(desc->arrayLayers);
			}
		}
	
		// Check depth‑stencil attachment if present.
		if (depth_stencil) {
			auto const* desc = depth_stencil->GetTextureDescription();
			if (desc) {
				CheckLayers(desc->arrayLayers);
			}
		}
	
		// If no attachments were provided, assume a default layer count of 1.
		return common_layers.value_or(1u);
	}

	vk::Extent3D GetMipExtent(const vk::Extent3D& base_extent, uint32_t mip_level) {
		return vk::Extent3D{
			std::max(1u, base_extent.width >> mip_level),
			std::max(1u, base_extent.height >> mip_level),
			std::max(1u, base_extent.depth >> mip_level)
		};
	}

	std::size_t EstimateAspectBufferSize(vk::Format format, vk::ImageAspectFlagBits aspect, const vk::Extent3D& extent) {
		std::uint32_t texel_count = extent.width * extent.height * extent.depth;
		switch (format) {
		case vk::Format::eD32Sfloat:
			if (aspect == vk::ImageAspectFlagBits::eDepth) return texel_count * 4;
			break;
		case vk::Format::eD32SfloatS8Uint:
			if (aspect == vk::ImageAspectFlagBits::eDepth) return texel_count * 4;
			if (aspect == vk::ImageAspectFlagBits::eStencil) return texel_count * 1;
			break;
		case vk::Format::eD24UnormS8Uint:
			if (aspect == vk::ImageAspectFlagBits::eDepth) return texel_count * 3;
			if (aspect == vk::ImageAspectFlagBits::eStencil) return texel_count * 1;
			break;
		case vk::Format::eD16Unorm:
			if (aspect == vk::ImageAspectFlagBits::eDepth) return texel_count * 2;
			break;
		default:
			throw std::runtime_error("Unsupported depth-stencil format for size estimation");
		}
	}

	void AddBarrier(
		vk::PipelineStageFlags2 dst_stage,
		vk::AccessFlags2 dst_access, 
		vk::ImageLayout dst_layout,
		VulkanResource* tex, 
		auto&& barriers
	) {
		auto [cur_stage, cur_access, cur_layout] = tex->GetState();
		if (cur_stage != dst_stage || cur_access != dst_access || cur_layout != dst_layout) {
			barriers.emplace_back(
				cur_stage, cur_access,
				dst_stage, dst_access,
				cur_layout, dst_layout,
				std::numeric_limits<std::uint32_t>::max(), 
				std::numeric_limits<std::uint32_t>::max(),
				tex->GetNative().texture,
				tex->WholeResourceRange()
			);
			tex->SetState({dst_stage, dst_access, dst_layout});
		}
	}

	void AddBarrier(
		vk::PipelineStageFlags2 dst_stage,
        vk::AccessFlags2 dst_access, 
		VulkanResource* buf, 
		auto&& barriers
	) {
		auto [cur_stage, cur_access, cur_layout] = buf->GetState();
		if (cur_stage != dst_stage || cur_access != dst_access) {
			barriers.emplace_back(
				cur_stage, cur_access,
				dst_stage, dst_access,
				std::numeric_limits<std::uint32_t>::max(), 
				std::numeric_limits<std::uint32_t>::max(),
				buf->GetNative().buffer,
				0, 
				std::numeric_limits<vk::DeviceSize>::max()
			);
			buf->SetState({dst_stage, dst_access, cur_layout});
		}		
	}

	vk::PipelineBindPoint GetBindPoint(ShaderStage stages) {
		if (HasCrossGroupConflicts(stages, ShaderStage::Graphics, ShaderStage::Compute, ShaderStage::RayTracing))
			throw std::invalid_argument("Conflicting shader stage");
		if ((stages & ShaderStage::Graphics) != ShaderStage::Unknown)
			return vk::PipelineBindPoint::eGraphics;
		if ((stages & ShaderStage::Compute) != ShaderStage::Unknown)
			return vk::PipelineBindPoint::eCompute;
		if ((stages & ShaderStage::RayTracing) != ShaderStage::Unknown)
			return vk::PipelineBindPoint::eRayTracingKHR;
		throw std::invalid_argument("No shader stage specified");
	}

	decltype(auto) GetShaderSegmentAndBinding(
		VulkanGPUExecutable* exec,
        std::size_t ns, 
		std::size_t where
	) {
		auto seg_id = exec->GetAssociatedShaderDataSegmentID();
		if (!seg_id) {
			throw std::invalid_argument("Invalid shader data segment");
		}
		auto* seg = plastic::utility::QueryObject<VulkanShaderDataSegment>(*seg_id);
		if (!seg) {
			throw std::runtime_error("Shader data segment lost");
		}
		auto&& [info, sampler] = seg->GetLayoutBinding(ns, where);
		if (sampler) {
			throw std::logic_error(std::format("Binding (set={}, binding={}) is immutable sampler", ns, where));
		}
		return std::make_pair(seg, info);
	}

	enum class WaitFor : std::uint8_t {
		Frame,
		RenderingStart,
		RenderingEnd,
		GraphicsBindPoint,
		ComputeBindPoint,
		RayTracingBindPoint,
		Count
	};
	
	class StateMachine {
	private:

		VulkanGPUExecutable* m_graphics_exec = nullptr;
		VulkanGPUExecutable* m_compute_exec = nullptr;
		VulkanGPUExecutable* m_ray_tracing_exec = nullptr;

		VulkanResource* m_frame_texture = nullptr;
		VulkanView* m_frame_view = nullptr;

		VulkanResource* m_depth_stencil = nullptr;
		std::vector<VulkanResource*> m_render_targets;

		std::array<std::deque<std::coroutine_handle<>>, static_cast<std::size_t>(WaitFor::Count)> m_pending_queues; 

		bool m_is_rendering = false;		
		
		bool IsConditionSatisfied(WaitFor type) const {
			switch (type) {
			case WaitFor::Frame:				return m_frame_texture && m_frame_view;
			case WaitFor::RenderingStart:		return m_is_rendering;
			case WaitFor::RenderingEnd:			return !m_is_rendering;
			case WaitFor::GraphicsBindPoint:	return m_graphics_exec != nullptr;
			case WaitFor::ComputeBindPoint:	  	return m_compute_exec != nullptr;
			case WaitFor::RayTracingBindPoint: 	return m_ray_tracing_exec != nullptr;
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
		std::span<VulkanResource* const> GetRenderTargets() const { 
			return m_render_targets;
		}
		
		VulkanResource* GetDepthStencil() const { return m_depth_stencil; }

		void SetFrame(VulkanResource* frame_texture, VulkanView* frame_view) {
			m_frame_texture = frame_texture;
			m_frame_view = frame_view;
			ResumeIfReady(WaitFor::Frame);
		}

		void BeginRendering(std::span<VulkanResource* const> rt, VulkanResource* ds) {
			m_render_targets.assign(rt.begin(), rt.end());
			m_depth_stencil = ds;
			m_is_rendering = true;
			ResumeIfReady(WaitFor::RenderingStart);
		}
	
		void EndRendering() {
			m_is_rendering = false;
			m_render_targets.clear();
			m_depth_stencil = nullptr;
			ResumeIfReady(WaitFor::RenderingEnd);
		}
	
		void SetGraphicsExecutable(VulkanGPUExecutable* exec) {
			if (!m_is_rendering) throw std::logic_error("Graphics executable must be set inside a rendering instance");
			m_graphics_exec = exec;
			ResumeIfReady(WaitFor::GraphicsBindPoint);
		}
	
		void SetComputeExecutable(VulkanGPUExecutable* exec) {
			if (m_is_rendering) throw std::logic_error("Compute executable cannot be set inside a rendering instance");
			m_compute_exec = exec;
			ResumeIfReady(WaitFor::ComputeBindPoint);
		}
	
		void SetRayTracingExecutable(VulkanGPUExecutable* exec) {
			if (m_is_rendering) throw std::logic_error("Ray tracing executable cannot be set inside a rendering instance");
			m_ray_tracing_exec = exec;
			ResumeIfReady(WaitFor::RayTracingBindPoint);
		}

		auto WaitForFrame() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_frame_texture && sm.m_frame_view; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::Frame); }
				std::pair<VulkanResource*, VulkanView*> await_resume() const {
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
		
		auto WaitForExecutable(vk::PipelineBindPoint bind_point) {
			struct Awaiter {
				StateMachine& sm;
				vk::PipelineBindPoint bind_point;
				bool await_ready() const {
					switch (bind_point) {
					case vk::PipelineBindPoint::eGraphics: return sm.m_graphics_exec != nullptr;
					case vk::PipelineBindPoint::eCompute: return sm.m_compute_exec != nullptr;
					case vk::PipelineBindPoint::eRayTracingKHR: return sm.m_ray_tracing_exec != nullptr;
					default: throw std::invalid_argument("Invalid bind point");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					WaitFor wait_type;
					switch (bind_point) {
					case vk::PipelineBindPoint::eGraphics: wait_type = WaitFor::GraphicsBindPoint; break;
					case vk::PipelineBindPoint::eCompute: wait_type = WaitFor::ComputeBindPoint; break;
					case vk::PipelineBindPoint::eRayTracingKHR: wait_type = WaitFor::RayTracingBindPoint; break;
					default: throw std::invalid_argument("Invalid bind point");
					}
					sm.AddPending(h, wait_type);
				}
				VulkanGPUExecutable* await_resume() const {
					switch (bind_point) {
					case vk::PipelineBindPoint::eGraphics: return sm.m_graphics_exec;
					case vk::PipelineBindPoint::eCompute: return sm.m_compute_exec;
					case vk::PipelineBindPoint::eRayTracingKHR: return sm.m_ray_tracing_exec;
					default: return nullptr;
					}
				}
			};
			return Awaiter{*this, bind_point};
		}
	
		auto WaitForContext(vk::PipelineBindPoint bind_point) {
			struct Awaiter {
				StateMachine& sm;
				vk::PipelineBindPoint bind_point;
				bool await_ready() const {
					switch (bind_point) {
					case vk::PipelineBindPoint::eGraphics:
						return sm.m_is_rendering;
					case vk::PipelineBindPoint::eCompute:
						return !sm.m_is_rendering;
					case vk::PipelineBindPoint::eRayTracingKHR:
						return !sm.m_is_rendering;
					default:
						throw std::invalid_argument("Invalid bind point");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					switch (bind_point) {
					case vk::PipelineBindPoint::eGraphics:
						if (!sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingStart);
						break;
					case vk::PipelineBindPoint::eCompute:
						if (sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingEnd);
						break;
					case vk::PipelineBindPoint::eRayTracingKHR:
						if (sm.m_is_rendering) sm.AddPending(h, WaitFor::RenderingEnd);
					default:
						throw std::invalid_argument("Invalid bind point");
					}
				}
				void await_resume() const {}
			};
			return Awaiter{*this, bind_point};
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

}

namespace fyuu_rhi::vulkan {

	VulkanCommandBuffer::VulkanCommandBuffer(VulkanLogicalDevice const& logical_device, CommandObjectType type, vk::CommandBufferLevel level, std::string_view debug_name) 
		: PolymorphicCommandBufferBase(this),
		VulkanCommon(this),
		m_logical_device_id(logical_device.GetID()),
		m_dispatcher(logical_device.GetDispatcher()),
		m_pool(logical_device.CreateCommandPool(type, debug_name)),
		m_impl(logical_device.CreateCommandBuffer(*m_pool, level, debug_name)),
		m_commands(),
		m_sync(),
		m_recording_flag(false) {
	
	}

	VulkanCommandBuffer& VulkanCommandBuffer::BeginRecording() {

		m_sync.Acquire();

		if (!std::exchange(m_recording_flag, true)) {
			if (!m_logical_device_id) {
				throw std::invalid_argument("invalid logical device");
			}
			auto* logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);
			if (!logical_device) {
				throw std::runtime_error("logical device lost");
			}
			logical_device->ResetCommandPool(*m_pool, {});
			vk::CommandBufferBeginInfo begin_info{};
			m_impl->begin(begin_info, *m_dispatcher);
			m_commands.clear();
		}

		return *this;

	}

	void VulkanCommandBuffer::EndRecording() {
		
		using GPUTask = plastic::concurrency::SerialTask<void>;

		m_sync.ThrowIfNotOwned();

		if (std::exchange(m_recording_flag, false) && !m_commands.empty()) {

			StateMachine state_machine;

			auto visitor = plastic::utility::Overload{
				[this, &state_machine](CmdBeginRendering const* begin_rendering) -> GPUTask {

					std::vector<vk::ImageMemoryBarrier2> texture_barriers;
					std::vector<vk::RenderingAttachmentInfo> color_attachments;
					std::vector<VulkanResource*> render_targets;

					std::size_t rt_descs_size = begin_rendering->render_targets.size();
					for (std::size_t i = 0; i < rt_descs_size; ++i) {

						auto& rt_desc = begin_rendering->render_targets[i];

						auto& rt_id = rt_desc.texture_id;
						if (!rt_id) {
							throw std::invalid_argument("Invalid render target");
						}

						VulkanResource* rt = plastic::utility::QueryObject<VulkanResource>(*rt_id);
						if (!rt) {
							throw std::runtime_error("Render target lost");
						}

						auto& rtv_id = rt_desc.view_id;
						if (!rtv_id) {
							throw std::invalid_argument("Invalid render target view");
						}

						VulkanView* rtv = plastic::utility::QueryObject<VulkanView>(*rtv_id);
						if (!rtv) {
							throw std::runtime_error("Render target view lost");
						}

						AddBarrier(
							vk::PipelineStageFlagBits2::eColorAttachmentOutput,
							vk::AccessFlagBits2::eColorAttachmentWrite,
							vk::ImageLayout::eColorAttachmentOptimal,
							rt,
							texture_barriers
						);

						color_attachments.emplace_back(
							rtv->GetNative().texture,
							vk::ImageLayout::eColorAttachmentOptimal,
							vk::ResolveModeFlagBits::eNone,
							nullptr,
							vk::ImageLayout::eUndefined,
							ToVulkanLoadOp(rt_desc.load),
							ToVulkanStoreOp(rt_desc.store),
							rt_desc.load == LoadOp::Clear ? vk::ClearValue(rt_desc.clear) : vk::ClearValue{},
							nullptr
						);

						render_targets.emplace_back(rt);

					}

					std::optional<std::pair<vk::RenderingAttachmentInfo, vk::RenderingAttachmentInfo>> depth_stencil_info;
					VulkanResource* depth_stencil = nullptr;
					auto const& ds_desc = begin_rendering->depth_stencil;

					if (ds_desc) {
						auto& ds_id = ds_desc->texture_id;
						if (!ds_id) {
							throw std::invalid_argument("Invalid depth stencil");
						}
						depth_stencil = plastic::utility::QueryObject<VulkanResource>(*ds_id);
						if (!depth_stencil) {
							throw std::runtime_error("Depth stencil lost");
						}

						auto& dsv_id = ds_desc->view_id;
						if (!dsv_id) {
							throw std::invalid_argument("Invalid Depth stencil view");
						}

						VulkanView* dsv = plastic::utility::QueryObject<VulkanView>(*dsv_id);
						if (!dsv) {
							throw std::runtime_error("Depth stencil view lost");
						}

						AddBarrier(
							vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
							vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
							vk::ImageLayout::eDepthStencilAttachmentOptimal,
							depth_stencil,
							texture_barriers
						);

						depth_stencil_info.emplace(
							vk::RenderingAttachmentInfo(
								dsv->GetNative().texture,
								vk::ImageLayout::eDepthStencilAttachmentOptimal,
								vk::ResolveModeFlagBits::eNone,
								nullptr,
								vk::ImageLayout::eUndefined,
								ToVulkanLoadOp(ds_desc->d_load),
								ToVulkanStoreOp(ds_desc->d_store),
								ds_desc->d_load == LoadOp::Clear ? vk::ClearDepthStencilValue(ds_desc->depth_clear, ds_desc->stencil_clear) : vk::ClearDepthStencilValue{},
								nullptr
							),
							vk::RenderingAttachmentInfo(
								dsv->GetNative().texture,
								vk::ImageLayout::eDepthStencilAttachmentOptimal,
								vk::ResolveModeFlagBits::eNone,
								nullptr,
								vk::ImageLayout::eUndefined,
								ToVulkanLoadOp(ds_desc->s_load),
								ToVulkanStoreOp(ds_desc->s_store),
								ds_desc->s_load == LoadOp::Clear ? vk::ClearDepthStencilValue(ds_desc->depth_clear, ds_desc->stencil_clear) : vk::ClearDepthStencilValue{},
								nullptr
							)
						);

					}

					auto [frame, frame_view] = co_await state_machine.WaitForFrame();

					if (!texture_barriers.empty()) {
						vk::DependencyInfo dep_info(
							{},
							{},
							{},
							texture_barriers
						);
						
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					auto frame_size = frame->GetTextureSize();

					vk::RenderingInfo rendering_info(
						{},
						{
							{ 0u, 0u },
							{ static_cast<std::uint32_t>(frame_size.width), static_cast<std::uint32_t>(frame_size.height) }
						},
						GetLayerCount(render_targets, depth_stencil),
						0u,
						color_attachments,
						depth_stencil_info ? &depth_stencil_info->first : nullptr,
						depth_stencil_info ? &depth_stencil_info->second : nullptr
					);

					m_impl->beginRendering(rendering_info, *m_dispatcher);

					state_machine.BeginRendering(render_targets, depth_stencil);

					co_return;

				},
				[this, &state_machine](CmdEndRendering const*) -> GPUTask {

					std::vector<vk::ImageMemoryBarrier2> texture_barriers;

					co_await state_machine.WaitForRenderingStart();

					m_impl->endRendering(*m_dispatcher);

					for (VulkanResource* rt : state_machine.GetRenderTargets()) {
						AddBarrier(
							vk::PipelineStageFlagBits2::eBottomOfPipe,
							vk::AccessFlagBits2::eNone,
							vk::ImageLayout::ePresentSrcKHR,
							rt,
							texture_barriers
						);
					}

					if (auto* depth_stencil = state_machine.GetDepthStencil()) {
						AddBarrier(
							vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
							vk::AccessFlagBits2::eDepthStencilAttachmentRead,
							vk::ImageLayout::eDepthStencilReadOnlyOptimal,
							depth_stencil,
							texture_barriers
						);
					}

					if (!texture_barriers.empty()) {
						vk::DependencyInfo dep_info(
							{},
							{},
							{},
							texture_barriers
						);
						
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					state_machine.EndRendering();

					co_return;

				},
				[this, &state_machine](CmdSetViewports const* set_viewport) -> GPUTask {
					
					co_await state_machine.WaitForRenderingStart();

					std::vector<vk::Viewport> viewports;

					std::ranges::transform(
						set_viewport->viewports,
						std::back_inserter(viewports),
						[](CmdSetViewports::Viewport const& viewport) {
							return vk::Viewport(
								viewport.x,
								viewport.y,
								viewport.width,
								viewport.height,
								viewport.min_depth,
								viewport.max_depth
							);
						}
					);

					m_impl->setViewport(0, viewports, *m_dispatcher);
	
					co_return;

				},
				[this, &state_machine](CmdSetScissors const* set_scissors) -> GPUTask {
					
					co_await state_machine.WaitForRenderingStart();

					std::vector<vk::Rect2D> scissors;

					std::ranges::transform(
						set_scissors->scissors,
						std::back_inserter(scissors),
						[](CmdSetScissors::Scissor const& scissor) {
							return vk::Rect2D{
								vk::Offset2D{static_cast<int32_t>(scissor.x), static_cast<int32_t>(scissor.y)},
								vk::Extent2D{static_cast<uint32_t>(scissor.width), static_cast<uint32_t>(scissor.height)}
							};
						}
					);

					m_impl->setScissor(0, scissors, *m_dispatcher);

					co_return;

				},
				[this, &state_machine](CmdExecute const* execute) -> GPUTask {

					auto& gpu_exec_id = execute->gpu_exec_id;
					if (!gpu_exec_id) {
						throw std::invalid_argument("Invalid GPU executable program");
					}

					VulkanGPUExecutable* gpu_exec = plastic::utility::QueryObject<VulkanGPUExecutable>(*gpu_exec_id);
					if (!gpu_exec) {
						throw std::runtime_error("GPU executable program lost");
					}

					auto&& [pso_type, pso] = gpu_exec->GetPSO();

					vk::PipelineBindPoint bind_point{};

					if ((pso_type & ShaderStage::Graphics) != ShaderStage::Unknown) {
						co_await state_machine.WaitForRenderingStart();
						bind_point = vk::PipelineBindPoint::eGraphics;
					}

					if ((pso_type & ShaderStage::Compute) != ShaderStage::Unknown) {
						co_await state_machine.WaitForRenderingEnd();
						bind_point = vk::PipelineBindPoint::eCompute;
					}

					if ((pso_type & ShaderStage::RayTracing) != ShaderStage::Unknown) {
						co_await state_machine.WaitForRenderingEnd();
						bind_point = vk::PipelineBindPoint::eRayTracingKHR;
					}

					m_impl->bindPipeline(bind_point, pso, *m_dispatcher);

					std::optional<std::size_t> shader_data_segment_id = gpu_exec->GetAssociatedShaderDataSegmentID();
					if (!shader_data_segment_id) {
						throw std::invalid_argument("Invalid shader data segment");
					}
					auto* shader_data_segment = plastic::utility::QueryObject<VulkanShaderDataSegment>(*shader_data_segment_id);
					if (!shader_data_segment) {
						throw std::runtime_error("Shader data segment lost");
					}

					auto immutable_samplers = shader_data_segment->GetImmutableSamplers();

					std::unordered_map<std::size_t, std::vector<vk::WriteDescriptorSet>> writes_per_set;
					for (auto const& info : immutable_samplers) {
						vk::DescriptorImageInfo image_info(info.sampler->GetNative());
						writes_per_set[info.set].emplace_back(
							nullptr,
							info.binding,
							0,
							1,
							vk::DescriptorType::eSampler,
							&image_info,
							nullptr,
							nullptr
						);
					}

					auto pipeline_layout = shader_data_segment->GetPipelineLayout();
					for (auto& [set, writes] : writes_per_set) {
						m_impl->pushDescriptorSetKHR(
							bind_point,
							pipeline_layout,
							static_cast<std::uint32_t>(set),
							static_cast<std::uint32_t>(writes.size()),
							writes.data(),
							*m_dispatcher
						);
					}

					if ((pso_type & ShaderStage::Graphics) != ShaderStage::Unknown) {
						state_machine.SetGraphicsExecutable(gpu_exec);
					}

					if ((pso_type & ShaderStage::Compute) != ShaderStage::Unknown) {
						state_machine.SetComputeExecutable(gpu_exec);
					}

					if ((pso_type & ShaderStage::RayTracing) != ShaderStage::Unknown) {
						state_machine.SetRayTracingExecutable(gpu_exec);
					}

					co_return;

				},
				[this, &state_machine](CmdDispatchComputingTask const* dispatch_computing_task) -> GPUTask {
					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eCompute);
					m_impl->dispatch(
						dispatch_computing_task->threads_x, 
						dispatch_computing_task->threads_y, 
						dispatch_computing_task->threads_z,
						*m_dispatcher
					);
					co_return;
				},
				[this, &state_machine](CmdBindVertexBuffers const* bind_vertex_buffers) -> GPUTask {

					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);

					std::vector<vk::BufferMemoryBarrier2> buffer_barriers;
					std::map<std::size_t, std::pair<vk::Buffer, vk::DeviceSize>> slot_map;

					for (auto const& binding : bind_vertex_buffers->bindings) {
						auto& id = binding.id;
						if (!id) {
							throw std::invalid_argument("Invalid vertex buffer");
						}
				
						VulkanResource* vertex_buffer = plastic::utility::QueryObject<VulkanResource>(*id);
						if (!vertex_buffer) {
							throw std::runtime_error("Vertex buffer lost");
						}

						AddBarrier(
							vk::PipelineStageFlagBits2::eVertexAttributeInput, 
							vk::AccessFlagBits2::eVertexAttributeRead, 
							vertex_buffer,
							buffer_barriers
						);

						slot_map.try_emplace(binding.where, vertex_buffer->GetNative().buffer, binding.offset);

					}

					if (!slot_map.empty()) {

						vk::DependencyInfo dep_info(
							{}, 
							{}, 
							buffer_barriers,
							{}
						);

						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);

						std::vector<vk::Buffer> vertex_buffers;
						std::vector<vk::DeviceSize> offsets;

						auto [first, _]= *slot_map.begin();
						auto [last, __]= *slot_map.rbegin();
						std::size_t count = last - first + 1;
					
						for (std::size_t slot = first; slot <= last; ++slot) {
							if (auto it = slot_map.find(slot); it != slot_map.end()) {
								auto&& [buffer, offset] = it->second;
								vertex_buffers.emplace_back(buffer);
								offsets.emplace_back(offset);
							} 
							else {
								vertex_buffers.emplace_back(nullptr);
								offsets.emplace_back(0u);
							}
						}
					
						m_impl->bindVertexBuffers(first, count, vertex_buffers.data(), offsets.data(), *m_dispatcher);

					}

					co_return;

				},
				[this, &state_machine](CmdBindIndexBuffer const* bind_index_buffer) -> GPUTask {

					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);

					auto& id = bind_index_buffer->id;
					if (!id) {
						throw std::invalid_argument("Invalid index buffer");
					}
			
					VulkanResource* index_buffer = plastic::utility::QueryObject<VulkanResource>(*id);
					if (!index_buffer) {
						throw std::runtime_error("Index buffer lost");
					}
				
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;

					AddBarrier(
						vk::PipelineStageFlagBits2::eIndexInput, 
						vk::AccessFlagBits2::eIndexRead, 
						index_buffer,
						buffer_barriers
					);

					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info(
							{}, 
							{}, 
							buffer_barriers,
							{}
						);
	
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					switch(index_buffer->GetFormat()) {
					case vk::Format::eR16Uint:
						m_impl->bindIndexBuffer(index_buffer->GetNative().buffer, 0u, vk::IndexType::eUint16, *m_dispatcher);
						break;
					case vk::Format::eR32Uint:
						m_impl->bindIndexBuffer(index_buffer->GetNative().buffer, 0u, vk::IndexType::eUint32, *m_dispatcher);
						break;
					default:
						throw std::invalid_argument("Unknown index buffer format");
					}

					co_return;

				},
				[this, &state_machine](CmdDrawInstanced const* draw_instanced) -> GPUTask {
					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);
					m_impl->draw(draw_instanced->vertices, draw_instanced->instances, draw_instanced->start_vertex, draw_instanced->first_instance, *m_dispatcher);
					co_return;
				},
				[this, &state_machine](CmdDrawIndexedInstanced const* draw_indexed_instanced) -> GPUTask {
					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);
					m_impl->drawIndexed(draw_indexed_instanced->indices, draw_indexed_instanced->instances, draw_indexed_instanced->first_index, draw_indexed_instanced->start_vertex, draw_indexed_instanced->first_instance, *m_dispatcher);
					co_return;
				},
				[this, &state_machine](CmdBindBuffer const* bind_buffer) -> GPUTask {

					auto stages = bind_buffer->stages;
					vk::PipelineBindPoint bind_point = GetBindPoint(stages);
					VulkanGPUExecutable* exec = co_await state_machine.WaitForExecutable(bind_point);
					auto&& [shader_data_segment, info] = GetShaderSegmentAndBinding(exec, bind_buffer->ns, bind_buffer->where);

					auto& id = bind_buffer->id;
					if (!id) {
						throw std::invalid_argument("Invalid buffer");
					}

					VulkanResource* buffer = plastic::utility::QueryObject<VulkanResource>(*id);
					if (!buffer) {
						throw std::runtime_error("Buffer lost");
					}

					vk::DescriptorBufferInfo buffer_info(
						buffer->GetNative().buffer,
						bind_buffer->offset,
						std::numeric_limits<vk::DeviceSize>::max()
					);
					
					std::optional<vk::BufferView> buffer_view;

					if (info->descriptorType == vk::DescriptorType::eUniformTexelBuffer || 
						info->descriptorType == vk::DescriptorType::eStorageTexelBuffer) {
						auto& view_id = bind_buffer->texel_view_id;
						if (!view_id) {
							throw std::invalid_argument(
								std::format(
									"The binding (set = {}, binding = {}) is texel buffer and view must be set", 
									bind_buffer->ns, info->binding
								)
							);
						}
				
						VulkanView* texel_view = plastic::utility::QueryObject<VulkanView>(*view_id);
						if (!texel_view) {
							throw std::runtime_error("View lost");
						}

						buffer_view.emplace(texel_view->GetNative().buffer);

					}

					vk::WriteDescriptorSet write_info(
						nullptr,
						info->binding,
						0u,
						info->descriptorCount,
						info->descriptorType,
						nullptr,
						&buffer_info,
						buffer_view ? &*buffer_view : nullptr
					);

					vk::PipelineStageFlags2 target_stage;
					if ((stages & ShaderStage::Vertex) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eVertexShader;
					if ((stages & ShaderStage::TessellationControl) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eTessellationControlShader;
					if ((stages & ShaderStage::TessellationEvaluation) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
					if ((stages & ShaderStage::Geometry) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eGeometryShader;
					if ((stages & ShaderStage::Fragment) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eFragmentShader;
					if ((stages & ShaderStage::Compute) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eComputeShader;
					if ((stages & ShaderStage::RayTracing) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eRayTracingShaderKHR;

					vk::AccessFlags2 target_access;
					switch (info->descriptorType) {
					case vk::DescriptorType::eUniformBuffer:
					case vk::DescriptorType::eUniformBufferDynamic:
						target_access = vk::AccessFlagBits2::eUniformRead;
						break;
					case vk::DescriptorType::eStorageBuffer:
					case vk::DescriptorType::eStorageBufferDynamic:
						target_access = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
						break;
					case vk::DescriptorType::eUniformTexelBuffer:
						target_access = vk::AccessFlagBits2::eShaderSampledRead;
						break;
					case vk::DescriptorType::eStorageTexelBuffer:
						target_access = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
						break;
					default:
						break;
					}

					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;
					auto [current_stage, current_access, current_layout] = buffer->GetState();
					if (current_stage != target_stage || current_access != target_access) {
						buffer_barriers.emplace_back(
							current_stage,
							current_access,
							target_stage,
							target_access,
							std::numeric_limits<uint32_t>::max(), 		// srcQueueFamilyIndex
							std::numeric_limits<uint32_t>::max(), 		// dstQueueFamilyIndex
							buffer->GetNative().buffer,
							0u,											// offset
							std::numeric_limits<vk::DeviceSize>::max()	// size							
						);
						buffer->SetState({ target_stage, target_access, current_layout });
					}

					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, {});
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					m_impl->pushDescriptorSetKHR(
						bind_point,
						shader_data_segment->GetPipelineLayout(),
						bind_buffer->ns,
						1u,
						&write_info, 
						*m_dispatcher
					);

					co_return;

				},
				[this, &state_machine](CmdPushData const* push_data) -> GPUTask {

					auto& bytes = push_data->bytes;
					std::size_t push_size = bytes.size();

					if (push_size % 4u != 0) {
						throw std::runtime_error("Push bytes must be 4-byte aligned.");
					}

					VulkanGPUExecutable* exec = co_await state_machine.WaitForExecutable(GetBindPoint(push_data->stages));
					auto&& [shader_data_segment, _] = GetShaderSegmentAndBinding(exec, push_data->ns, push_data->where);
					
					m_impl->pushConstants(
						shader_data_segment->GetPipelineLayout(),
						ShaderStageToVkBits(push_data->stages),
						static_cast<std::size_t>(push_data->offset), 
						bytes.size(), 
						bytes.data(),
						*m_dispatcher
					);

				},
				[this, &state_machine](CmdBindSampler const* bind_sampler) -> GPUTask {
					
					vk::PipelineBindPoint bind_point = GetBindPoint(bind_sampler->stages);
					VulkanGPUExecutable* exec = co_await state_machine.WaitForExecutable(bind_point);
					auto&& [shader_data_segment, info] = GetShaderSegmentAndBinding(exec, bind_sampler->ns, bind_sampler->where);

					auto& id = bind_sampler->id;
					if (!id) {
						throw std::invalid_argument("Invalid sampler");
					}

					auto sampler = plastic::utility::QueryObject<VulkanSampler>(*id);
					if (!sampler) {
						throw std::runtime_error("Sampler lost");
					}

					vk::DescriptorImageInfo image_info(sampler->GetNative());

					vk::WriteDescriptorSet write_info(
						nullptr,
						info->binding,
						0u,
						info->descriptorCount,
						info->descriptorType,
						&image_info,
						nullptr,
						nullptr
					);				

					m_impl->pushDescriptorSetKHR(
						bind_point,
						shader_data_segment->GetPipelineLayout(),
						bind_sampler->ns,
						1u,
						&write_info, 
						*m_dispatcher
					);

					co_return;

				},
				[this, &state_machine](CmdBindTexture const* bind_texture) -> GPUTask {

					auto stages = bind_texture->stages;
					vk::PipelineBindPoint bind_point = GetBindPoint(stages);
					VulkanGPUExecutable* exec = co_await state_machine.WaitForExecutable(bind_point);
					auto&& [shader_data_segment, info] = GetShaderSegmentAndBinding(exec, bind_texture->ns, bind_texture->where);

					auto& texture_id = bind_texture->texture_id;
					if (!texture_id) {
						throw std::invalid_argument("Invalid texture");
					}

					VulkanResource* texture = plastic::utility::QueryObject<VulkanResource>(*texture_id);
					if (!texture) {
						throw std::runtime_error("Texture lost");
					}
					
					auto& view_id = bind_texture->view_id;
					if (!view_id) {
						throw std::invalid_argument("Invalid view");
					}

					VulkanView* view = plastic::utility::QueryObject<VulkanView>(*view_id);
					if (!view) {
						throw std::runtime_error("View lost");
					}

					vk::ImageLayout target_layout;
					vk::AccessFlags2 target_access;
					switch (info->descriptorType) {
					case vk::DescriptorType::eSampledImage:
						target_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
						target_access = vk::AccessFlagBits2::eShaderSampledRead;
						break;
					case vk::DescriptorType::eStorageImage:
						target_layout = vk::ImageLayout::eGeneral;
						target_access = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite;
						break;
					case vk::DescriptorType::eInputAttachment:
						target_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
						target_access = vk::AccessFlagBits2::eInputAttachmentRead;
						break;
					case vk::DescriptorType::eCombinedImageSampler:
						throw std::runtime_error(
							std::format(
								"Combined image sampler binding (set = {}, binding = {}) is deprecated.",
								bind_texture->ns, info->binding
							)
						);
					default:
						throw std::invalid_argument(
							std::format(
								"Invalid descriptor type for CmdBindTexture: {}",
								vk::to_string(info->descriptorType)
							)
						);
					}

					vk::PipelineStageFlags2 target_stage;
					if ((stages & ShaderStage::Vertex) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eVertexShader;
					if ((stages & ShaderStage::TessellationControl) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eTessellationControlShader;
					if ((stages & ShaderStage::TessellationEvaluation) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
					if ((stages & ShaderStage::Geometry) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eGeometryShader;
					if ((stages & ShaderStage::Fragment) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eFragmentShader;
					if ((stages & ShaderStage::Compute) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eComputeShader;
					if ((stages & ShaderStage::RayTracing) != ShaderStage::Unknown) target_stage |= vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
				
					if (!target_stage) {
						throw std::invalid_argument("No shader stage specified for texture binding");
					}

					SmallVector<vk::ImageMemoryBarrier2, 1u> texture_barriers;
					auto [current_stage, current_access, current_layout] = texture->GetState();
					if (current_stage != target_stage || current_access != target_access || current_layout != target_layout) {
						texture_barriers.emplace_back(
							current_stage,
							current_access,
							target_stage,
							target_access,
							current_layout,
							target_layout,
							std::numeric_limits<std::uint32_t>::max(), // srcQueueFamilyIndex
							std::numeric_limits<std::uint32_t>::max(), // dstQueueFamilyIndex
							texture->GetNative().texture,
							texture->WholeResourceRange()
						);
						texture->SetState({ target_stage, target_access, target_layout });
					}

					vk::DependencyInfo dep_info({}, {}, {}, texture_barriers);
					m_impl->pipelineBarrier2(dep_info, *m_dispatcher);

					vk::DescriptorImageInfo image_info(
						nullptr,
						view->GetNative().texture,
						target_layout
					);

					vk::WriteDescriptorSet write_info(
						nullptr,				// dstSet (null for push descriptors)
						info->binding,
						0u,						// dstArrayElement
						info->descriptorCount,
						info->descriptorType,
						&image_info, 			// pImageInfo
						nullptr,				// pBufferInfo
						nullptr					// pTexelBufferView
					);

					m_impl->pushDescriptorSetKHR(
						bind_point,
						shader_data_segment->GetPipelineLayout(),
						bind_texture->ns,
						1u, // descriptorWriteCount
						&write_info, 
						*m_dispatcher
					);

					co_return;

				},
				[this, &state_machine](CmdCopyTextureToBuffer const* copy_cmd) -> GPUTask {
					
					co_await state_machine.WaitForRenderingEnd();

					if (!copy_cmd->src_id || !copy_cmd->dest_id) {
						throw std::invalid_argument("Invalid source or destination");
					}
				
					VulkanResource* src_texture = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->src_id);
					VulkanResource* dest_buffer = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->dest_id);
					if (!src_texture || !dest_buffer) {
						throw std::runtime_error("Resource lost");
					}
				
					auto tex_desc = src_texture->GetTextureDescription();
					if (!tex_desc) {
						throw std::invalid_argument("Source is not a texture");
					}
					if (!dest_buffer->GetBufferDescription()) {
						throw std::invalid_argument("Destination is not a buffer");
					}

					SmallVector<vk::ImageMemoryBarrier2, 1u> texture_barriers;
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;

					auto aspects = src_texture->GetAspectFlags();
					bool has_depth = (aspects & vk::ImageAspectFlagBits::eDepth) != vk::ImageAspectFlagBits::eNone;
					bool has_stencil = (aspects & vk::ImageAspectFlagBits::eStencil) != vk::ImageAspectFlagBits::eNone;
				
					vk::ImageSubresourceLayers base_subresource;
					base_subresource.mipLevel = static_cast<std::uint32_t>(copy_cmd->src_subresource.mip_level);
					base_subresource.baseArrayLayer = static_cast<std::uint32_t>(copy_cmd->src_subresource.array_layer);
					base_subresource.layerCount = static_cast<std::uint32_t>(copy_cmd->src_subresource.layer_count);

					vk::Offset3D src_offset(
						static_cast<std::int32_t>(copy_cmd->src_offset.x),
						static_cast<std::int32_t>(copy_cmd->src_offset.y),
						static_cast<std::int32_t>(copy_cmd->src_offset.z)
					);
				
					vk::Extent3D extent(
						static_cast<std::uint32_t>(copy_cmd->extent.width),
						static_cast<std::uint32_t>(copy_cmd->extent.height),
						static_cast<std::uint32_t>(copy_cmd->extent.depth)
					);

					vk::Extent3D actual_extent = extent;
					if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
						actual_extent = GetMipExtent(tex_desc->extent, base_subresource.mipLevel);
					}

					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						vk::ImageLayout::eTransferSrcOptimal,
						src_texture,
						texture_barriers
					);
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						dest_buffer,
						buffer_barriers
					);

					if (!texture_barriers.empty() || !buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, texture_barriers);
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					auto DoCopy = [&](vk::ImageAspectFlagBits aspect, vk::DeviceSize buffer_offset) {
						vk::ImageSubresourceLayers subresource = base_subresource;
						subresource.aspectMask = aspect;

						vk::BufferImageCopy region(
							buffer_offset,
							0, 0,
							subresource,
							src_offset,
							actual_extent
						);

						m_impl->copyImageToBuffer(
							src_texture->GetNative().texture,
							vk::ImageLayout::eTransferSrcOptimal,
							dest_buffer->GetNative().buffer,
							1u,
							&region, 
							*m_dispatcher
						);
						};

					vk::DeviceSize current_offset = static_cast<vk::DeviceSize>(copy_cmd->dest_offset);
					if (has_depth && has_stencil) {
						vk::Extent3D mip_extent = actual_extent;
						std::size_t depth_size = EstimateAspectBufferSize(tex_desc->format, vk::ImageAspectFlagBits::eDepth, mip_extent);
						std::size_t stencil_size = EstimateAspectBufferSize(tex_desc->format, vk::ImageAspectFlagBits::eStencil, mip_extent);

						DoCopy(vk::ImageAspectFlagBits::eDepth, current_offset);
						DoCopy(vk::ImageAspectFlagBits::eStencil, current_offset + depth_size);
					} 
					else if (has_depth) {
						DoCopy(vk::ImageAspectFlagBits::eDepth, current_offset);
					} 
					else if (has_stencil) {
						DoCopy(vk::ImageAspectFlagBits::eStencil, current_offset);
					} 
					else {
						DoCopy(vk::ImageAspectFlagBits::eColor, current_offset);
					}

					co_return;

				},
				[this, &state_machine](CmdCopyBufferToTexture const* copy_cmd) -> GPUTask {

					co_await state_machine.WaitForRenderingEnd();

					if (!copy_cmd->src_id || !copy_cmd->dest_id) {
						throw std::invalid_argument("Invalid source or destination");
					}
				
					VulkanResource* src_buffer = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->src_id);
					VulkanResource* dest_texture = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->dest_id);
					if (!src_buffer || !dest_texture) {
						throw std::runtime_error("Resource lost");
					}
				
					auto tex_desc = dest_texture->GetTextureDescription();
					if (!src_buffer->GetBufferDescription()) {
						throw std::invalid_argument("Source is not a buffer");
					}
					if (!tex_desc) {
						throw std::invalid_argument("Destination is not a texture");
					}
				
					vk::ImageAspectFlags aspects = dest_texture->GetAspectFlags();
					bool has_depth = (aspects & vk::ImageAspectFlagBits::eDepth) != vk::ImageAspectFlagBits::eNone;
					bool has_stencil = (aspects & vk::ImageAspectFlagBits::eStencil) != vk::ImageAspectFlagBits::eNone;
				
					vk::ImageSubresourceLayers base_subresource;
					base_subresource.mipLevel = static_cast<std::uint32_t>(copy_cmd->dest_subresource.mip_level);
					base_subresource.baseArrayLayer = static_cast<std::uint32_t>(copy_cmd->dest_subresource.array_layer);
					base_subresource.layerCount = static_cast<std::uint32_t>(copy_cmd->dest_subresource.layer_count);
				
					vk::Offset3D dest_offset(
						static_cast<std::int32_t>(copy_cmd->dest_offset.x),
						static_cast<std::int32_t>(copy_cmd->dest_offset.y),
						static_cast<std::int32_t>(copy_cmd->dest_offset.z)
					);
				
					vk::Extent3D extent(
						static_cast<std::uint32_t>(copy_cmd->extent.width),
						static_cast<std::uint32_t>(copy_cmd->extent.height),
						static_cast<std::uint32_t>(copy_cmd->extent.depth)
					);
				
					vk::Extent3D actual_extent = extent;
					if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
						actual_extent = GetMipExtent(tex_desc->extent, base_subresource.mipLevel);
					}
				
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;
					SmallVector<vk::ImageMemoryBarrier2, 1u> texture_barriers;

					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::ImageLayout::eTransferDstOptimal,
						dest_texture,
						texture_barriers
					);
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						src_buffer,
						buffer_barriers
					);

					if (!texture_barriers.empty() || !buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, texture_barriers);
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					auto DoCopy = [&](vk::ImageAspectFlagBits aspect, vk::DeviceSize buffer_offset) {
						vk::ImageSubresourceLayers subresource = base_subresource;
						subresource.aspectMask = aspect;
				
						vk::BufferImageCopy region(
							buffer_offset,
							0, 0,
							subresource,
							dest_offset,
							actual_extent
						);
				
						m_impl->copyBufferToImage(
							src_buffer->GetNative().buffer,
							dest_texture->GetNative().texture,
							vk::ImageLayout::eTransferDstOptimal,
							1u,
							&region, 
							*m_dispatcher
						);
						};

					vk::DeviceSize current_offset = static_cast<vk::DeviceSize>(copy_cmd->src_offset);
					if (has_depth && has_stencil) {
						vk::Extent3D mip_extent = actual_extent;
						std::size_t depth_size = EstimateAspectBufferSize(tex_desc->format, vk::ImageAspectFlagBits::eDepth, mip_extent);
						std::size_t stencil_size = EstimateAspectBufferSize(tex_desc->format, vk::ImageAspectFlagBits::eStencil, mip_extent);

						DoCopy(vk::ImageAspectFlagBits::eDepth, current_offset);
						DoCopy(vk::ImageAspectFlagBits::eStencil, current_offset + depth_size);
					} 
					else if (has_depth) {
						DoCopy(vk::ImageAspectFlagBits::eDepth, current_offset);
					} 
					else if (has_stencil) {
						DoCopy(vk::ImageAspectFlagBits::eStencil, current_offset);
					} 
					else {
						DoCopy(vk::ImageAspectFlagBits::eColor, current_offset);
					}

					co_return;

				},
				[this, &state_machine](CmdCopyBufferToBuffer const* copy_cmd) -> GPUTask {

					co_await state_machine.WaitForRenderingEnd();

					if (!copy_cmd->src_id || !copy_cmd->dest_id) {
						throw std::invalid_argument("Invalid source or destination ID");
					}
				
					VulkanResource* src_buffer = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->src_id);
					VulkanResource* dest_buffer = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->dest_id);
					if (!src_buffer || !dest_buffer) {
						throw std::runtime_error("Resource lost");
					}
				
					if (!src_buffer->GetBufferDescription() || !dest_buffer->GetBufferDescription()) {
						throw std::invalid_argument("Both resources must be buffers");
					}
				
					SmallVector<vk::BufferMemoryBarrier2, 2u> buffer_barriers;
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						dest_buffer,
						buffer_barriers
					);
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						src_buffer,
						buffer_barriers
					);
					
					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, {});
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					vk::BufferCopy region(
						static_cast<vk::DeviceSize>(copy_cmd->src_offset),
						static_cast<vk::DeviceSize>(copy_cmd->dest_offset),
						static_cast<vk::DeviceSize>(copy_cmd->size)
					);
				
					m_impl->copyBuffer(src_buffer->GetNative().buffer, dest_buffer->GetNative().buffer, 1u, &region, *m_dispatcher);
				
					co_return;

				},
				[this, &state_machine](CmdCopyTextureToTexture const* copy_cmd) -> GPUTask {

					co_await state_machine.WaitForRenderingEnd();

					if (!copy_cmd->src_id || !copy_cmd->dest_id) {
						throw std::invalid_argument("Invalid source or destination ID");
					}

					VulkanResource* src_texture = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->src_id);
					VulkanResource* dest_texture = plastic::utility::QueryObject<VulkanResource>(*copy_cmd->dest_id);
					if (!src_texture || !dest_texture) {
						throw std::runtime_error("Resource lost");
					}

					auto src_tex_desc = src_texture->GetTextureDescription();
					auto dest_tex_desc = dest_texture->GetTextureDescription();

					if (!src_tex_desc || !dest_tex_desc) {
						throw std::invalid_argument("Both resources must be textures");
					}

					vk::ImageAspectFlags src_aspects = src_texture->GetAspectFlags();
					vk::ImageAspectFlags dest_aspects = dest_texture->GetAspectFlags();

					// Ensure source and destination aspects are compatible
					if ((src_aspects & (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) !=
						(dest_aspects & (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil))) {
						throw std::invalid_argument("Source and destination depth-stencil aspects must match");
					}

					bool has_depth = (src_aspects & vk::ImageAspectFlagBits::eDepth) != vk::ImageAspectFlagBits::eNone;
					bool has_stencil = (src_aspects & vk::ImageAspectFlagBits::eStencil) != vk::ImageAspectFlagBits::eNone;

					vk::ImageSubresourceLayers src_base_subresource;
					src_base_subresource.mipLevel = static_cast<std::uint32_t>(copy_cmd->src_subresource.mip_level);
					src_base_subresource.baseArrayLayer = static_cast<std::uint32_t>(copy_cmd->src_subresource.array_layer);
					src_base_subresource.layerCount = static_cast<std::uint32_t>(copy_cmd->src_subresource.layer_count);

					vk::ImageSubresourceLayers dest_base_subresource;
					dest_base_subresource.mipLevel = static_cast<std::uint32_t>(copy_cmd->dest_subresource.mip_level);
					dest_base_subresource.baseArrayLayer = static_cast<std::uint32_t>(copy_cmd->dest_subresource.array_layer);
					dest_base_subresource.layerCount = static_cast<std::uint32_t>(copy_cmd->dest_subresource.layer_count);

					vk::Offset3D src_offset(
						static_cast<std::int32_t>(copy_cmd->src_offset.x),
						static_cast<std::int32_t>(copy_cmd->src_offset.y),
						static_cast<std::int32_t>(copy_cmd->src_offset.z)
					);

					vk::Offset3D dest_offset(
						static_cast<std::int32_t>(copy_cmd->dest_offset.x),
						static_cast<std::int32_t>(copy_cmd->dest_offset.y),
						static_cast<std::int32_t>(copy_cmd->dest_offset.z)
					);

					vk::Extent3D extent(
						static_cast<std::uint32_t>(copy_cmd->extent.width),
						static_cast<std::uint32_t>(copy_cmd->extent.height),
						static_cast<std::uint32_t>(copy_cmd->extent.depth)
					);

					vk::Extent3D actual_extent = extent;
					if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
						actual_extent = GetMipExtent(src_tex_desc->extent, src_base_subresource.mipLevel);
					}

					SmallVector<vk::ImageMemoryBarrier2, 2u> texture_barriers;
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferWrite,
						vk::ImageLayout::eTransferDstOptimal,
						dest_texture,
						texture_barriers
					);
					AddBarrier(
						vk::PipelineStageFlagBits2::eTransfer,
						vk::AccessFlagBits2::eTransferRead,
						vk::ImageLayout::eTransferSrcOptimal,
						src_texture,
						texture_barriers
					);

					if (!texture_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, {}, texture_barriers);
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}

					auto DoCopy = [&](vk::ImageAspectFlagBits aspect) {
						vk::ImageSubresourceLayers src_sub = src_base_subresource;
						src_sub.aspectMask = aspect;
				
						vk::ImageSubresourceLayers dest_sub = dest_base_subresource;
						dest_sub.aspectMask = aspect;
				
						vk::ImageCopy region(
							src_sub,
							src_offset,
							dest_sub,
							dest_offset,
							actual_extent
						);
				
						m_impl->copyImage(
							src_texture->GetNative().texture,
							vk::ImageLayout::eTransferSrcOptimal,
							dest_texture->GetNative().texture,
							vk::ImageLayout::eTransferDstOptimal,
							1u,
							&region, 
							*m_dispatcher
						);
						};
				
					if (has_depth && has_stencil) {
						DoCopy(vk::ImageAspectFlagBits::eDepth);
						DoCopy(vk::ImageAspectFlagBits::eStencil);
					} 
					else if (has_depth) {
						DoCopy(vk::ImageAspectFlagBits::eDepth);
					} 
					else if (has_stencil) {
						DoCopy(vk::ImageAspectFlagBits::eStencil);
					} 
					else {
						DoCopy(vk::ImageAspectFlagBits::eColor);
					}
				
					co_return;
				},
				[this, &state_machine](CmdExecuteIndirect const* execute_indirect) -> GPUTask {

					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);

					if (!execute_indirect->id) {
						throw std::invalid_argument("Invalid indirect argument buffer");
					}
					VulkanResource* arg_buffer = plastic::utility::QueryObject<VulkanResource>(*execute_indirect->id);
					if (!arg_buffer) {
						throw std::runtime_error("Argument buffer lost");
					}
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;
					AddBarrier(
						vk::PipelineStageFlagBits2::eDrawIndirect,
						vk::AccessFlagBits2::eIndirectCommandRead,
						arg_buffer,
						buffer_barriers
					);
					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, {});
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}
					m_impl->drawIndirect(
						arg_buffer->GetNative().buffer,
						static_cast<vk::DeviceSize>(execute_indirect->offset_into_buffer),
						static_cast<uint32_t>(execute_indirect->draws),
						sizeof(vk::DrawIndirectCommand), 
						*m_dispatcher
					);
					co_return;
				},
				[this, &state_machine](CmdExecuteIndirectIndexed const* execute_indirect_indexed) -> GPUTask {

					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);

					if (!execute_indirect_indexed->id) {
						throw std::invalid_argument("Invalid indirect argument buffer");
					}
					VulkanResource* arg_buffer = plastic::utility::QueryObject<VulkanResource>(*execute_indirect_indexed->id);
					if (!arg_buffer) {
						throw std::runtime_error("Argument buffer lost");
					}
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;
					AddBarrier(
						vk::PipelineStageFlagBits2::eDrawIndirect,
						vk::AccessFlagBits2::eIndirectCommandRead,
						arg_buffer,
						buffer_barriers
					);
					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, {});
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}
					m_impl->drawIndexedIndirect(
						arg_buffer->GetNative().buffer,
						static_cast<vk::DeviceSize>(execute_indirect_indexed->offset_into_buffer),
						static_cast<uint32_t>(execute_indirect_indexed->draws),
						sizeof(vk::DrawIndexedIndirectCommand), 
						*m_dispatcher
					);
					co_return;
				},
				[this, &state_machine](CmdDispatchIndirect const* dispatch_indirect) -> GPUTask {

					co_await state_machine.WaitForContext(vk::PipelineBindPoint::eGraphics);

					if (!dispatch_indirect->id) {
						throw std::invalid_argument("Invalid indirect argument buffer");
					}
					VulkanResource* arg_buffer = plastic::utility::QueryObject<VulkanResource>(*dispatch_indirect->id);
					if (!arg_buffer) {
						throw std::runtime_error("Argument buffer lost");
					}
					SmallVector<vk::BufferMemoryBarrier2, 1u> buffer_barriers;
					AddBarrier(
						vk::PipelineStageFlagBits2::eDrawIndirect,
						vk::AccessFlagBits2::eIndirectCommandRead,
						arg_buffer,
						buffer_barriers
					);
					if (!buffer_barriers.empty()) {
						vk::DependencyInfo dep_info({}, {}, buffer_barriers, {});
						m_impl->pipelineBarrier2(dep_info, *m_dispatcher);
					}
					m_impl->dispatchIndirect(
						arg_buffer->GetNative().buffer,
						static_cast<vk::DeviceSize>(dispatch_indirect->offset_into_buffer), 
						*m_dispatcher
					);
					co_return;
				},
				[](auto const*) -> GPUTask {
					co_return;
				}
			};

			for (auto const& cmd : m_commands) {
				cmd->Visit(visitor);
			}

			state_machine.Finalize();

			m_impl->end(*m_dispatcher);

		}
	}

	void VulkanCommandBuffer::Reset() {
		m_sync.ThrowIfNotOwned();
		m_commands.clear();
	}

	VulkanCommandBuffer& VulkanCommandBuffer::SetFrame(VulkanResource const& frame, VulkanView const& frame_view) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetFrame>();
		command->texture_id = frame.GetID();
		command->view_id = frame_view.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::BeginRendering(std::span<VulkanResource const* const> rts, std::span<VulkanView const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears) {
		
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

	VulkanCommandBuffer& VulkanCommandBuffer::BeginRendering(std::span<VulkanResource const* const> rts, std::span<VulkanView const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears, VulkanResource const& ds, VulkanView const& dsv, LoadOp depth_load, StoreOp depth_store, float depth_clear, LoadOp stencil_load, StoreOp stencil_store, std::uint8_t stencil_clear) {
		
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

	VulkanCommandBuffer& VulkanCommandBuffer::EndRendering() {
		m_sync.ThrowIfNotOwned();
		m_commands.emplace_back(plastic::utility::MakeUnique<CmdEndRendering>());
		return *this;
	}

	
	VulkanCommandBuffer& VulkanCommandBuffer::SetViewports(std::span<CmdSetViewports::Viewport const> viewports) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetViewports>();

		command->viewports.assign(viewports.begin(), viewports.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::SetScissors(std::span<CmdSetScissors::Scissor const> scissors) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetScissors>();

		command->scissors.assign(scissors.begin(), scissors.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::Execute(VulkanGPUExecutable const& gpu_exec) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdExecute>();

		command->gpu_exec_id = gpu_exec.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::DispatchComputingTask(std::size_t threads_x, std::size_t threads_y, std::size_t threads_z, std::size_t threads_per_thread_group_x, std::size_t threads_per_thread_group_y, std::size_t threads_per_thread_group_z) {
		
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

	
	VulkanCommandBuffer& VulkanCommandBuffer::BindVertexBuffers(std::span<VulkanResource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindVertexBuffers>();
		
		std::size_t binding_count = (std::min)({ vertex_buffers.size(), wheres.size(), offsets.size(), strides.size() });
		for (std::size_t i = 0; i < binding_count; ++i) {
			command->bindings.emplace_back(vertex_buffers[i]->GetID(), wheres[i], offsets[i], strides[i]);
		}

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::DrawInstanced(std::size_t vertices, std::size_t instances, std::size_t start_vertex, std::size_t first_instance) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDrawInstanced>();
	
		command->vertices = vertices;
		command->instances = instances;
		command->start_vertex = start_vertex;
		command->first_instance = first_instance;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::DrawIndexed(std::size_t indices, std::size_t instances, std::size_t first_index, std::size_t start_vertex, std::size_t first_instance) {
		
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

	VulkanCommandBuffer& VulkanCommandBuffer::BindBuffer(
		ShaderStage stages,
		std::size_t where,
		VulkanResource const& buffer,
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

	VulkanCommandBuffer &VulkanCommandBuffer::BindBuffer(ShaderStage stages, std::size_t where, VulkanResource const& buffer, VulkanView const& texel_view, std::size_t offset, std::size_t ns) {
		
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

	VulkanCommandBuffer& VulkanCommandBuffer::PushData(ShaderStage stages, std::size_t where, std::span<std::byte const> bytes, std::size_t offset, std::size_t ns) {

		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdPushData>();

		command->where = where;
		command->ns = ns;
		command->offset = offset;
		command->bytes.assign(bytes.begin(), bytes.end());
		command->stages = stages;

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::BindSampler(ShaderStage stages, std::size_t where, VulkanSampler const& sampler, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindSampler>();

		command->id = sampler.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::BindTexture(ShaderStage stages, std::size_t where, VulkanResource const& texture, VulkanView const& view, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindTexture>();

		command->texture_id = texture.GetID();
		command->view_id = view.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	VulkanCommandBuffer& VulkanCommandBuffer::CopyTextureToBuffer(
		VulkanResource const& src_texture,
		VulkanResource const& dest_buffer,
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
	
	VulkanCommandBuffer& VulkanCommandBuffer::CopyBufferToTexture(
		VulkanResource const& src_buffer,
		VulkanResource const& dest_texture,
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
	
	VulkanCommandBuffer& VulkanCommandBuffer::CopyBufferToBuffer(
		VulkanResource const& src_buffer,
		VulkanResource const& dest_buffer,
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
	
	VulkanCommandBuffer& VulkanCommandBuffer::CopyTextureToTexture(
		VulkanResource const& src_texture,
		VulkanResource const& dest_texture,
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

	VulkanCommandBuffer& VulkanCommandBuffer::BindIndexBuffer(
		VulkanResource const& buffer
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdBindIndexBuffer>();
		cmd->id = buffer.GetID();
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	VulkanCommandBuffer& VulkanCommandBuffer::ExecuteIndirect(
		VulkanResource const& arg_buffer,
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

	VulkanCommandBuffer& VulkanCommandBuffer::ExecuteIndirectIndexed(
		VulkanResource const& arg_buffer,
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

	VulkanCommandBuffer& VulkanCommandBuffer::DispatchIndirect(
		VulkanResource const& arg_buffer,
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

	vk::CommandBuffer VulkanCommandBuffer::GetNative() const noexcept {
		return *m_impl;
	}

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)