module;

#include <GL/glcorearb.h>
#ifdef WIN32
#include <GL/wglext.h>
#endif // WIN32

export module graphics:win32opengl;
import window;
import :interface;
import std;

namespace graphics::api::opengl {
#ifdef WIN32
	export class Win32OpenGLRenderDevice : public IRenderDevice {
	private:
		HWND m_hwnd;
		HDC m_device_context;
		HGLRC m_gl_render_context;

	public:
		Win32OpenGLRenderDevice(platform::Win32Window& window);
		~Win32OpenGLRenderDevice() noexcept override;

		void Clear(float r, float g, float b, float a) override;

		void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) override;

		bool BeginFrame() override;

		void EndFrame() override;

		API GetAPI() const noexcept override;

	};
#endif // WIN32
}