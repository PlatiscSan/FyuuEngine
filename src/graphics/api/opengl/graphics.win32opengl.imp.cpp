module;
#define WGL_WGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#ifdef WIN32
#include <GL/wglext.h>
#endif // WIN32

module graphics:win32opengl;

namespace graphics::api::opengl {
#ifdef WIN32
	HDC CreateDeviceContext(HWND hwnd) {
		
		HDC dc = GetDC(hwnd);
		PIXELFORMATDESCRIPTOR pfd{};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;

		int pixel_format = ChoosePixelFormat(dc, &pfd);
		if (pixel_format == 0) {
			throw platform::Win32Exception();
		}

		if (!SetPixelFormat(dc, pixel_format, &pfd)) {
			throw platform::Win32Exception();
			return FALSE;
		}

		return dc;

	}

	HGLRC CreateGLRenderContext(HDC device_context) {
		auto gl_render_context = wglCreateContext(device_context);
		if (!gl_render_context) {
			throw platform::Win32Exception();
		}
		return gl_render_context;
	}

	Win32OpenGLRenderDevice::~Win32OpenGLRenderDevice() noexcept {
		wglMakeCurrent(nullptr, nullptr);
		ReleaseDC(static_cast<platform::Win32Window*>(m_window)->GetHWND(), m_device_context);
		wglDeleteContext(m_gl_render_context);
	}

	void Win32OpenGLRenderDevice::Clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void Win32OpenGLRenderDevice::SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) {
		glViewport(x, y, width, height);
	}

	bool Win32OpenGLRenderDevice::BeginFrame() {
		return !IsIconic(static_cast<platform::Win32Window*>(m_window)->GetHWND());
	}

	void Win32OpenGLRenderDevice::EndFrame() {
		SwapBuffers(m_device_context);
	}

	API Win32OpenGLRenderDevice::GetAPI() const noexcept {
		return API::OpenGL;
	}

	ICommandObject& Win32OpenGLRenderDevice::AcquireCommandObject() {
		// TODO: insert return statement here
	}

#endif // WIN32
}


