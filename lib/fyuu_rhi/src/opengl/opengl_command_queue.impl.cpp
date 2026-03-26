/* opengl_command_queue.impl.cpp */
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
#include <variant>
#include <optional>
#include <coroutine>
#include <format>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
#include "declare_pool.hpp"
module fyuu_rhi:opengl_command_queue_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_command_queue;
import plastic.sealed_polymorphism;
import plastic.serial_task;
import :types;
import :opengl_logical_device;
import :opengl_common;
import :opengl_command_buffer;
import :opengl_shader_data_segment;
import :opengl_gpu_executable;
import :opengl_resource;
import :opengl_view;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::opengl;

	enum class WaitFor : std::uint8_t {
		Frame,
		RenderingStart,
		RenderingEnd,
		GraphicsProgram,
		ComputeProgram,
		IndexType,
		Count
	};

	enum class ContextType : std::uint8_t { 
		Graphics, 
		Compute, 
	};
	
	class StateMachine {
	private:
		OpenGLGPUExecutable* m_graphics_exec = nullptr;
		OpenGLGPUExecutable* m_compute_exec = nullptr;
		OpenGLResource* m_frame_texture = nullptr;
		OpenGLView* m_frame_view = nullptr;

		std::array<std::deque<std::coroutine_handle<>>, static_cast<std::size_t>(WaitFor::Count)> m_pending_queues;
		std::deque<GLuint> m_fbos;

		GLuint m_fbo;
		GLenum m_index_type;

		bool IsConditionSatisfied(WaitFor type) const {
			switch (type) {
			case WaitFor::Frame:				return m_frame_texture && m_frame_view;
			case WaitFor::RenderingStart:		return m_fbo;
			case WaitFor::RenderingEnd:			return !m_fbo;
			case WaitFor::GraphicsProgram:		return m_graphics_exec;
			case WaitFor::ComputeProgram:		return m_compute_exec;
			case WaitFor::IndexType:			return m_index_type;
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
		void BeginRendering(GLuint fbo) {
			m_fbo = fbo;
			ResumeIfReady(WaitFor::RenderingStart);
		}
	
		void EndRendering() {
			m_fbos.emplace_back(std::exchange(m_fbo, 0u));
			ResumeIfReady(WaitFor::RenderingEnd);
		}

		void SetFrame(OpenGLResource* frame_texture, OpenGLView* frame_view) {
			m_frame_texture = frame_texture;
			m_frame_view = frame_view;
			ResumeIfReady(WaitFor::Frame);
		}
	
		void SetGraphicsExecutable(OpenGLGPUExecutable* exec) {
			if (!m_fbo) throw std::logic_error("Graphics executable must be set inside a rendering instance");
			m_graphics_exec = exec;
			ResumeIfReady(WaitFor::GraphicsProgram);
		}
	
		void SetComputeExecutable(OpenGLGPUExecutable* exec) {
			if (m_fbo) throw std::logic_error("Compute executable cannot be set inside a rendering instance");
			m_compute_exec = exec;
			ResumeIfReady(WaitFor::ComputeProgram);
		}
	
		void SetIndexType(GLenum format) {
			m_index_type = format;
			ResumeIfReady(WaitFor::IndexType);
		}

		auto WaitForFrame() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_frame_texture && sm.m_frame_view; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::Frame); }
				std::pair<OpenGLResource*, OpenGLView*> await_resume() const {
					return { sm.m_frame_texture, sm.m_frame_view }; 
				}
			};
			return Awaiter{*this};
		}

		auto WaitForRenderingStart() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_fbo; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::RenderingStart); }
				void await_resume() const {}
			};
			return Awaiter{*this};
		}
	
		auto WaitForRenderingEnd() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return !sm.m_fbo; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::RenderingEnd); }
				void await_resume() const {}
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
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					switch (type) {
					case ContextType::Graphics:
						sm.AddPending(h, WaitFor::GraphicsProgram);
						break;
					case ContextType::Compute:
						sm.AddPending(h, WaitFor::ComputeProgram);
						break;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				OpenGLGPUExecutable* await_resume() const {
					switch (type) {
					case ContextType::Graphics:
						return sm.m_graphics_exec;
					case ContextType::Compute:
						return sm.m_compute_exec;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}			
				}
			};
			return Awaiter{*this, type};
		}

		auto WaitForIndexType() {
			struct Awaiter {
				StateMachine& sm;
				bool await_ready() const { return sm.m_index_type; }
				void await_suspend(std::coroutine_handle<> h) const { sm.AddPending(h, WaitFor::IndexType); }
				GLenum await_resume() const { return sm.m_index_type; }
			};
			return Awaiter{*this};
		}

		auto WaitForContext(ContextType type) {
			struct Awaiter {
				StateMachine& sm;
				ContextType type;
				bool await_ready() const {
					switch (type) {
					case ContextType::Graphics:
						return sm.m_fbo;
					case ContextType::Compute:
						return !sm.m_fbo;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_suspend(std::coroutine_handle<> h) const {
					switch (type) {
					case ContextType::Graphics:
						if (!sm.m_fbo) sm.AddPending(h, WaitFor::RenderingStart);
						break;
					case ContextType::Compute:
						if (sm.m_fbo) sm.AddPending(h, WaitFor::RenderingEnd);
						break;
					default:
						throw std::invalid_argument("Invalid pipline type");
					}
				}
				void await_resume() const {}
			};
			return Awaiter{*this, type};
		}
	
		std::deque<GLuint> Finalize() {
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
			return std::move(m_fbos);
		}

	};

	ContextType GetContextType(ShaderStage stages) {
        if (HasCrossGroupConflicts(stages, ShaderStage::Graphics, ShaderStage::Compute, ShaderStage::RayTracing)) {
            throw std::invalid_argument("Conflicting shader stage");
        }
        if ((stages & ShaderStage::Graphics) != ShaderStage::Unknown) {
            return ContextType::Graphics;
        }
        if ((stages & ShaderStage::Compute) != ShaderStage::Unknown) {
            return ContextType::Compute;
        }
        if ((stages & ShaderStage::RayTracing) != ShaderStage::Unknown) {
            throw std::invalid_argument("OpenGL does not support ray tracing");
        }
        throw std::invalid_argument("No shader stage specified");
    }

	decltype(auto) GetShaderSegmentAndBinding(
        OpenGLGPUExecutable* exec, 
        std::size_t ns, 
		std::size_t where
	) {
        auto seg_id = exec->GetAssociatedShaderDataSegmentID();
        if (!seg_id) {
			throw std::invalid_argument("Invalid shader data segment");
		}
        auto* seg = plastic::utility::QueryObject<OpenGLShaderDataSegment>(*seg_id);
        if (!seg) {
			throw std::runtime_error("Shader data segment lost");
		}
        auto&& [info, sampler] = seg->GetBindingInfo(ns, where);
        if (sampler) {
            throw std::logic_error(std::format("Binding (binding = {}) is immutable sampler", ns, where));
        }
        return std::make_pair(seg, info);
    }

}

