/* opengl_swap_chain.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <vector>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
export module fyuu_rhi:opengl_swap_chain;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :opengl_common;
import :opengl_future;
import :opengl_view;

namespace fyuu_rhi::opengl {
	
	export class OpenGLSwapChain
		: public PolymorphicSwapChainBase,
		public OpenGLCommon<OpenGLSwapChain> {
	private:
		struct Frame {
			OpenGLResource texture;
			OpenGLView view;
			std::shared_ptr<OpenGLFuture> future;
			Frame(OpenGLLogicalDevice const& logical_device, std::size_t width, std::size_t height, ResourceFlags resource_flags);
			Frame(Frame const&) noexcept = default;
			Frame(Frame&& other) noexcept;
			~Frame() noexcept;
		};
		
		std::optional<std::size_t> m_logical_device_id;
		std::optional<std::size_t> m_present_queue_id;
		std::optional<std::size_t> m_surface_id;
		SwapChainOption m_swap_chain_option;
		OpenGLPromise m_internal_promise;
		std::vector<Frame> m_frames;
		std::size_t m_frame_index;

		void SwapBuffer();

		void Resize(
			OpenGLLogicalDevice const& logical_device,
			OpenGLSurface const& surface,
			std::optional<std::size_t> const& back_buffer_size = std::nullopt
		);

	public:
		OpenGLSwapChain(
			OpenGLPhysicalDevice const& physical_device,
			OpenGLLogicalDevice const& logical_device,
			OpenGLCommandQueue const& present_queue,
			OpenGLSurface const& surface,
			SwapChainOption const& swap_chain_option
		);

		OpenGLSwapChain(OpenGLSwapChain&& other) = default;

		~OpenGLSwapChain() noexcept;

		void Resize(std::optional<std::size_t> const& back_buffer_size = std::nullopt);

		std::tuple<OpenGLResource*, OpenGLView*, std::shared_ptr<OpenGLFuture>> GetNextFrame();

		void Present(std::shared_ptr<OpenGLFuture> const& future = nullptr);

		void SetVSync(bool mode);

	};


} // namespace fyuu_rhi::opengl


#endif // !defined(__APPLE__)