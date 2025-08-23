module;
#ifdef WIN32
#include <Windows.h>
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#endif // WIN32

module graphics:win32opengl_renderer;

namespace graphics::api::opengl {
#ifdef WIN32
	static HGLRC InitializeGL(HDC device_context) {

		HGLRC gl_render_context = nullptr;

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
			throw platform::Win32Exception();
		}

		if (!SetPixelFormat(device_context, pixel_format, &pfd)) {
			throw platform::Win32Exception();
		}

		HGLRC temp_context = wglCreateContext(device_context);
		if (!temp_context) {
			throw platform::Win32Exception();
		}

		if (!wglMakeCurrent(device_context, temp_context)) {
			throw platform::Win32Exception();
		}

		if (!gladLoadGL()) {
			throw std::runtime_error("Failed to initialize GLAD");
		}

		if (wglCreateContextAttribsARB) {

			std::array attributes = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 3,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0
			};

			gl_render_context = wglCreateContextAttribsARB(device_context, nullptr, attributes.data());
			if (gl_render_context) {
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(temp_context);
				wglMakeCurrent(device_context, gl_render_context);
			}
			else {
				gl_render_context = temp_context;
			}
		}

		return gl_render_context;

	}

	LRESULT Win32OpenGLRenderDevice::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		switch (msg) {
		case WM_DESTROY:
			m_is_window_alive = false;
			break;
		case WM_SIZE:
			m_is_minimized = wparam == SIZE_MINIMIZED;
			m_is_window_alive = true;
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

	void Win32OpenGLRenderDevice::SwapBuffer() {
		if (!SwapBuffers(m_device_context)) {
			throw platform::Win32Exception();
		}
	}

	Win32OpenGLRenderDevice::~Win32OpenGLRenderDevice() noexcept {
		if (m_window && m_device_context && m_gl_render_context) {
			wglMakeCurrent(nullptr, nullptr);
			ReleaseDC(static_cast<platform::Win32Window*>(m_window)->GetHWND(), m_device_context);
			wglDeleteContext(m_gl_render_context);
		}
	}

	Win32OpenGLRenderDevice::Win32OpenGLRenderDevice(Win32OpenGLRenderDevice&& other) noexcept
		: BaseOpenGLRenderDevice(std::move(other)),
		m_device_context(std::exchange(other.m_device_context, nullptr)),
		m_gl_render_context(std::exchange(other.m_gl_render_context, nullptr)) {

		auto window = static_cast<platform::Win32Window*>(m_window);
		window->DetachMsgProcessor(other.m_msg_processor_uuid);

		m_msg_processor_uuid = static_cast<platform::Win32Window*>(window)->AttachMsgProcessor(
			[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
				return WndProc(hwnd, msg, wparam, lparam);
			}
		);

	}

	Win32OpenGLRenderDevice& Win32OpenGLRenderDevice::operator=(Win32OpenGLRenderDevice&& other) noexcept {

		if (this != &other) {

			if (m_window && m_device_context && m_gl_render_context) {
				wglMakeCurrent(nullptr, nullptr);
				ReleaseDC(static_cast<platform::Win32Window*>(m_window)->GetHWND(), m_device_context);
				wglDeleteContext(m_gl_render_context);
			}

			static_cast<BaseOpenGLRenderDevice&&>(*this) = std::move(static_cast<BaseOpenGLRenderDevice&&>(other));

			m_device_context = std::exchange(other.m_device_context, nullptr);
			m_gl_render_context = std::exchange(other.m_gl_render_context, nullptr);
			wglMakeCurrent(m_device_context, m_gl_render_context);

			auto window = static_cast<platform::Win32Window*>(m_window);
			window->DetachMsgProcessor(other.m_msg_processor_uuid);

			m_msg_processor_uuid = static_cast<platform::Win32Window*>(window)->AttachMsgProcessor(
				[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
					return WndProc(hwnd, msg, wparam, lparam);
				}
			);

		}

		return *this;
	}

	void Win32OpenGLRenderDeviceBuilder::BuildImp() {

		auto window = static_cast<platform::Win32Window*>(m_window);
		HDC device_context = GetDC(window->GetHWND());
		HGLRC gl_render_context = InitializeGL(device_context);

		if (m_logger) {
			m_logger->Info(
				std::source_location::current(),
				"OpenGL Version {}, GLSL Version {}",
				reinterpret_cast<char const*>(glGetString(GL_VERSION)),
				reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION))
			);
		}

		m_render_device.emplace(
			util::MakeReferred(m_logger), 
			*m_window, 
			m_frame_count, 
			device_context, gl_render_context
		);

	}

#endif // WIN32
}


