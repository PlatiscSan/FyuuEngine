module;

#include <GL/glcorearb.h>
#ifdef WIN32
#include <GL/wglext.h>
#endif // WIN32

export module win32_opengl_renderer;
export import win32_window;
export import renderer_interface;
import std;

namespace graphics::api::opengl {
#ifdef WIN32
	class Win32OpenGLRenderDevice : public IRenderDevice {
	private:
		HDC m_device_context;
		HGLRC m_gl_render_context;

		static HDC CreateDeviceContext(platform::Win32Window& window) {

			HWND hwnd = window.GetHWND();

			HDC dc = GetDC(window.GetHWND());
			PIXELFORMATDESCRIPTOR pfd = { 0 };
			pfd.nSize = sizeof(pfd);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.cColorBits = 32;
			int const pf = ChoosePixelFormat(dc, &pfd);

			if (pf == 0 || !SetPixelFormat(dc, pf, &pfd)) {
				throw platform::Win32Exception();
			}

			ReleaseDC(hwnd, dc);
			dc = GetDC(window.GetHWND());

			return dc;

		}

		static HGLRC CreateGLRenderContext(HDC device_context) {
			auto gl_render_context = wglCreateContext(device_context);
			if (!gl_render_context) {
				throw platform::Win32Exception();
			}
			return gl_render_context;
		}

	public:
		Win32OpenGLRenderDevice(platform::Win32Window& window)
			: m_device_context(Win32OpenGLRenderDevice::CreateDeviceContext(window)),
			m_gl_render_context(Win32OpenGLRenderDevice::CreateGLRenderContext(m_device_context)) {
			wglMakeCurrent(m_device_context, m_gl_render_context);
		}


		std::unique_ptr<IShader> CreateShader(std::filesystem::path const& vs_src, std::filesystem::path const& fs_src) override {
			return nullptr;
		}

		std::unique_ptr<ITexture> CreateTexture(int width, std::uint32_t height, void const* data) override {
			return nullptr;
		}

		std::unique_ptr<IVertexBuffer> CreateVertexBuffer() override {
			return nullptr;
		}

		void Clear(float r, float g, float b, float a) override {

		}

		void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) override {

		}

		void DrawTriangles(std::uint32_t vertex_count) override {

		}


	};
#endif // WIN32
}