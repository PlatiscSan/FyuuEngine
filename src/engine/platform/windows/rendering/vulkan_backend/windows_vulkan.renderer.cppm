module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32


export module windows_vulkan:renderer;
export import vulkan_backend;
export import windows_window;
import std;

namespace fyuu_engine::windows::vulkan {
#ifdef _WIN32

	export class WindowsVulkanRenderer
		: public fyuu_engine::vulkan::BaseVulkanRenderer<WindowsVulkanRenderer> {
		friend class fyuu_engine::vulkan::BaseVulkanRenderer<WindowsVulkanRenderer>;

	private:
		util::PointerWrapper<WindowsWindow> m_window;
		boost::uuids::uuid m_msg_processor_uuid;

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;
		std::pair<std::uint32_t, std::uint32_t> GetTargetWindowWidthAndHeight() const noexcept;

	public:
		using Base = fyuu_engine::vulkan::BaseVulkanRenderer<WindowsVulkanRenderer>;

		WindowsVulkanRenderer(
			core::LoggerPtr const& logger,
			fyuu_engine::vulkan::VulkanCore&& core,
			fyuu_engine::vulkan::SwapChainBundle&& swap_chain_bundle,
			vk::UniqueRenderPass&& render_pass,
			fyuu_engine::vulkan::BufferPoolSizeMap const& pool_size_map,
			util::PointerWrapper<WindowsWindow> const& window
		);

		~WindowsVulkanRenderer() noexcept;

	};

	export class WindowsVulkanRendererBuilder
		: public fyuu_engine::vulkan::BaseVulkanRendererBuilder<WindowsVulkanRendererBuilder, WindowsVulkanRenderer> {
		friend class fyuu_engine::vulkan::BaseVulkanRendererBuilder<WindowsVulkanRendererBuilder, WindowsVulkanRenderer>;
	private:
		util::PointerWrapper<WindowsWindow> m_window;

		vk::UniqueSurfaceKHR CreateSurface(vk::UniqueInstance const& instance);

		std::shared_ptr<WindowsVulkanRenderer> BuildImpl();
		void BuildImpl(std::optional<WindowsVulkanRenderer>& buffer);
		void BuildImpl(std::span<std::byte> buffer);

	public:
		using Base = fyuu_engine::vulkan::BaseVulkanRendererBuilder<WindowsVulkanRendererBuilder, WindowsVulkanRenderer>;

		WindowsVulkanRendererBuilder();

		WindowsVulkanRendererBuilder& SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window);
	};

#endif // _WIN32

}