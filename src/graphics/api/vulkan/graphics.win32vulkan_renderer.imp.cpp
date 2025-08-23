module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

module graphics:win32vulkan_renderer;
import std;

namespace graphics::api::vulkan {
#ifdef WIN32

	LRESULT Win32VulkanRenderDevice::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		switch (msg) {
		case WM_DESTROY:
			m_is_window_alive = false;
			break;
		case WM_SIZE:
			m_is_minimized = m_core.logical_device && wparam == SIZE_MINIMIZED;
			m_is_window_alive = true;
			break;
		case WM_NCDESTROY:
			m_is_window_alive = false;
			break;
		default:
			m_is_window_alive = true;
			break;
		}

		return 0; // pass to next handler

	}

	Win32VulkanRenderDevice::Win32VulkanRenderDevice(Win32VulkanRenderDevice&& other) noexcept
		: BaseVulkanRenderDevice(std::move(other)) {

		auto window = static_cast<platform::Win32Window*>(m_window);
		window->DetachMsgProcessor(other.m_msg_processor_uuid);

		m_msg_processor_uuid = static_cast<platform::Win32Window*>(window)->AttachMsgProcessor(
			[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
				return WndProc(hwnd, msg, wparam, lparam);
			}
		);

	}

	Win32VulkanRenderDevice& Win32VulkanRenderDevice::operator=(Win32VulkanRenderDevice&& other) noexcept {

		if (this != &other) {

			static_cast<BaseVulkanRenderDevice&&>(*this) = std::move(static_cast<BaseVulkanRenderDevice&&>(other));

			auto window = static_cast<platform::Win32Window*>(m_window);
			window->DetachMsgProcessor(other.m_msg_processor_uuid);

			m_msg_processor_uuid = static_cast<platform::Win32Window*>(window)->AttachMsgProcessor(
				[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
					return WndProc(hwnd, msg, wparam, lparam);
				}
			);

		}

		return *this;

	}

	vk::UniqueSurfaceKHR graphics::api::vulkan::Win32VulkanBuilder::CreateSurface(vk::Instance& instance) {
		vk::Win32SurfaceCreateInfoKHR create_info{};
		create_info.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
		create_info.hinstance = GetModuleHandle(nullptr);
		create_info.hwnd = static_cast<platform::Win32Window*>(m_window)->GetHWND();

		return instance.createWin32SurfaceKHRUnique(create_info, m_allocator);
	}

#endif // WIN32
} // namespace graphics::api::vulkan