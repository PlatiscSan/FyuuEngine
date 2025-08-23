export module imgui_layer;
#ifdef WIN32
export import :d3d12;
export import :win32opengl;
export import :win32vulkan;
#endif // WIN32

namespace ui::imgui {
	export BaseIMGUILayer& CreateIMGUILayer(platform::IWindow& main_window, graphics::BaseRenderDevice& main_device);
}