namespace fyuu_rhi::opengl {
	
	OpenGLCommandQueue::OpenGLCommandQueue(
		OpenGLLogicalDevice const& logical_device,			
		CommandObjectType queue_type,
		QueuePriority priority
	) : PolymorphicCommandQueueBase(this),
		OpenGLCommon(this, logical_device),
		m_internal_promise(logical_device) {

	}

	void OpenGLCommandQueue::WaitUntilCompleted() {
		m_internal_promise.Signal();
		auto future = m_internal_promise.GetFuture();
		future->Wait();
	}

	void OpenGLCommandQueue::ExecuteCommand(OpenGLCommandBuffer& command_buffer, OpenGLPromise& promise, std::shared_ptr<OpenGLFuture> const& future) {
		
		future->Wait();

		MakeCurrent();

		StateMachine state_machine;

		using GPUTask = plastic::concurrency::SerialTask<void>; 

		auto visitor = plastic::utility::Overload{
			[&state_machine](CmdSetFrame const* set_frame) -> GPUTask {

				auto const& frame_id = set_frame->texture_id;
				if (!frame_id) {
					throw std::invalid_argument("Invalid frame");
				}

				OpenGLResource* frame = plastic::utility::QueryObject<OpenGLResource>(*frame_id);
				if (!frame) {
					throw std::runtime_error("Frame lost");
				}

				auto const& view_id = set_frame->view_id;
				if (!view_id) {
					throw std::invalid_argument("Invalid frame view");
				}

				OpenGLView* frame_view = plastic::utility::QueryObject<OpenGLView>(*view_id);
				if (!frame_view) {
					throw std::runtime_error("Frame view lost");
				}

				state_machine.SetFrame(frame, frame_view);

				co_return;

			},
			[this, &state_machine](CmdBeginRendering const* begin_rendering) -> GPUTask {

				std::vector<GLuint> color_tex_ids;

				std::size_t rt_descs_size = begin_rendering->render_targets.size();
				for (std::size_t i = 0; i < rt_descs_size; ++i) {
					auto const& rt_desc = begin_rendering->render_targets[i];
					auto const& rt_id = rt_desc.texture_id;
					if (!rt_id) {
						throw std::invalid_argument("Invalid render target");
					}

					OpenGLResource* rt = plastic::utility::QueryObject<OpenGLResource>(*rt_id);
					if (!rt) {
						throw std::runtime_error("Render target lost");
					}

					color_tex_ids.emplace_back(rt->GetNative());

				}

				GLuint depth_tex_id = 0;
				auto& ds_desc = begin_rendering->depth_stencil;
				if (ds_desc) {
					auto const& ds_id = ds_desc->texture_id;
					if (!ds_id) {
						throw std::invalid_argument("Invalid depth stencil");
					}

					OpenGLResource* ds = plastic::utility::QueryObject<OpenGLResource>(*ds_id);
					if (!ds) {
						throw std::runtime_error("Depth stencil lost");
					}
				}
					
				GLuint fbo = GetFramebuffer(color_tex_ids, depth_tex_id);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);

				for (std::size_t i = 0; i < rt_descs_size; ++i) {
					auto const& rt_desc = begin_rendering->render_targets[i];
					if (rt_desc.load == LoadOp::Clear) {
						auto const& clear = rt_desc.clear;
						glClearNamedFramebufferfv(fbo, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), 0, clear.data());
					}
				}

				if (ds_desc) {
					if (ds_desc->d_load == LoadOp::Clear) {
						glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &ds_desc->depth_clear);
					}
					if (ds_desc->s_load == LoadOp::Clear) {
						GLuint stencil_clear = ds_desc->stencil_clear;
						glClearNamedFramebufferuiv(fbo, GL_STENCIL, 0, &stencil_clear);
					}
				}

