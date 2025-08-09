export module imgui_layer;
#ifdef WIN32
export import :d3d12;
export import :win32opengl;
#endif // WIN32

namespace ui::imgui {
	export core::ILayer& CreateImGUILayer(platform::IWindow& main_window, graphics::BaseRenderDevice& main_device);
}
