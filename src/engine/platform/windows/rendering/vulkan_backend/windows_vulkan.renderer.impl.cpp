module;
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

module windows_vulkan:renderer;
import vulkan_hpp;

namespace fyuu_engine::windows::vulkan {
#ifdef _WIN32

	LRESULT WindowsVulkanRenderer::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {

		switch (msg) {
		case WM_DESTROY:
			m_is_window_alive = false;
			break;
		case WM_SIZE:
			m_is_minimized = m_core.logical_device && wparam == SIZE_MINIMIZED;
			m_is_window_alive = true;
			RecreateSwapChain();
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

	std::pair<std::uint32_t, std::uint32_t> WindowsVulkanRenderer::GetTargetWindowWidthAndHeight() const noexcept {
		return m_window->GetWidthAndHeight();
	}

	WindowsVulkanRenderer::WindowsVulkanRenderer(
		core::LoggerPtr const& logger,
		fyuu_engine::vulkan::VulkanCore&& core,
		fyuu_engine::vulkan::SwapChainBundle&& swap_chain_bundle,
		vk::UniqueRenderPass&& render_pass,
		fyuu_engine::vulkan::BufferPoolSizeMap const& pool_size_map,
		util::PointerWrapper<WindowsWindow> const& window
	) : Base(util::MakeReferred(logger), std::move(core), std::move(swap_chain_bundle), std::move(render_pass), pool_size_map),
		m_window(util::MakeReferred(window)),
		m_msg_processor_uuid(
			m_window->AttachMsgProcessor(
				[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
					return WndProc(hwnd, msg, wparam, lparam);
				}
			)
		) {

	}

	WindowsVulkanRenderer::~WindowsVulkanRenderer() noexcept {
		m_window->DetachMsgProcessor(m_msg_processor_uuid);
	}

	vk::UniqueSurfaceKHR WindowsVulkanRendererBuilder::CreateSurface(vk::UniqueInstance const& instance) {

		vk::Win32SurfaceCreateInfoKHR create_info({}, GetModuleHandle(nullptr), m_window->GetHWND());
		return instance->createWin32SurfaceKHRUnique(create_info, m_allocator);

	}

	std::shared_ptr<WindowsVulkanRenderer> WindowsVulkanRendererBuilder::BuildImpl() {

		fyuu_engine::vulkan::VulkanCore core = CreateCore();
		auto [width, height] = m_window->GetWidthAndHeight();
		fyuu_engine::vulkan::SwapChainBundle swap_chain_bundle = fyuu_engine::vulkan::CreateSwapChainBundle(core, width, height);
		vk::UniqueRenderPass render_pass = fyuu_engine::vulkan::CreateRenderPass(core, swap_chain_bundle);

		return std::make_shared<WindowsVulkanRenderer>(
			m_logger,
			std::move(core),
			std::move(swap_chain_bundle),
			std::move(render_pass),
			m_buffer_pool_size_map,
			m_window
		);

	}

	void WindowsVulkanRendererBuilder::BuildImpl(std::optional<WindowsVulkanRenderer>& buffer) {

		fyuu_engine::vulkan::VulkanCore core = CreateCore();
		auto [width, height] = m_window->GetWidthAndHeight();
		fyuu_engine::vulkan::SwapChainBundle swap_chain_bundle = fyuu_engine::vulkan::CreateSwapChainBundle(core, width, height);
		vk::UniqueRenderPass render_pass = fyuu_engine::vulkan::CreateRenderPass(core, swap_chain_bundle);

		buffer.emplace(
			m_logger,
			std::move(core),
			std::move(swap_chain_bundle),
			std::move(render_pass),
			m_buffer_pool_size_map,
			m_window
		);

	}

	void WindowsVulkanRendererBuilder::BuildImpl(std::span<std::byte> buffer) {

		if (buffer.size() < sizeof(WindowsVulkanRenderer)) {
			throw std::invalid_argument("no sufficient buffer space");
		}

		fyuu_engine::vulkan::VulkanCore core = CreateCore();
		auto [width, height] = m_window->GetWidthAndHeight();
		fyuu_engine::vulkan::SwapChainBundle swap_chain_bundle = fyuu_engine::vulkan::CreateSwapChainBundle(core, width, height);
		vk::UniqueRenderPass render_pass = fyuu_engine::vulkan::CreateRenderPass(core, swap_chain_bundle);

		new(buffer.data()) WindowsVulkanRenderer(
			m_logger,
			std::move(core),
			std::move(swap_chain_bundle),
			std::move(render_pass),
			m_buffer_pool_size_map,
			m_window
		);

	}

	WindowsVulkanRendererBuilder::WindowsVulkanRendererBuilder() 
		: Base() {
		m_instance_extensions.emplace_back("VK_KHR_win32_surface");
	}


	WindowsVulkanRendererBuilder& WindowsVulkanRendererBuilder::SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window) {
		m_window = util::MakeReferred(window);
		return *this;
	}


#endif // _WIN32
}

