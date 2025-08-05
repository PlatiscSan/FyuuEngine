module graphics;

namespace graphics {
	IRenderDevice& graphics::CreateMainRenderDevice(platform::IWindow& main_window, API api) {
#ifdef WIN32
		auto& window = static_cast<platform::Win32Window&>(main_window);
		switch (api) {
		case graphics::API::Meta:
			throw std::invalid_argument("Meta can only run on apple");
		case graphics::API::OpenGL:
		{
			static api::opengl::Win32OpenGLRenderDevice opengl_device(window);
			return opengl_device;
		}
		case graphics::API::Vulkan:
			break;
		case graphics::API::DirectX12:
		{
			static api::d3d12::D3D12RenderDevice d3d12_device(window);
			return d3d12_device;
		}
		default:
			throw std::invalid_argument("Unknown API");
		}
#endif // WIN32

	}
}