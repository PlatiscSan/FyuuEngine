module imgui_layer;
import std;

namespace ui::imgui {
	BaseIMGUILayer& CreateIMGUILayer(platform::IWindow& main_window, graphics::BaseRenderDevice& main_device) {
#ifdef WIN32
		auto& window = static_cast<platform::Win32Window&>(main_window);
		switch (main_device.GetAPI()) {
		case graphics::API::Meta:
			throw std::invalid_argument("Meta can only run on apple");
		case graphics::API::OpenGL:
		{
			static api::opengl::Win32OpenGLIMGUILayer opengl_layer(
				window, 
				static_cast<graphics::api::opengl::Win32OpenGLRenderDevice&>(main_device)
			);
			return opengl_layer;
		}
		case graphics::API::Vulkan:
		{
			static api::vulkan::Win32VulkanIMGUILayer vulkan_layer(
				window,
				static_cast<graphics::api::vulkan::Win32VulkanRenderDevice&>(main_device)
			);
			return vulkan_layer;
		}
		case graphics::API::DirectX12:
		{
			static api::d3d12::D3D12IMGUILayer d3d12_layer(
				window,
				static_cast<graphics::api::d3d12::D3D12RenderDevice&>(main_device)
			);
			return d3d12_layer;
		}
		default:
			throw std::invalid_argument("Unknown API");
		}
#endif // WIN32
	}
}
