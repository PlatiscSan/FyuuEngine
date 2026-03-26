/* vulkan_swap_chain.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <tuple>
#include <vector>
#include <optional>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_swap_chain;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_resource;
import :vulkan_view;
import :vulkan_future;

namespace fyuu_rhi::vulkan {

	export class VulkanSwapChain
		: public PolymorphicSwapChainBase,
		public VulkanCommon<VulkanSwapChain> {
	private:
		struct Frame {
			VulkanResource texture;
			VulkanView view;
			std::shared_ptr<VulkanFuture> future;
			
			Frame(VulkanLogicalDevice const& logical_device, vk::Image back_buffer, vk::Format back_buffer_format, std::size_t width, std::size_t height, std::size_t index);

		};

		struct Promise {
			VulkanPromise impl;
			std::optional<std::size_t> acquired_frame_index;

			Promise(VulkanLogicalDevice const& logical_device);
		};

		std::optional<std::size_t> m_physical_device_id;
		std::optional<std::size_t> m_logical_device_id;
		std::optional<std::size_t> m_present_queue_id;
		std::optional<std::size_t> m_surface_id;
		SwapChainOption m_option;

		std::tuple<vk::UniqueSwapchainKHR, vk::Format, vk::Extent2D> m_swap_chain_bundle;
		std::vector<vk::PresentModeKHR> m_allowed_modes;
		vk::PresentModeKHR m_main_present_mode;
		std::vector<Frame> m_frames;
		std::vector<Promise> m_internal_promises;
		std::size_t m_next_promise_index;
		std::optional<std::size_t> m_current_promise_index;

		void CreateSwapChain(
			VulkanPhysicalDevice const& physical_device,
			VulkanLogicalDevice const& logical_device,
			VulkanCommandQueue const& present_queue,
			VulkanSurface const& surface,
			std::size_t back_buffer_size
		);

		void CreateRenderTarget(
			VulkanLogicalDevice const& logical_device,
			std::size_t back_buffer_size
		);

		void Resize(
			VulkanPhysicalDevice const& physical_device,
			VulkanLogicalDevice const& logical_device,
			VulkanCommandQueue const& present_queue,
			VulkanSurface const& surface,
			std::size_t back_buffer_size
		);

	public:
		VulkanSwapChain(
			VulkanPhysicalDevice const& physical_device,
			VulkanLogicalDevice const& logical_device,
			VulkanCommandQueue const& present_queue,
			VulkanSurface const& surface,
			SwapChainOption const& swap_chain_option
		);

		~VulkanSwapChain() noexcept;

		void Resize(std::optional<std::size_t> const& back_buffer_size = std::nullopt);

		std::tuple<VulkanResource*, VulkanView*, std::shared_ptr<VulkanFuture>> GetNextFrame();

		void Present(std::shared_ptr<VulkanFuture> const& future = nullptr);

		void SetVSync(bool mode);

		vk::SwapchainKHR GetNative() const noexcept;

	};

}

#endif // !defined(__APPLE__)