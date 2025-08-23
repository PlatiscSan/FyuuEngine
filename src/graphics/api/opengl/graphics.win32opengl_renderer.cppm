module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


#ifdef WIN32
#include <Windows.h>
#include <glad/glad.h>
#endif // WIN32

export module graphics:win32opengl_renderer;
export import :base_opengl_renderer;

namespace graphics::api::opengl {
#ifdef WIN32

	export class Win32OpenGLRenderDevice : public BaseOpenGLRenderDevice {
	private:
		HDC m_device_context;
		HGLRC m_gl_render_context;
		boost::uuids::uuid m_msg_processor_uuid;

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

		void SwapBuffer() override;

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		Win32OpenGLRenderDevice(
			Logger&& logger, 
			platform::IWindow& window, 
			std::size_t frame_count,
			HDC device_context,
			HGLRC gl_render_context
		) : BaseOpenGLRenderDevice(std::forward<Logger>(logger), window, frame_count),
			m_device_context(device_context),
			m_gl_render_context(gl_render_context),
			m_msg_processor_uuid(
				static_cast<platform::Win32Window*>(m_window)->AttachMsgProcessor(
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
						return WndProc(hwnd, msg, wparam, lparam);
					}
				)
			) {
		}

		~Win32OpenGLRenderDevice() noexcept override;

		Win32OpenGLRenderDevice(Win32OpenGLRenderDevice&& other) noexcept;
		Win32OpenGLRenderDevice& operator=(Win32OpenGLRenderDevice&& other) noexcept;


	};

	export class Win32OpenGLRenderDeviceBuilder final
		: public BaseOpenGLRenderDeviceBuilder<Win32OpenGLRenderDeviceBuilder, Win32OpenGLRenderDevice> {
	private:
		friend class BaseOpenGLRenderDeviceBuilder<Win32OpenGLRenderDeviceBuilder, Win32OpenGLRenderDevice>;
		void BuildImp();
	};

#endif // WIN32
}