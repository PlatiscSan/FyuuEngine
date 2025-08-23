module;

export module graphics;
import std;
export import :interface;

#ifdef WIN32
export import :d3d12_shader;
export import :win32vulkan_renderer;
export import :win32opengl_renderer;
#endif // WIN32

namespace graphics {
	export template <std::convertible_to<core::LoggerPtr> Logger>
	BaseRenderDevice& CreateMainRenderDevice(
		Logger&& logger, 
		platform::IWindow& main_window, 
		API api, 
		std::string_view app_name
	) {
#ifdef WIN32
		static std::variant<
			std::monostate,
			api::d3d12::D3D12RenderDevice,
			api::vulkan::Win32VulkanRenderDevice,
			api::opengl::Win32OpenGLRenderDevice
		> render_device;

		static std::once_flag render_device_initialized;

		auto& window = static_cast<platform::Win32Window&>(main_window);

		switch (api) {
		case graphics::API::Meta:
			throw std::invalid_argument("Meta can only run on apple");
		case graphics::API::OpenGL:
			std::call_once(
				render_device_initialized,
				[&]() {
					api::opengl::Win32OpenGLRenderDeviceBuilder builder;
					builder.SetLogger(std::forward<Logger>(logger))
						.SetWindow(&window)
						.SetFrameCount(3u)
						.Build();
					render_device.emplace<api::opengl::Win32OpenGLRenderDevice>(*builder.GetRenderDevice());
				}
			);
			return std::get<api::opengl::Win32OpenGLRenderDevice>(render_device);

		case graphics::API::Vulkan:
			std::call_once(
				render_device_initialized,
				[&]() {
					api::vulkan::Win32VulkanRenderDeviceBuilder builder;
					builder.SetApplicationName(app_name)
						.SetEngineName("FyuuEngine")
						.AddInstanceExtension(vk::KHRSurfaceExtensionName)
						.AddInstanceExtension(vk::KHRWin32SurfaceExtensionName)
						.AddDeviceExtension(vk::KHRSwapchainExtensionName)
						.SetAllocator(nullptr)
						.SetLogger(std::forward<Logger>(logger))
						.SetWindow(&window)
						.EnableValidationLayers()
						.Build();

					render_device.emplace<api::vulkan::Win32VulkanRenderDevice>(*builder.GetRenderDevice());
				}
			);
			return std::get<api::vulkan::Win32VulkanRenderDevice>(render_device);

		case graphics::API::DirectX12:
			std::call_once(
				render_device_initialized,
				[&]() {
					render_device.emplace<api::d3d12::D3D12RenderDevice>(std::forward<Logger>(logger), window, false, 3u);
				}
			);
			return std::get<api::d3d12::D3D12RenderDevice>(render_device);

		default:
			throw std::invalid_argument("Unknown API");
		}
#endif // WIN32y
	}
}