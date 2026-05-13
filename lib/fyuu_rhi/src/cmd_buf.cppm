module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <vector>
#include <string>
#include <deque>
#include <array>
#include <atomic>
#include <mutex>
#include <variant>
#include <optional>
#include <string_view>
#include <concepts>
#include <span>
#include <coroutine>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:command_buffer;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :command_types;
import :resource;
import :view;
import :gpu_executable;
import :shader_types;
import :sampler;

namespace fyuu_rhi {
	export template <class Backend> class CommandBuffer {
	public:
		using Implementation = typename Backend::CommandBuffer;

		struct ColorAttachment {
			View<Backend> view;
			LoadOp load_op;
			StoreOp store_op;
			Color clearColor;
		};

		struct DepthAttachment {
			View<Backend> view;
			LoadOp load_op;
			StoreOp store_op;
			float clear_depth = 1.0f;
		};

		struct StencilAttachment {
			View<Backend> view;
			LoadOp load_op;
			StoreOp store_op;
			std::uint8_t clear_stencil = 0;
		};

		class Barrier {
		private:
		};

		class RenderPass {
		private:
			std::string m_name;
			std::vector<std::string> m_dependency_passes;
			GPUExecutable<Backend> m_gpu_exec;
			std::vector<ColorAttachment> m_color_atta;
			std::optional<DepthAttachment> m_depth_atta;
			std::optional<StencilAttachment> m_stencil_atta;
			std::vector<Viewport> m_vp;
			std::vector<Rect2D> m_scissors;
			std::vector<BindingDescription> m_bindings;
			std::optional<Buffer<Backend>> m_idx_buf;
			std::optional<CMDDrawInstanced> m_draw_inst;


		public:
			RenderPass(std::string_view name) : m_name(name) {}

			RenderPass& DependsOn(std::string_view pass_name) {
				m_dependency_passes.emplace_back(pass_name);
				return *this;
			}

			RenderPass& AddColorAttachment(
				View<Backend> const& view,
				LoadOp load_op,
				StoreOp store_op,
				Color const& clearColor = Color{}
			) {
				m_color_atta.push_back({view, load_op, store_op, clearColor});
				return *this;
			}

			RenderPass& SetDepthAttachment(
				View<Backend> const& view,
				LoadOp load_op,
				StoreOp store_op,
				float clearDepth = 1.0f
			) {
				m_depth_atta.emplace{view, load_op, store_op, clearDepth};
				return *this;
			}

			RenderPass& SetStencilAttachment(const View<Backend>& view,
				LoadOp load_op,
				StoreOp store_op,
				std::uint8_t clear_stencil = 0
			) {
				m_stencil_atta.emplace{view, load_op, store_op, clear_stencil};
				return *this;
			}

			RenderPass& SetViewports(std::span<Viewport const> viewports) {
				m_vp.assign(viewports.begin(), viewports.end());
				return *this;
			}

			RenderPass& SetScissors(std::span<Rect2D const> scissors) {
				m_scissors.assign(scissors.begin(), scissors.end());
				return *this;
			}

			RenderPass& Execute(GPUExecutable<Backend> const& gpu_exec) {
				m_gpu_exec = gpu_exec;
				return *this;
			}

			RenderPass& BindVertexBuffers(
				std::span<Resource<Backend> const> buffers,
				std::span<std::size_t const> slots,
				std::span<std::size_t const> offsets,
				std::span<std::size_t const> strides
			) {
				std::size_t binding_count = (std::min)({ buffers.size(), slots.size(), offsets.size(), strides.size() });
				for (std::size_t i = 0; i < binding_count; ++i) {
					m_bindings.emplace_back(buffers[i], slots[i], offsets[i], strides[i]);
				}
				return *this;
			}

			RenderPassBuilder& BindIndexBuffer(Resource<Backend> const& buffer) {
				m_idx_buf.emplace(buffer);
				return *this;
			}


		};


		class ComputeTask {
		private:
			std::string m_name;
			std::vector<std::string> m_dependency_tasks;

		public:
			ComputeTask(std::string_view name) : m_name(name) {}

			ComputeTask& DependsOn(std::string_view task_name) {
				m_dependency_tasks.emplace_back(task_name);
				return *this;
			}
		};

		using Command = std::variant<std::monostate, Barrier, RenderPass, ComputeTask>;

	private:
		Implementation m_impl;
		std::vector<Command> m_cmds;
		std::mutex m_cmds_m;

	public:
		class Recorder {
		private:
			CommandBuffer* m_cmd_buffer;
			std::unique_lock<std::mutex> m_lk;
		
		public:
			Recorder(CommandBuffer* cmd_buffer, std::unique_lock<std::mutex>&& lk) noexcept
				: m_cmd_buffer(cmd_buffer), m_lk(std::move(lk)) {}

			RenderPass BeginRenderPass(std::string_view name) {
				m_cmd_buffer->m_cmds.emplace_back(name);
				return m_cmd_buffer->m_cmds.back();
			}

			ComputeTask BeginComputeTask(std::string_view name) {
				m_cmd_buffer->m_cmds.emplace_back(name);
				return m_cmd_buffer->m_cmds.back();
			}

		};

		template <std::convertible_to<Implementation> I>
		CommandBuffer(I&& impl)
			: m_impl(std::forward<I>(impl)),
			m_cmds(),
			m_cmds_m() {

		}

		Recorder Record() {
			std::unique_lock<std::mutex> lk(m_cmds_m);
			Backend::ResetCommandBuffer(m_impl);
			m_cmds.clear();
			return { this, std::move(lk) };
		}

		void Submit() {


		}

	};
}