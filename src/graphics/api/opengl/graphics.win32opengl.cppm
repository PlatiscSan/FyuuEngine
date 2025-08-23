module;
#ifdef WIN32
#include <Windows.h>
#include <glad/glad.h>
#endif // WIN32

export module graphics:win32opengl;
import window;
import :interface;
import std;

namespace graphics::api::opengl {
#ifdef WIN32
	extern HDC CreateDeviceContext(HWND hwnd);
	extern HGLRC CreateGLRenderContext(HDC device_context);

	export class Win32OpenGLRenderDevice : public BaseRenderDevice {
	private:
		HDC m_device_context;
		HGLRC m_gl_render_context;
		core::LoggerPtr m_logger;

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		Win32OpenGLRenderDevice(Logger&& logger, platform::Win32Window& window) 
			: BaseRenderDevice(std::forward<Logger>(logger), window),
			m_device_context(CreateDeviceContext(window.GetHWND())),
			m_gl_render_context(CreateGLRenderContext(m_device_context)) {
			wglMakeCurrent(m_device_context, m_gl_render_context);
		}

		~Win32OpenGLRenderDevice() noexcept override;

		void Clear(float r, float g, float b, float a) override;

		void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) override;

		bool BeginFrame() override;

		void EndFrame() override;

		API GetAPI() const noexcept override;

		ICommandObject& AcquireCommandObject() override;


	};
#endif // WIN32
}