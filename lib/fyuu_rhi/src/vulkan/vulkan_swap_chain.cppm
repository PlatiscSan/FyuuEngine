export module fyuu_rhi:vulkan_swap_chain;
import vulkan_hpp;
import plastic.any_pointer;
import :vulkan_declaration;
import :swap_chain;
import plastic.any_pointer;

namespace fyuu_rhi::vulkan {

	export class VulkanSwapChain
		: plastic::utility::EnableSharedFromThis<VulkanSwapChain>,
		public ISwapChain {
		friend class ISwapChain;

	private:
		struct SwapChain {
			vk::UniqueSwapchainKHR vk_swap_chain;
			vk::Format image_format;
			vk::Extent2D extent;
			std::uint32_t buffer_size;
			bool v_sync = true;
		};

		vk::UniqueSemaphore m_image_available;
		vk::UniqueSemaphore m_rendering_completed;
		SwapChain m_impl;
		std::size_t m_present_queue;

		static SwapChain CreateSwapChainImpl(
			VulkanPhysicalDevice const& physical_device,
			VulkanLogicalDevice const& logical_device,
			VulkanCommandQueue const& present_queue,
			VulkanSurface const& surface,
			std::uint32_t buffer_size
		);

		void ResizeImpl(std::uint32_t width, std::uint32_t height);

	public:
		VulkanSwapChain(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			plastic::utility::AnyPointer<VulkanCommandQueue> const& present_queue,
			plastic::utility::AnyPointer<VulkanSurface> const& surface,
			std::uint32_t buffer_size = 3u
		);

		VulkanSwapChain(
			VulkanPhysicalDevice const& physical_device,
			VulkanLogicalDevice const& logical_device,
			VulkanCommandQueue const& present_queue,
			VulkanSurface const& surface,
			std::uint32_t buffer_size = 3u
		);
	};

}