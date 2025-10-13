module;
#include <glad/glad.h>
#ifdef _WIN32
#include <glad/glad_wgl.h>
#endif // _WIN32

module windows_opengl:renderer;
import win32exception;

namespace fyuu_engine::windows::opengl {

	void WindowsOpenGLRenderer::MakeCurrent() {
		static thread_local WindowsOpenGLRenderer* current_renderer;
		if (current_renderer != this) {
			if (!wglMakeCurrent(m_device_context, m_gl_rendering_context)) {
				throw windows::Win32Exception();
			}
			current_renderer = this;
		}
	}

	void WindowsOpenGLRenderer::SwapBuffer() {
		SwapBuffers(m_device_context);
	}

	LRESULT WindowsOpenGLRenderer::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		switch (msg) {
		case WM_DESTROY:
			m_is_window_alive = false;
			break;
		case WM_SIZE:
			m_is_minimized = wparam == SIZE_MINIMIZED;
			m_is_window_alive = true;
			CreateFrameBuffer(LOWORD(lparam), HIWORD(lparam));
			break;
		case WM_NCDESTROY:
			m_is_window_alive = false;
			break;
		default:
			m_is_window_alive = true;
			break;
		}

		return 0; // pass to next handler

	}

	std::pair<std::uint32_t, std::uint32_t> WindowsOpenGLRenderer::GetTargetWindowWidthAndHeight() const noexcept {
		return m_window->GetWidthAndHeight();
	}

	WindowsOpenGLRenderer::WindowsOpenGLRenderer(
		core::LoggerPtr const& logger,
		HDC device_context,
		HGLRC gl_rendering_context,
		util::PointerWrapper<WindowsWindow> const& window
	) : Base(util::MakeReferred(logger)),
		m_device_context(device_context),
		m_gl_rendering_context(gl_rendering_context),
		m_window(util::MakeReferred(window)),
		m_msg_processor_uuid(
			m_window->AttachMsgProcessor(
				[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
					return WndProc(hwnd, msg, wparam, lparam);
				}
			)
		) {


	}

	void WindowsOpenGLRendererBuilder::InitializeOpenGLImpl() {

		HDC device_context = GetDC(m_window->GetHWND());

		PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0,
			24,
			8,
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int pixel_format = ChoosePixelFormat(device_context, &pfd);
		if (pixel_format == 0) {
			throw windows::Win32Exception();
		}

		if (!SetPixelFormat(device_context, pixel_format, &pfd)) {
			throw windows::Win32Exception();
		}

		HGLRC temp_context = wglCreateContext(device_context);
		if (!temp_context) {
			throw windows::Win32Exception();
		}

		if (!wglMakeCurrent(device_context, temp_context)) {
			throw windows::Win32Exception();
		}

		if (!gladLoadGL()) {
			throw std::runtime_error("Failed to initialize GL");
		}

		if (!gladLoadWGL(device_context)) {
			throw std::runtime_error("Failed to initialize WGL");
		}

		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(temp_context);

	}

	HGLRC WindowsOpenGLRendererBuilder::CreateRenderingContext(HDC device_context) {

		HGLRC gl_rendering_context;

		if (!wglCreateContextAttribsARB) {

			gl_rendering_context = wglCreateContext(device_context);
			if (!gl_rendering_context) {
				throw windows::Win32Exception();
			}

			return gl_rendering_context;

		}

		static std::array const attributes = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		gl_rendering_context = wglCreateContextAttribsARB(device_context, nullptr, attributes.data());

		if (!gl_rendering_context) {
			throw windows::Win32Exception();
		}

		return gl_rendering_context;

	}

	std::shared_ptr<WindowsOpenGLRenderer> WindowsOpenGLRendererBuilder::BuildImpl() {

		InitializeOpenGL();

		HDC device_context = GetDC(m_window->GetHWND());
		HGLRC gl_rendering_context = CreateRenderingContext(device_context);
		wglMakeCurrent(device_context, gl_rendering_context);
		CheckSupports();
		BindLogger();

		return std::make_shared<WindowsOpenGLRenderer>(m_logger, device_context, gl_rendering_context, m_window);

	}

	void WindowsOpenGLRendererBuilder::BuildImpl(std::optional<WindowsOpenGLRenderer>& buffer) {

		InitializeOpenGL();

		HDC device_context = GetDC(m_window->GetHWND());
		HGLRC gl_rendering_context = CreateRenderingContext(device_context);
		wglMakeCurrent(device_context, gl_rendering_context);
		CheckSupports();
		BindLogger();

		buffer.emplace(m_logger, device_context, gl_rendering_context, m_window);

	}

	void WindowsOpenGLRendererBuilder::BuildImpl(std::span<std::byte> buffer) {

		InitializeOpenGL();

		HDC device_context = GetDC(m_window->GetHWND());
		HGLRC gl_rendering_context = CreateRenderingContext(device_context);
		wglMakeCurrent(device_context, gl_rendering_context);
		CheckSupports();
		BindLogger();

		new(buffer.data()) WindowsOpenGLRenderer(
			m_logger, device_context, gl_rendering_context, m_window
		);
	}

	WindowsOpenGLRendererBuilder::WindowsOpenGLRendererBuilder() {
	}

	WindowsOpenGLRendererBuilder& WindowsOpenGLRendererBuilder::SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window) {
		m_window = util::MakeReferred(window);
		return *this;
	}

}