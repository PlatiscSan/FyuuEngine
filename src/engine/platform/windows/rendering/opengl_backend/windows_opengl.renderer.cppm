module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <glad/glad.h>
#ifdef _WIN32
#include <glad/glad_wgl.h>
#endif // _WIN32

export module windows_opengl:renderer;
export import opengl_backend;
export import windows_window;
import std;

namespace fyuu_engine::windows::opengl {
#ifdef _WIN32
	export class WindowsOpenGLRenderer
		: public fyuu_engine::opengl::BaseOpenGLRenderer<WindowsOpenGLRenderer> {
		friend class fyuu_engine::opengl::BaseOpenGLRenderer<WindowsOpenGLRenderer>;
	private:
		static thread_local inline std::atomic<WindowsOpenGLRenderer*> s_current_context;

		HDC m_device_context;
		HGLRC m_gl_rendering_context;
		util::PointerWrapper<WindowsWindow> m_window;
		boost::uuids::uuid m_msg_processor_uuid;

		void MakeCurrent();
		void SwapBuffer();

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

		std::pair<std::uint32_t, std::uint32_t> GetTargetWindowWidthAndHeight() const noexcept;

	public:
		using Base = fyuu_engine::opengl::BaseOpenGLRenderer<WindowsOpenGLRenderer>;
		WindowsOpenGLRenderer(
			core::LoggerPtr const& logger,
			HDC device_context,
			HGLRC gl_rendering_context,
			util::PointerWrapper<WindowsWindow> const& window
		);

	};

	export class WindowsOpenGLRendererBuilder
		: public fyuu_engine::opengl::BaseOpenGLRendererBuilder<WindowsOpenGLRendererBuilder, WindowsOpenGLRenderer> {
		friend class fyuu_engine::opengl::BaseOpenGLRendererBuilder<WindowsOpenGLRendererBuilder, WindowsOpenGLRenderer>;
	private:
		util::PointerWrapper<WindowsWindow> m_window;

		void InitializeOpenGLImpl();

		static HGLRC CreateRenderingContext(HDC device_context);

		std::shared_ptr<WindowsOpenGLRenderer> BuildImpl();

		void BuildImpl(std::optional<WindowsOpenGLRenderer>& buffer);

		void BuildImpl(std::span<std::byte> buffer);

	public:
		using Base = fyuu_engine::opengl::BaseOpenGLRendererBuilder<WindowsOpenGLRendererBuilder, WindowsOpenGLRenderer>;

		WindowsOpenGLRendererBuilder();

		WindowsOpenGLRendererBuilder& SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window);

	};
#endif // _WIN32

}