module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

export module graphics:win32vulkan_renderer;
export import :base_vulkan_renderer;

namespace graphics::api::vulkan {
#ifdef WIN32

	export class Win32VulkanRenderDevice final : public BaseVulkanRenderDevice {
	private:
		boost::uuids::uuid m_msg_processor_uuid;

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

	public:
		template <
			std::convertible_to<core::LoggerPtr> Logger,
			std::convertible_to<VulkanCore> CompatibleVulkanCore,
			std::convertible_to<SwapChainBundle> CompatibleSwapChainBundle,
			std::convertible_to<vk::UniqueRenderPass> CompatibleRenderPass
		>
		Win32VulkanRenderDevice(
			Logger&& logger,
			platform::IWindow& window,
			CompatibleVulkanCore&& core,
			CompatibleSwapChainBundle&& swap_chain_bundle,
			CompatibleRenderPass&& render_pass
		) : BaseVulkanRenderDevice(
				std::forward<Logger>(logger),
				window,
				std::forward<CompatibleVulkanCore>(core),
				std::forward<CompatibleSwapChainBundle>(swap_chain_bundle),
				std::forward<CompatibleRenderPass>(render_pass)
			),
			m_msg_processor_uuid(
				static_cast<platform::Win32Window*>(m_window)->AttachMsgProcessor(
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
						return WndProc(hwnd, msg, wparam, lparam);
					}
				)
			) {

		}

		Win32VulkanRenderDevice(Win32VulkanRenderDevice&& other) noexcept;
		Win32VulkanRenderDevice& operator=(Win32VulkanRenderDevice&& other) noexcept;

	};

	class Win32VulkanBuilder final : public BaseVulkanBuilder<Win32VulkanBuilder, Win32VulkanRenderDevice> {
	private:
		friend class BaseVulkanBuilder<Win32VulkanBuilder, Win32VulkanRenderDevice>;

		vk::UniqueSurfaceKHR CreateSurface(vk::Instance& instance);

		template <
			std::convertible_to<VulkanCore> CompatibleVulkanCore,
			std::convertible_to<SwapChainBundle> CompatibleSwapChainBundle,
			std::convertible_to<vk::UniqueRenderPass> CompatibleRenderPass
		>
		void BuildImp(
			CompatibleVulkanCore&& core,
			CompatibleSwapChainBundle&& swap_chain_bundle,
			CompatibleRenderPass&& render_pass
		) {
			m_render_device.emplace(
				util::MakeReferred(m_logger),
				*m_window,
				std::forward<CompatibleVulkanCore>(core),
				std::forward<CompatibleSwapChainBundle>(swap_chain_bundle),
				std::forward<CompatibleRenderPass>(render_pass)
			);
		}

	};

#endif // WIN32
}