				state_machine.BeginRendering(fbo);
				co_return;

			},
			[&state_machine](CmdEndRendering const*) -> GPUTask {
				co_await state_machine.WaitForRenderingStart();
				state_machine.EndRendering();
				co_return;
			},
			[&state_machine](CmdSetViewports const* set_viewport) -> GPUTask {
				co_await state_machine.WaitForRenderingStart();
				std::vector<GLfloat> viewport_data;
				for (auto const& vp : set_viewport->viewports) {
					viewport_data.emplace_back(static_cast<GLfloat>(vp.x));
					viewport_data.emplace_back(static_cast<GLfloat>(vp.y));
					viewport_data.emplace_back(static_cast<GLfloat>(vp.width));
					viewport_data.emplace_back(static_cast<GLfloat>(vp.height));
				}
				if (!viewport_data.empty()) {
					glViewportArrayv(0, static_cast<GLsizei>(set_viewport->viewports.size()), viewport_data.data());
				}
				co_return;
			},
			[&state_machine](CmdSetScissors const* set_scissors) -> GPUTask {
				co_await state_machine.WaitForRenderingStart();
				std::vector<GLint> scissor_data;
				for (auto const& sc : set_scissors->scissors) {
					scissor_data.push_back(static_cast<GLint>(sc.x));
					scissor_data.push_back(static_cast<GLint>(sc.y));
					scissor_data.push_back(static_cast<GLint>(sc.width));
					scissor_data.push_back(static_cast<GLint>(sc.height));
				}
				if (!scissor_data.empty()) {
					glScissorArrayv(0, static_cast<GLsizei>(set_scissors->scissors.size()), scissor_data.data());
				}
				co_return;
			},
			[&state_machine](CmdExecute const* execute) -> GPUTask {
				
				auto& gpu_exec_id = execute->gpu_exec_id;
				if (!gpu_exec_id) {
					throw std::invalid_argument("Invalid GPU executable program");
				}

				OpenGLGPUExecutable* gpu_exec = plastic::utility::QueryObject<OpenGLGPUExecutable>(*gpu_exec_id);
				if (!gpu_exec) {
					throw std::runtime_error("GPU executable program lost");
				}

				auto&& [pso_type, pso] = gpu_exec->GetPSO();

				if ((pso_type & ShaderStage::Graphics) != ShaderStage::Unknown) {
					co_await state_machine.WaitForRenderingStart();
				}

				if ((pso_type & ShaderStage::Compute) != ShaderStage::Unknown) {
					co_await state_machine.WaitForRenderingEnd();
				}

				if ((pso_type & ShaderStage::RayTracing) != ShaderStage::Unknown) {
					co_await state_machine.WaitForRenderingEnd();
					throw std::invalid_argument("OpenGL dese not support ray tracing");
				}

				std::optional<std::size_t> shader_data_segment_id = gpu_exec->GetAssociatedShaderDataSegmentID();
				if (!shader_data_segment_id) {
					throw std::invalid_argument("Invalid shader data segment");
				}
				auto* shader_data_segment = plastic::utility::QueryObject<OpenGLShaderDataSegment>(*shader_data_segment_id);
				if (!shader_data_segment) {
					throw std::runtime_error("Shader data segment lost");
				}

				gpu_exec->Bind();

				auto immutable_samplers = shader_data_segment->GetImmutableSamplers();

				for (auto const& info : immutable_samplers) {
					glBindSampler(info.binding, info.impl->GetNative());
				}

				if ((pso_type & ShaderStage::Graphics) != ShaderStage::Unknown) {
					state_machine.SetGraphicsExecutable(gpu_exec);
				}

				if ((pso_type & ShaderStage::Compute) != ShaderStage::Unknown) {
					state_machine.SetComputeExecutable(gpu_exec);
				}

				co_return;

			},
			[&state_machine](CmdDispatchComputingTask const* dispatch_computing_task) -> GPUTask {
				co_await state_machine.WaitForContext(ContextType::Compute);
				glDispatchCompute(
					static_cast<GLuint>(dispatch_computing_task->threads_x),
					static_cast<GLuint>(dispatch_computing_task->threads_y),
					static_cast<GLuint>(dispatch_computing_task->threads_z)
				);
				co_return;
			},
			[&state_machine](CmdBindVertexBuffers const* bind_vertex_buffers) -> GPUTask {

				co_await state_machine.WaitForContext(ContextType::Graphics);

				for (auto const& binding : bind_vertex_buffers->bindings) {

					auto const& id = binding.id;
					if (!id) {
						throw std::invalid_argument("Invalid vertex buffer");
					}

					auto* buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
					if (!buffer) {
						throw std::runtime_error("Vertex buffer lost");
					}

					glBindVertexBuffer(
						static_cast<GLuint>(binding.where),
						buffer->GetNative(),
						static_cast<GLintptr>(binding.offset),
						static_cast<GLsizei>(binding.stride)
					);

				}

				co_return;
			},
			[&state_machine](CmdBindIndexBuffer const* bind_index_buffer) -> GPUTask {
				co_await state_machine.WaitForContext(ContextType::Graphics);
				
				auto const& id = bind_index_buffer->id;
				
				if (!id) {
					throw std::invalid_argument("Invalid index buffer");
				}

				auto* index_buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
				if (!index_buffer) {
					throw std::runtime_error("Index buffer lost");
				}

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->GetNative());

				auto format = index_buffer->GetInternalFormat();
				switch (format) {
				case GL_R16UI:
					state_machine.SetIndexType(GL_UNSIGNED_SHORT);
					break;
				case GL_R32UI:
					state_machine.SetIndexType(GL_UNSIGNED_INT);
					break;
				default:
					throw std::invalid_argument("Unknown index buffer format");
				}

				co_return;
				
			},
			[&state_machine](CmdDrawInstanced const* draw_instanced) -> GPUTask {
				OpenGLGPUExecutable* gpu_exec = co_await state_machine.WaitForExecutable(ContextType::Graphics);
				glDrawArraysInstanced(
					gpu_exec->GetTopology(),
					static_cast<GLint>(draw_instanced->start_vertex),
					static_cast<GLsizei>(draw_instanced->vertices),
					static_cast<GLsizei>(draw_instanced->instances)
				);
				co_return;
			},
			[&state_machine](CmdDrawIndexedInstanced const* draw_indexed_instanced) -> GPUTask {
				OpenGLGPUExecutable* gpu_exec = co_await state_machine.WaitForExecutable(ContextType::Graphics);
				GLenum index_type = co_await state_machine.WaitForIndexType();
				GLsizeiptr offset = static_cast<GLsizeiptr>(draw_indexed_instanced->first_index * (index_type == GL_UNSIGNED_SHORT ? 2 : 4));
				glDrawElementsInstanced(
					gpu_exec->GetTopology(),
					static_cast<GLsizei>(draw_indexed_instanced->indices),
					index_type,
					reinterpret_cast<void const*>(offset),
					static_cast<GLsizei>(draw_indexed_instanced->instances)
				);
				co_return;
			},
			[&state_machine](CmdBindBuffer const* bind_buffer) -> GPUTask {

				ContextType type = GetContextType(bind_buffer->stages);
				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(type);
				auto [_, info] = GetShaderSegmentAndBinding(exec, bind_buffer->ns, bind_buffer->where);

				GLenum target = std::visit(
					[info](auto&& flags) -> GLenum {
						using Flags = std::decay_t<decltype(flags)>;
						if constexpr (std::same_as<ResourceFlags, Flags>) {
							if (HasConflictingFlags(flags, ResourceFlags::Buffer)) {
								throw std::logic_error(
									std::format(
										"The binding (binding = {}) is invalid buffer", 
										info->where
									)
								);								
							}

							if ((flags & ResourceFlags::UniformTexelBuffer) != ResourceFlags::Unknown || 
								(flags & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown) {
								return 0;
							}

							if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown) {
								return GL_UNIFORM_BUFFER;
							}

							if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown) {
								return GL_SHADER_STORAGE_BUFFER;
							}

							throw std::runtime_error(
								std::format(
									"The binding (binding = {}) is an unknown",
									info->where
								)
							);

						}
						else {
							throw std::logic_error(
								std::format(
									"The binding (binding = {}) is not buffer", 
									info->where
								)
							);
						}
					},
					info->flags
				);

				auto const& id = bind_buffer->id;
				if (!id) {
					throw std::invalid_argument("Invalid argument buffer");
				}

				auto* buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
				if (!buffer) {
					throw std::runtime_error("Argument buffer lost");
				}

				if (target) {
					glBindBufferBase(target, info->where, buffer->GetNative());
					co_return;
				}

				auto const& view_id = bind_buffer->texel_view_id;
				if (!view_id) {
					throw std::invalid_argument("Invalid texel view");
				}

				OpenGLView* texel_view = plastic::utility::QueryObject<OpenGLView>(*view_id);
				if (!texel_view) {
					throw std::runtime_error("Texel view lost");
				}

				glBindTextureUnit(info->where, texel_view->GetNative());

				co_return;
			},
			[&state_machine](CmdPushData const* push_data) -> GPUTask {

				ContextType type = GetContextType(push_data->stages);
				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(type);
				auto [_, info] = GetShaderSegmentAndBinding(exec, push_data->ns, push_data->where);

				auto const& bytes = push_data->bytes;
				size_t size = bytes.size();
				if (size % 4 != 0) throw std::runtime_error("Push data must be 4-byte aligned");
	
				GLuint prog = exec->GetProgram(push_data->stages);

				void const* data = bytes.data();
				if (size == 4)
					glProgramUniform1fv(prog, info->where, 1, reinterpret_cast<GLfloat const*>(data));
				else if (size == 8)
					glProgramUniform2fv(prog, info->where, 1, reinterpret_cast<GLfloat const*>(data));
				else if (size == 12)
					glProgramUniform3fv(prog, info->where, 1, reinterpret_cast<GLfloat const*>(data));
				else if (size == 16)
					glProgramUniform4fv(prog, info->where, 1, reinterpret_cast<GLfloat const*>(data));
				else
					throw std::runtime_error("Unsupported push data size");

				co_return;

			},
			[&state_machine](CmdBindSampler const* bind_sampler) -> GPUTask {

				ContextType type = GetContextType(bind_sampler->stages);
				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(type);
				auto [_, info] = GetShaderSegmentAndBinding(exec, bind_sampler->ns, bind_sampler->where);

				auto& id = bind_sampler->id;
				if (!id) {
					throw std::invalid_argument("Invalid sampler");
				}

				auto sampler = plastic::utility::QueryObject<OpenGLSampler>(*id);
				if (!sampler) {
					throw std::runtime_error("Sampler lost");
				}

				glBindSampler(info->where, sampler->GetNative());

				co_return;
			},
			[&state_machine](CmdBindTexture const* bind_texture) -> GPUTask {

				ContextType type = GetContextType(bind_texture->stages);
				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(type);
				auto [_, info] = GetShaderSegmentAndBinding(exec, bind_texture->ns, bind_texture->where);

				auto& texture_id = bind_texture->texture_id;
				if (!texture_id) {
					throw std::invalid_argument("Invalid texture");
				}
		
				OpenGLResource* texture = plastic::utility::QueryObject<OpenGLResource>(*texture_id);
				if (!texture) {
					throw std::runtime_error("Texture lost");
				}

				auto& view_id = bind_texture->view_id;
				if (!view_id) {
					throw std::invalid_argument("Invalid texture view");
				}
		
				OpenGLView* view = plastic::utility::QueryObject<OpenGLView>(*view_id);
				if (!view) {
					throw std::runtime_error("Texture view lost");
				}

				glBindTextureUnit(info->where, view->GetNative());

				co_return;
			},
			[&state_machine](CmdCopyTextureToBuffer const* copy_cmd) -> GPUTask {
				co_await state_machine.WaitForRenderingEnd();

				auto src_id = copy_cmd->src_id;
				auto dest_id = copy_cmd->dest_id;
				if (!src_id || !dest_id) {
					throw std::invalid_argument("Invalid source or destination");
				}
			
				auto* src_tex = plastic::utility::QueryObject<OpenGLResource>(*src_id);
				auto* dest_buf = plastic::utility::QueryObject<OpenGLResource>(*dest_id);
				if (!src_tex || !dest_buf) {
					throw std::runtime_error("Resource lost");
				}
			
				auto [format, type] = src_tex->GetFormat();
			
				GLint level = static_cast<GLint>(copy_cmd->src_subresource.mip_level);
				GLint x = static_cast<GLint>(copy_cmd->src_offset.x);
				GLint y = static_cast<GLint>(copy_cmd->src_offset.y);
				GLint z = static_cast<GLint>(copy_cmd->src_offset.z);
				GLsizei width = static_cast<GLsizei>(copy_cmd->extent.width);
				GLsizei height = static_cast<GLsizei>(copy_cmd->extent.height);
				GLsizei depth = static_cast<GLsizei>(copy_cmd->extent.depth);
				if (width == 0 || height == 0 || depth == 0) {
					throw std::invalid_argument("Size cannot be zero"); 
				}
			
				std::optional<std::size_t> bytes_per_pixel = src_tex->GetBytesPerPixel();
				if (!bytes_per_pixel) {
					throw std::invalid_argument("Not texture or the format is compressed one");
				}
				std::size_t row_size = width * (*bytes_per_pixel);
				std::size_t layer_size = row_size * height;
				std::size_t total_size = layer_size * depth * copy_cmd->src_subresource.layer_count;
			
				std::vector<std::byte> temp(total_size);
				glGetTextureSubImage(
					src_tex->GetNative(), level, x, y, z, width, height, depth,
					format, type, static_cast<GLsizei>(total_size), temp.data()
				);
			
				glBindBuffer(GL_COPY_WRITE_BUFFER, dest_buf->GetNative());
				glBufferSubData(
					GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(copy_cmd->dest_offset),
					static_cast<GLsizeiptr>(total_size), temp.data()
				);
				glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
			
				co_return;
			},
			[&state_machine](CmdCopyBufferToTexture const* copy_cmd) -> GPUTask {
				co_await state_machine.WaitForRenderingEnd();

				auto src_id = copy_cmd->src_id;
				auto dest_id = copy_cmd->dest_id;
				if (!src_id || !dest_id) {
					throw std::invalid_argument("Invalid source or destination");
				}
			
				auto* src_buf = plastic::utility::QueryObject<OpenGLResource>(*src_id);
				auto* dest_tex = plastic::utility::QueryObject<OpenGLResource>(*dest_id);
				if (!src_buf || !dest_tex) {
					throw std::runtime_error("Resource lost");
				}
			
				GLenum internal_format = dest_tex->GetInternalFormat();
				auto [format, type] = dest_tex->GetFormat();
			
				GLint level = static_cast<GLint>(copy_cmd->dest_subresource.mip_level);
				GLint x = static_cast<GLint>(copy_cmd->dest_offset.x);
				GLint y = static_cast<GLint>(copy_cmd->dest_offset.y);
				GLint z = static_cast<GLint>(copy_cmd->dest_offset.z);
				GLsizei width = static_cast<GLsizei>(copy_cmd->extent.width);
				GLsizei height = static_cast<GLsizei>(copy_cmd->extent.height);
				GLsizei depth = static_cast<GLsizei>(copy_cmd->extent.depth);
				if (width == 0 || height == 0 || depth == 0) {
					throw std::invalid_argument("Size cannot be zero"); 
				}
			
				std::optional<std::size_t> bytes_per_pixel = dest_tex->GetBytesPerPixel();
				if (!bytes_per_pixel) {
					throw std::invalid_argument("Not texture or the format is compressed one");
				}
				std::size_t row_size = width * (*bytes_per_pixel);
				std::size_t layer_size = row_size * height;
				std::size_t total_size = layer_size * depth * copy_cmd->dest_subresource.layer_count;
			
				std::vector<std::byte> temp(total_size);
				glBindBuffer(GL_COPY_READ_BUFFER, src_buf->GetNative());
				glGetBufferSubData(
					GL_COPY_READ_BUFFER, static_cast<GLintptr>(copy_cmd->src_offset),
					static_cast<GLsizeiptr>(total_size), temp.data()
				);
				glBindBuffer(GL_COPY_READ_BUFFER, 0);
			
				GLenum target = dest_tex->GetTarget();
				if (target == GL_TEXTURE_1D) {
					glTextureSubImage1D(dest_tex->GetNative(), level, x, width, format, type, temp.data());
				} else if (target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE) {
					glTextureSubImage2D(dest_tex->GetNative(), level, x, y, width, height, format, type, temp.data());
				} else if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) {
					glTextureSubImage3D(dest_tex->GetNative(), level, x, y, z, width, height, depth, format, type, temp.data());
				} else {
					throw std::runtime_error("Unsupported texture target for copy");
				}
			
				co_return;
				
			},
			[&state_machine](CmdCopyBufferToBuffer const* copy_cmd) -> GPUTask {

				co_await state_machine.WaitForRenderingEnd();

				auto src_id = copy_cmd->src_id;
				auto dest_id = copy_cmd->dest_id;
				if (!src_id || !dest_id) {
					throw std::invalid_argument("Invalid source or destination");
				}
			
				auto* src_buffer = plastic::utility::QueryObject<OpenGLResource>(*src_id);
				auto* dest_buffer = plastic::utility::QueryObject<OpenGLResource>(*dest_id);
				if (!src_buffer || !dest_buffer) {
					throw std::runtime_error("Buffer lost");
				}
			
				glBindBuffer(GL_COPY_READ_BUFFER, src_buffer->GetNative());
				glBindBuffer(GL_COPY_WRITE_BUFFER, dest_buffer->GetNative());
				glCopyBufferSubData(
					GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
					static_cast<GLintptr>(copy_cmd->src_offset),
					static_cast<GLintptr>(copy_cmd->dest_offset),
					static_cast<GLsizeiptr>(copy_cmd->size)
				);
				glBindBuffer(GL_COPY_READ_BUFFER, 0);
				glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
			
				co_return;

			},
			[&state_machine](CmdCopyTextureToTexture const* copy_cmd) -> GPUTask {		
				co_await state_machine.WaitForRenderingEnd();

				auto src_id = copy_cmd->src_id;
				auto dest_id = copy_cmd->dest_id;
				if (!src_id || !dest_id) {
					throw std::invalid_argument("Invalid source or destination");
				}
			
				auto* src_tex = plastic::utility::QueryObject<OpenGLResource>(*src_id);
				auto* dest_tex = plastic::utility::QueryObject<OpenGLResource>(*dest_id);
				if (!src_tex || !dest_tex) {
					throw std::runtime_error("Texture lost");
				}
			
				GLuint src_name = src_tex->GetNative();
				GLuint dst_name = dest_tex->GetNative();
				GLenum src_target = src_tex->GetTarget();
				GLenum dst_target = dest_tex->GetTarget();
			
				GLint srcLevel = static_cast<GLint>(copy_cmd->src_subresource.mip_level);
				GLint srcX = static_cast<GLint>(copy_cmd->src_offset.x);
				GLint srcY = static_cast<GLint>(copy_cmd->src_offset.y);
				GLint srcZ = static_cast<GLint>(copy_cmd->src_offset.z);
				GLint dstLevel = static_cast<GLint>(copy_cmd->dest_subresource.mip_level);
				GLint dstX = static_cast<GLint>(copy_cmd->dest_offset.x);
				GLint dstY = static_cast<GLint>(copy_cmd->dest_offset.y);
				GLint dstZ = static_cast<GLint>(copy_cmd->dest_offset.z);
				GLsizei width = static_cast<GLsizei>(copy_cmd->extent.width);
				GLsizei height = static_cast<GLsizei>(copy_cmd->extent.height);
				GLsizei depth = static_cast<GLsizei>(copy_cmd->extent.depth);
			
				if (width == 0 || height == 0 || depth == 0) {
					throw std::invalid_argument("Size cannot be zero"); 
				}
			
				glCopyImageSubData(
					src_name, src_target, srcLevel, srcX, srcY, srcZ,
					dst_name, dst_target, dstLevel, dstX, dstY, dstZ,
					width, height, depth
				);
			
				co_return;
			},
			[&state_machine](CmdExecuteIndirect const* execute_indirect) -> GPUTask {
				
				co_await state_machine.WaitForContext(ContextType::Graphics);

				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(ContextType::Graphics);
				GLenum mode = exec->GetTopology();
			
				auto const& id = execute_indirect->id;
				if (!id) {
					throw std::invalid_argument("Invalid argument buffer");
				}
			
				auto* buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
				if (!buffer) {
					throw std::runtime_error("Argument buffer lost");
				}
			
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer->GetNative());
				glDrawArraysIndirect(mode, reinterpret_cast<void*>(execute_indirect->offset_into_buffer));
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
			
				co_return;

			},
			[&state_machine](CmdExecuteIndirectIndexed const* execute_indirect_indexed) -> GPUTask {
				
				co_await state_machine.WaitForContext(ContextType::Graphics);

				OpenGLGPUExecutable* exec = co_await state_machine.WaitForExecutable(ContextType::Graphics);
				GLenum mode = exec->GetTopology();
				GLenum index_type = co_await state_machine.WaitForIndexType();
			
				auto const& id = execute_indirect_indexed->id;
				if (!id) {
					throw std::invalid_argument("Invalid argument buffer");
				}
			
				auto* buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
				if (!buffer) {
					throw std::runtime_error("Argument buffer lost");
				}
			
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer->GetNative());
				glDrawElementsIndirect(mode, index_type, reinterpret_cast<void*>(execute_indirect_indexed->offset_into_buffer));
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
			
				co_return;

			},
			[&state_machine](CmdDispatchIndirect const* dispatch_indirect) -> GPUTask {
				co_await state_machine.WaitForContext(ContextType::Compute);

				auto const& id = dispatch_indirect->id;
				if (!id) {
					throw std::invalid_argument("Invalid argument buffer");
				}
			
				auto* buffer = plastic::utility::QueryObject<OpenGLResource>(*id);
				if (!buffer) {
					throw std::runtime_error("Argument buffer lost");
				}
			
				glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer->GetNative());
				glDispatchComputeIndirect(static_cast<GLintptr>(dispatch_indirect->offset_into_buffer));
				glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);

			},
			[](auto const*) -> GPUTask {
				co_return;
			}
		};

		std::span<UniqueCommand const> commands = command_buffer.GetCommands();
		for (auto const& command : commands) {
			command->Visit(visitor);
		}

		m_fbos = state_machine.Finalize();

		promise.Signal();
		
	}

	std::shared_ptr<OpenGLFuture> OpenGLCommandQueue::ExecuteCommand(OpenGLCommandBuffer& command_buffer, std::shared_ptr<OpenGLFuture> const& future) {
		ExecuteCommand(command_buffer, m_internal_promise, future);
		return m_internal_promise.GetFuture();
	}

	std::deque<GLuint> OpenGLCommandQueue::AcquirePendingPresent() noexcept {
		return std::move(m_fbos);
	}

} // namespace fyuu_rhi::opengl

#endif