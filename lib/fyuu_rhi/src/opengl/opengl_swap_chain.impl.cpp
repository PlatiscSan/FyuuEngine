/* opengl_swap_chain.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <system_error>
#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>
#include <deque>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
module fyuu_rhi:opengl_swap_chain_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_swap_chain;
import :types;
import :opengl_common;
import :opengl_physical_device;
import :opengl_logical_device;
import :opengl_command_queue;
import :opengl_surface;

namespace {
	template <class T, class... Ts>
	bool AllSame(T&& first, Ts&&... rest) {
		return ((first == rest) && ...);
	}
}

namespace fyuu_rhi::opengl {

	OpenGLSwapChain::Frame::Frame(OpenGLLogicalDevice const& logical_device, std::size_t width, std::size_t height, ResourceFlags resource_flags)
		: texture(logical_device, { width, height, 1u }, VideoMemoryType::DeviceLocal, resource_flags),
		view(logical_device, texture, 0u, 1u, 0u, 1u),
		future(nullptr) {
	}

	OpenGLSwapChain::Frame::Frame(Frame&& other) noexcept
		: texture(std::move(other.texture)),
		view(std::move(other.view)),
		future(std::move(other.future)) {
	}

	OpenGLSwapChain::Frame::~Frame() noexcept {
		future->Wait();
	}

	void OpenGLSwapChain::SwapBuffer() {
		auto context = GetContext();
#if defined(_WIN32)
		if (!wglSwapLayerBuffers(context->device_context, WGL_SWAP_MAIN_PLANE)) {
			DWORD ec = GetLastError();
			throw std::system_error(ec, std::system_category(), "wglSwapLayerBuffers failed");
		}
#elif defined(__linux__)
		glXSwapBuffers(context->display, context->drawable);
#elif defined(__ANDROID__)
		if (!eglSwapBuffers(context->display, context->surface)) {
			throw std::runtime_error("eglSwapBuffers failed");
		}
#endif

	}

	void OpenGLSwapChain::Resize(
		OpenGLLogicalDevice const& logical_device,
		OpenGLSurface const& surface,
		std::optional<std::size_t> const& back_buffer_size
	) {
		
		auto [width, height] = surface.GetSize();

		m_frames.clear();
		if (back_buffer_size) {
			std::size_t new_size = *back_buffer_size;
			for (std::size_t i = 0; i < new_size; ++i) {
				m_frames.emplace_back(logical_device, width, height, m_swap_chain_option.back_buffer_flags);
			}
			m_swap_chain_option.back_buffer_size = new_size;
		}
		else {
			for (std::size_t i = 0; i < m_swap_chain_option.back_buffer_size; ++i) {
				m_frames.emplace_back(logical_device, width, height, m_swap_chain_option.back_buffer_flags);
			}
		}

		m_frame_index = 0u;

	}

	OpenGLSwapChain::OpenGLSwapChain(
		OpenGLPhysicalDevice const& physical_device,
		OpenGLLogicalDevice const& logical_device,
		OpenGLCommandQueue const& present_queue,
		OpenGLSurface const& surface,
		SwapChainOption const& swap_chain_option
	) : PolymorphicSwapChainBase(this),
		OpenGLCommon(
			[this, &physical_device, &logical_device, &present_queue, &surface]() {
				bool same = AllSame(physical_device.GetContext(), logical_device.GetContext(), present_queue.GetContext(), surface.GetContext());
				if (!same) {
					throw std::invalid_argument("various GL context");
				}
				return OpenGLCommon<OpenGLSwapChain>(this, physical_device);
			}()),
		m_logical_device_id(logical_device.GetID()),
		m_present_queue_id(present_queue.GetID()),
		m_surface_id(surface.GetID()),
		m_swap_chain_option(swap_chain_option),
		m_internal_promise(logical_device),
		m_frames(),
		m_frame_index(0u) {
		Resize(logical_device, surface);
	}

	OpenGLSwapChain::~OpenGLSwapChain() noexcept {

		m_internal_promise.Signal();
		auto future = m_internal_promise.GetFuture();
		future->Wait();

	}

	void OpenGLSwapChain::Resize(std::optional<std::size_t> const& back_buffer_size) {

#define CHECK_GET_OPENGL_OBJECT(id, Type, var, name)\
		if (!id) throw std::runtime_error("Invalid " name);\
		Type* var = plastic::utility::QueryObject<Type>(*id);\
		if (!var) throw std::invalid_argument(name " lost");

		CHECK_GET_OPENGL_OBJECT(m_logical_device_id, OpenGLLogicalDevice, logical_device, "logical device")
		CHECK_GET_OPENGL_OBJECT(m_surface_id, OpenGLSurface, surface, "surface")

		m_internal_promise.Signal();
		auto future = m_internal_promise.GetFuture();
		future->Wait();

		Resize(*logical_device, *surface, back_buffer_size);
		
	}

	std::tuple<OpenGLResource*, OpenGLView*, std::shared_ptr<OpenGLFuture>> OpenGLSwapChain::GetNextFrame() {
		Frame& frame = m_frames[m_frame_index];
		return { &frame.texture, &frame.view, frame.future };
	}

	void OpenGLSwapChain::Present(std::shared_ptr<OpenGLFuture> const& future) {

		MakeCurrent();

		Frame& frame = m_frames[m_frame_index];

		CHECK_GET_OPENGL_OBJECT(m_surface_id, OpenGLSurface, surface, "surface")
		CHECK_GET_OPENGL_OBJECT(m_present_queue_id, OpenGLCommandQueue, present_queue, "present queue")

		auto [width, height] = surface->GetSize();

		if (future) {
			future->Wait();
		}

		std::deque<GLuint> pending_fbos = present_queue->AcquirePendingPresent();
		
		for (GLuint fbo : pending_fbos) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(
				0, 0, static_cast<GLint>(width), static_cast<GLint>(height),
				0, 0, static_cast<GLint>(width), static_cast<GLint>(height),
				GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		SwapBuffer();

		m_internal_promise.Signal();
		frame.future = m_internal_promise.GetFuture();

		m_frame_index = (m_frame_index + 1) % m_swap_chain_option.back_buffer_size;

	}

	void OpenGLSwapChain::SetVSync(bool mode) {

		auto context = GetContext();

#if defined(_WIN32)
		if (wglSwapIntervalEXT) {
			wglSwapIntervalEXT(mode ? 1 : 0);
		}
#elif defined(__linux__)
		if (glXSwapIntervalEXT) {
			glXSwapIntervalEXT(context->display, context->drawable, mode ? 1 : 0);
		}
#elif defined(__ANDROID__)
		eglSwapInterval(context->display, mode ? 1 : 0);
#endif
		m_swap_chain_option.enable_v_sync = mode;
	}



} // namespace fyuu_rhi::opengl


#endif // !defined(__APPLE__)