export module graphics:win32vulkan;
import :vulkan;
import window;
import std;

export namespace graphics::api::vulkan {
#ifdef WIN32
	class Win32VulkanRenderDevice : public BaseVulkanRenderDevice {
	public:
		Win32VulkanRenderDevice();

	};
#endif // WIN32
}