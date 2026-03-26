/* vulkan_command_queue.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <mutex>
#include <vector>
#include <optional>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_command_queue;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_queue_allocator;
import :vulkan_future;

namespace fyuu_rhi::vulkan {
	export class VulkanSubmitInfoFiller {
		friend class VulkanCommandQueue;
		friend class VulkanPromise;
	protected:
		std::vector<vk::Semaphore>& signal_semaphores;
		std::vector<std::uint64_t>& signal_values;
		vk::Fence& fence;

		constexpr VulkanSubmitInfoFiller(
			std::vector<vk::Semaphore>& signal_semaphores_,
			std::vector<std::uint64_t>& signal_values_,
			vk::Fence& fence_
		) noexcept;

	};

	export class VulkanCommandQueue
		: public PolymorphicCommandQueueBase,
		public VulkanCommon<VulkanCommandQueue> {
		friend class VulkanSubmitInfoFiller;
	private:
		std::optional<std::size_t> m_logical_device_id;
		UniqueVulkanCommandQueueInfo m_queue_info;
		vk::Queue m_impl;
		std::shared_ptr<vk::DispatchLoaderDynamic> m_dispatcher;
		VulkanPromise m_internal_promise;

		std::mutex m_submit_mutex;
		std::vector<vk::Semaphore> m_signal_semaphores;
		std::vector<std::uint64_t> m_signal_values;
		vk::Fence m_fence;
		
	public:
		VulkanCommandQueue(
			VulkanLogicalDevice& logical_device,
			CommandObjectType type,
			QueuePriority priority
		);

		std::uint32_t GetFamily() const noexcept;
		std::uint32_t GetIndex() const noexcept;

		vk::Queue GetNative() const noexcept;

		VulkanSubmitInfoFiller GetSubmitInfoFiller() noexcept;

		void WaitUntilCompleted() const;

		void ExecuteCommand(VulkanCommandBuffer& command_buffer, VulkanPromise& promise, std::shared_ptr<VulkanFuture> const& future = nullptr);
		
		std::shared_ptr<VulkanFuture> ExecuteCommand(VulkanCommandBuffer& command_buffer, std::shared_ptr<VulkanFuture> const& future = nullptr);

		bool Present(vk::PresentInfoKHR const& info) const;

	};
} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)