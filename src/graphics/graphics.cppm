export module graphics;
import std;
export import :interface;

#ifdef WIN32
export import :d3d12;
export import :win32vulkan;
export import :win32opengl;
#endif // WIN32

namespace graphics {
	export IRenderDevice& CreateMainRenderDevice(platform::IWindow& main_window, API api);
}