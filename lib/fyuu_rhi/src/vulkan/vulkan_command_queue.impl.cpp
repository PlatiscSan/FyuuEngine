/* vulkan_command_queue.impl.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <mutex>
#include <vector>
#include <optional>
#include <format>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:vulkan_command_queue_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_command_queue;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_logical_device;
import :vulkan_queue_allocator;
import :vulkan_future;
import :vulkan_command_buffer;

namespace fyuu_rhi::vulkan {

	constexpr VulkanSubmitInfoFiller::VulkanSubmitInfoFiller(
		std::vector<vk::Semaphore>& signal_semaphores_, 
		std::vector<std::uint64_t>& signal_values_,
		vk::Fence& fence_
	) noexcept
		: signal_semaphores(signal_semaphores_),
        signal_values(signal_values_),
		fence(fence_) {
	}

    VulkanCommandQueue::VulkanCommandQueue(VulkanLogicalDevice& logical_device, CommandObjectType type, QueuePriority priority)
		: PolymorphicCommandQueueBase(this),
		VulkanCommon(this), 
		m_logical_device_id(logical_device.GetID()),
		m_queue_info(logical_device.AllocateCommandQueue(type, priority)),
		m_impl(
			logical_device.GetNative().getQueue(
				m_queue_info->family,
				m_queue_info->index,
				logical_device.GetRawDispatcher()
			)
		),
		m_dispatcher(logical_device.GetDispatcher()),
		m_internal_promise(logical_device),
		m_submit_mutex(),
		m_signal_semaphores(),
		m_signal_values(),
		m_fence() {
	}

	std::uint32_t VulkanCommandQueue::GetFamily() const noexcept {
		return m_queue_info->family;
	}

	std::uint32_t VulkanCommandQueue::GetIndex() const noexcept {
		return m_queue_info->index;
	}

	vk::Queue VulkanCommandQueue::GetNative() const noexcept {
		return m_impl;
	}

	VulkanSubmitInfoFiller VulkanCommandQueue::GetSubmitInfoFiller() noexcept {
		return { m_signal_semaphores, m_signal_values, m_fence };
	}

	void VulkanCommandQueue::WaitUntilCompleted() const {
		m_impl.waitIdle(*m_dispatcher);
	}

	void VulkanCommandQueue::ExecuteCommand(VulkanCommandBuffer& command_buffer, VulkanPromise& promise, std::shared_ptr<VulkanFuture> const& future) {

		std::lock_guard<std::mutex> lock(m_submit_mutex);

		m_signal_semaphores.clear();
		m_signal_values.clear();
		m_fence = nullptr;

		vk::TimelineSemaphoreSubmitInfo timeline_submit_info{};
		vk::SubmitInfo submit_info{};

		std::array cmds = { command_buffer.GetNative() };

		std::array<vk::Semaphore, 2u> wait_semaphores;
		std::array<std::uint64_t, 1u> wait_timeline_values;

		if (future) {

			wait_semaphores[0] = future->GetBinarySemaphore();
			
			vk::Semaphore timeline_semaphore = future->GetTimelineSemaphore();
			std::optional<std::uint64_t> timeline_value = future->GetTimelineValue();
			if (timeline_semaphore && timeline_value) {
				wait_semaphores[1] = timeline_semaphore;
				wait_timeline_values[0] = *timeline_value;
			}

			timeline_submit_info.setWaitSemaphoreValues(wait_timeline_values);

			submit_info.setWaitSemaphores(wait_semaphores);
			submit_info.setPNext(&timeline_submit_info);

		}

		submit_info.setCommandBuffers(cmds);

		// promise fill up the necessary info and signal the value
		promise.CommandQueueSignal(this);

		submit_info.setSignalSemaphores(m_signal_semaphores);
		timeline_submit_info.setSignalSemaphoreValues(m_signal_values);

		vk::Result result = m_impl.submit(
			1,
			&submit_info,
			m_fence,
			*m_dispatcher
		);

		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Queue::submit failed, result: {}", static_cast<std::size_t>(result)));
		}	

	}

	std::shared_ptr<VulkanFuture> VulkanCommandQueue::ExecuteCommand(VulkanCommandBuffer& command_buffer, std::shared_ptr<VulkanFuture> const& future) {
		ExecuteCommand(command_buffer, m_internal_promise, future);
		return m_internal_promise.GetFuture();
	}

	bool VulkanCommandQueue::Present(vk::PresentInfoKHR const& info) const {
		vk::Result result = m_impl.presentKHR(info, *m_dispatcher);
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
			return false;
		}
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Queue::submit failed, result: {}", static_cast<std::size_t>(result)));
		}
		return true;
    }

} // namespace fyuu_rhi::vulkan


#endif // !defined(__APPLE__)