export module graphics;
import std;
export import :interface;

#ifdef WIN32
export import :d3d12_shader;
export import :win32vulkan_renderer;
export import :win32opengl;
#endif // WIN32

namespace graphics {
	export template <std::convertible_to<core::LoggerPtr> Logger>
	BaseRenderDevice& CreateMainRenderDevice(Logger&&logger, platform::IWindow& main_window, API api) {
#ifdef WIN32
		auto& window = static_cast<platform::Win32Window&>(main_window);
		switch (api) {
		case graphics::API::Meta:
			throw std::invalid_argument("Meta can only run on apple");
		case graphics::API::OpenGL:
		{
			//static api::opengl::Win32OpenGLRenderDevice opengl_device(std::forward<Logger>(logger), window);
			//return opengl_device;
		}
		case graphics::API::Vulkan:
		{
			//static api::vulkan::Win32VulkanRenderDevice vulkan_device(
			//	std::forward<Logger>(logger), 
			//	window, 
			//	nullptr,
			//	std::vector<char const*>({ vk::KHRSurfaceExtensionName, vk::KHRWin32SurfaceExtensionName })
			//);
			//return vulkan_device;
		}
		case graphics::API::DirectX12:
		{
			static api::d3d12::D3D12RenderDevice d3d12_device(std::forward<Logger>(logger), window);
			return d3d12_device;
		}
		default:
			throw std::invalid_argument("Unknown API");
		}
#endif // WIN32y
	}
}