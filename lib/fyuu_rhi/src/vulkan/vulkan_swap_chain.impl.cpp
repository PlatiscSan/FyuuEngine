module fyuu_rhi:vulkan_swap_chain;
import :vulkan_physical_device;
import :vulkan_logical_device;
import :vulkan_surface;
import :vulkan_command_object;

namespace fyuu_rhi::vulkan {

	static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::span<vk::SurfaceFormatKHR> available_formats) {
		// BGRA8 Unorm in nonlinear space is preferred
		for (vk::SurfaceFormatKHR const& available_format : available_formats) {
			if (available_format.format == vk::Format::eB8G8R8A8Unorm && available_format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
				return available_format;
			}
		}

		//otherwise hope the first one is good enough
		return available_formats[0];

	}

	static vk::PresentModeKHR ChooseSwapPresentMode(std::span<vk::PresentModeKHR> available_present_modes, bool v_sync) {

		if (v_sync) {

			for (vk::PresentModeKHR const& available_present_mode : available_present_modes) {
				if (available_present_mode == vk::PresentModeKHR::eMailbox) {
					return available_present_mode;    // use Mailbox on high-perf devices
				}
			}

			return vk::PresentModeKHR::eFifo;        // otherwise use FIFO when on low-power devices, like a android devices
		}
		else {
			return vk::PresentModeKHR::eImmediate;
		}

	}

	static vk::Extent2D ChooseSwapExtent(vk::SurfaceCapabilitiesKHR& capabilities, std::uint32_t width, std::uint32_t height) {

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			vk::Extent2D actual_extent(width, height);

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	};

	static vk::UniqueSemaphore CreateSemaphore(
		vk::Device const& logical_device,
		std::shared_ptr<vk::DispatchLoaderDynamic> const& dispatcher
	) {
		vk::SemaphoreCreateInfo info;
		return logical_device.createSemaphoreUnique(
			info,
			nullptr,
			*dispatcher
		);
	}

	VulkanSwapChain::SwapChain VulkanSwapChain::CreateSwapChainImpl(
		VulkanPhysicalDevice const& physical_device,
		VulkanLogicalDevice const& logical_device,
		VulkanCommandQueue const& present_queue,
		VulkanSurface const& surface,
		std::uint32_t buffer_size
	) {

		std::shared_ptr<vk::DispatchLoaderDynamic> dispatcher = logical_device.GetDispatcher();

		vk::Bool32 present_support = physical_device.GetNative().getSurfaceSupportKHR(
			present_queue.GetFamily(),
			surface.GetNative(),
			*dispatcher
		);

		if (!present_support) {
			throw std::runtime_error("The queue does not support presentation to the given surface.");
		}

		VulkanSwapChainSupportDetails swap_chain_support = physical_device.QuerySwapChainSupport(surface);
		
		vk::SurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
		vk::PresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes, true);

		auto&& [width, height] = surface.GetWidthAndHeight();

		VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities, width, height);

		std::uint32_t desired_image_count = buffer_size > 0 ?
			buffer_size :
			swap_chain_support.capabilities.minImageCount + 1;

		std::uint32_t image_count = std::clamp(
			desired_image_count,
			swap_chain_support.capabilities.minImageCount,
			swap_chain_support.capabilities.maxImageCount > 0 ?
			swap_chain_support.capabilities.maxImageCount :
			std::numeric_limits<std::uint32_t>::max()
		);

		vk::SwapchainCreateInfoKHR swap_chain_create_info(
			{},
			surface.GetNative(),
			image_count,
			surface_format.format,
			surface_format.colorSpace,
			extent,
			1,													// always 1 unless we are doing stereoscopic 3D
			vk::ImageUsageFlagBits::eColorAttachment,			// use vk::ImageUsageFlagBits::eColorAttachment for offscreen rendering
			vk::SharingMode::eExclusive,						// we will set the flag the later
			{},
			swap_chain_support.capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode,
			vk::True,											// we don't care about pixels that are obscured
			nullptr
		);

		vk::UniqueSwapchainKHR vk_swap_chain = logical_device.GetNative().createSwapchainKHRUnique(
			swap_chain_create_info,
			nullptr,
			*dispatcher
		);

		return {std::move(vk_swap_chain), surface_format.format, extent, image_count };

	}

	void VulkanSwapChain::ResizeImpl(std::uint32_t width, std::uint32_t height) {

	}

	VulkanSwapChain::VulkanSwapChain(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
		plastic::utility::AnyPointer<VulkanCommandQueue> const& present_queue,
		plastic::utility::AnyPointer<VulkanSurface> const& surface,
		std::uint32_t buffer_size
	) : m_image_available(CreateSemaphore(logical_device->GetNative(), logical_device->GetDispatcher())),
		m_rendering_completed(CreateSemaphore(logical_device->GetNative(), logical_device->GetDispatcher())),
		m_impl(VulkanSwapChain::CreateSwapChainImpl(*physical_device, *logical_device, *present_queue, *surface, buffer_size)),
		m_present_queue(present_queue->GetID()) {
		
	}

	VulkanSwapChain::VulkanSwapChain(
		VulkanPhysicalDevice const& physical_device, 
		VulkanLogicalDevice const& logical_device, 
		VulkanCommandQueue const& present_queue, 
		VulkanSurface const& surface, 
		std::uint32_t buffer_size
	) : m_image_available(CreateSemaphore(logical_device.GetNative(), logical_device.GetDispatcher())),
		m_rendering_completed(CreateSemaphore(logical_device.GetNative(), logical_device.GetDispatcher())),
		m_impl(VulkanSwapChain::CreateSwapChainImpl(physical_device, logical_device, present_queue, surface, buffer_size)),
		m_present_queue(present_queue.GetID()) {
	}

}