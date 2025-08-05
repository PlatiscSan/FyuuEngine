module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

export module imgui_layer:win32opengl;
export import graphics;
export import layer;
import std;
#ifdef WIN32
export import <imgui_impl_win32.h>;
#endif // WIN32
import <imgui_impl_opengl3.h>;
import window;

namespace ui::imgui::api::opengl {
	export class Win32OpenGLImGUILayer : public core::ILayer {
	public:
		platform::Win32Window* m_window;
		graphics::api::opengl::Win32OpenGLRenderDevice* m_device;
		boost::uuids::uuid m_layer_uuid;
		ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		bool m_show_demo_window = true;
		bool m_show_another_window = false;

		static boost::uuids::uuid InitializeImGUI(
			Win32OpenGLImGUILayer* layer,
			platform::Win32Window* window,
			graphics::api::opengl::Win32OpenGLRenderDevice* device
		);

	public:
		Win32OpenGLImGUILayer(platform::Win32Window& window, graphics::api::opengl::Win32OpenGLRenderDevice& device);
		~Win32OpenGLImGUILayer() noexcept override;
		void BeginFrame() override;
		void EndFrame() override;
		void Update() override;
		void Render() override;
	};
}