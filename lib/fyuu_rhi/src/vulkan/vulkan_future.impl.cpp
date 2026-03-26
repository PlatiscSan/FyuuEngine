/* vulkan_future_impl.cpp */
/**
 * @file vulkan_future_impl.cpp
 * @brief Implementation of VulkanPromise and VulkanFuture.
 *
 * This file contains the definitions of the Vulkan promise/future classes,
 * including semaphore and fence handling, and allocation strategies.
 */
module;
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <atomic>
#include <optional>
#include <variant>
#include <memory_resource>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:vulkan_future_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_future;
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_logical_device;
import :vulkan_command_queue;
import :vulkan_swap_chain;

namespace {
	using namespace fyuu_rhi::vulkan;

	/**
	 * @brief Memory pool for VulkanFuture allocations.
	 *
	 * A synchronized pool reduces allocation overhead and avoids contention.
	 */
	static std::pmr::synchronized_pool_resource m_future_pool{};

	/**
	 * @brief Polymorphic allocator for VulkanFuture, using the pool above.
	 */
	static std::pmr::polymorphic_allocator<VulkanFuture> s_future_alloc{ &m_future_pool };
}

namespace fyuu_rhi::vulkan {

	/**
	 * @brief Constructs a timeline semaphore from a logical device.
	 * @param logical_device The logical device used to create the semaphore.
	 */
	VulkanPromise::TimelineSemaphore::TimelineSemaphore(VulkanLogicalDevice const& logical_device)
		: impl(logical_device.CreateTimelineSemaphore()),
		  next_value(1u) {
	}

	/**
	 * @brief Move constructor for TimelineSemaphore.
	 * @param other Another TimelineSemaphore to move from.
	 */
	VulkanPromise::TimelineSemaphore::TimelineSemaphore(TimelineSemaphore&& other) noexcept
		: impl(std::move(other.impl)),
		  next_value(other.next_value.exchange(0u, std::memory_order::relaxed)) {
	}

	/**
	 * @brief Constructs a VulkanPromise using timeline semaphores.
	 * @param logical_device The logical device used to create the semaphores.
	 */
	VulkanPromise::VulkanPromise(VulkanLogicalDevice const& logical_device)
		: PolymorphicPromiseBase(this),
		  VulkanCommon(this),
		  m_logical_device_id(logical_device.GetID()),
		  m_binary_semaphore(logical_device.CreateBinarySemaphore()),
		  m_cpu_sync(std::in_place_type<TimelineSemaphore>, logical_device),
		  m_future(nullptr),
		  m_value(nullptr) {
	}

	/**
	 * @brief Constructs a VulkanPromise using a fence for synchronization.
	 * @param logical_device The logical device used to create the semaphore and fence.
	 * @param UseFence Tag to select fence-based constructor.
	 */
	VulkanPromise::VulkanPromise(VulkanLogicalDevice const& logical_device, UseFence)
		: PolymorphicPromiseBase(this),
		  VulkanCommon(this),
		  m_logical_device_id(logical_device.GetID()),
		  m_binary_semaphore(logical_device.CreateBinarySemaphore()),
		  m_cpu_sync(std::in_place_type<vk::UniqueFence>, logical_device.CreateFence()),
		  m_future(nullptr),
		  m_value(nullptr) {
	}

	/**
	 * @brief Move constructor for VulkanPromise.
	 * @param other Another VulkanPromise to move from.
	 */
	VulkanPromise::VulkanPromise(VulkanPromise&& other) noexcept
		: PolymorphicPromiseBase(std::move(other)),
		  VulkanCommon(std::move(other)),
		  m_logical_device_id(std::move(other.m_logical_device_id)),
		  m_binary_semaphore(std::move(other.m_binary_semaphore)),
		  m_cpu_sync(std::move(other.m_cpu_sync)),
		  m_future(other.m_future.exchange(nullptr, std::memory_order::relaxed)),
		  m_value(std::move(other.m_value)) {
	}

	/**
	 * @brief Signals the GPU work completion by submitting to a command queue.
	 *
	 * This function adds the binary semaphore and either a timeline semaphore or fence
	 * to the queue's submit info, and creates a new VulkanFuture associated with the signal.
	 *
	 * @param queue The Vulkan command queue on which to signal.
	 */
	void VulkanPromise::CommandQueueSignal(VulkanCommandQueue* queue) {
		// Add the binary semaphore to the submit info.
		VulkanSubmitInfoFiller submit_info_filler = queue->GetSubmitInfoFiller();
		submit_info_filler.signal_semaphores.emplace_back(*m_binary_semaphore);

		std::visit(
			[this, &submit_info_filler](auto&& cpu_sync) {
				using CPUSync = std::decay_t<decltype(cpu_sync)>;
				if constexpr (std::same_as<CPUSync, VulkanPromise::TimelineSemaphore>) {
					// Add timeline semaphore to signal list.
					submit_info_filler.signal_semaphores.emplace_back(*cpu_sync.impl);

					// Atomically increment the next timeline value for this promise.
					// The first signaled value will be 1, then 2, etc.
					std::uint64_t timeline_value = cpu_sync.next_value.fetch_add(1u, std::memory_order::relaxed);
					submit_info_filler.signal_values.emplace_back(timeline_value);

					// Create a new future object for this timeline value and store it in the promise.
					m_future.store(
						std::allocate_shared<VulkanFuture>(s_future_alloc, this, timeline_value),
						std::memory_order::release
					);
				}
				else {
					// Use fence synchronization.
					submit_info_filler.fence = *cpu_sync;

					// Create a new future object without timeline value.
					m_future.store(
						std::allocate_shared<VulkanFuture>(s_future_alloc, this),
						std::memory_order::release
					);
				}
			},
			m_cpu_sync
		);
	}

	/**
	 * @brief Signals the back buffer acquisition of a swap chain.
	 *
	 * Used when presenting to a swap chain; acquires the next back buffer index and
	 * signals using the binary semaphore and fence.
	 *
	 * @param logical_device The logical device associated with the swap chain.
	 * @param swap_chain The swap chain whose back buffer is being acquired.
	 * @return The index of the acquired back buffer, or std::nullopt for swap chain out of date
	 */
	std::optional<std::size_t> VulkanPromise::SwapChainBackBufferSignal(VulkanLogicalDevice* logical_device, VulkanSwapChain* swap_chain, bool block) {
		return std::visit(
			[this, logical_device, swap_chain, block](auto&& cpu_sync) -> std::optional<std::size_t> {
				using CPUSync = std::decay_t<decltype(cpu_sync)>;
				if constexpr (std::same_as<CPUSync, vk::UniqueFence>) {
					// Acquire next back buffer index using the binary semaphore and fence.

					auto native = swap_chain->GetNative();

					std::optional<std::size_t> index = logical_device->AcquireNextBackBufferIndex(
						native,
						*m_binary_semaphore,
						*cpu_sync,
						block ? std::numeric_limits<std::uint64_t>::max() : 0u
					);

					// Create a new future object and store it in the promise.
					m_future.store(
						std::allocate_shared<VulkanFuture>(s_future_alloc, this),
						std::memory_order::release
					);

					return index;
				}
				else {
					// Not fence-based; cannot acquire swap chain back buffer.
					throw std::runtime_error("Not fence-based, cannot acquire swap chain back buffer.");
				}
			},
			m_cpu_sync
		);
	}

	void VulkanPromise::PresentingSignal(vk::SwapchainPresentFenceInfoEXT& info) {
		std::visit(
			[this, &info](auto&& cpu_sync) {
				using CPUSync = std::decay_t<decltype(cpu_sync)>;
				if constexpr (std::same_as<CPUSync, vk::UniqueFence>) {

					info.pFences = &*cpu_sync;

					// Create a new future object and store it in the promise.
					m_future.store(
						std::allocate_shared<VulkanFuture>(s_future_alloc, this),
						std::memory_order::release
					);
					
				}
				else {
					// Not fence-based; cannot present.
					throw std::runtime_error("Not fence-based, cannot present.");
				}
			},
			m_cpu_sync
		);	
	}

	/**
	 * @brief Retrieves the future associated with the next signaled operation.
	 *
	 * After this call, the promise's internal future pointer is reset to nullptr.
	 *
	 * @return Shared pointer to the VulkanFuture, or nullptr if none exists.
	 */
	std::shared_ptr<VulkanFuture> VulkanPromise::GetFuture() noexcept {
		// Exchange the stored future with nullptr and return the old value.
		// This ensures that each future can only be retrieved once.
		return m_future.exchange(nullptr, std::memory_order::release);
	}

	/**
	 * @brief Returns the ID of the logical device that owns this promise.
	 * @return Optional containing the device ID, or std::nullopt if not owned.
	 */
	std::optional<std::size_t> VulkanPromise::GetOwnerDeviceID() const noexcept {
		return m_logical_device_id;
	}

	/**
	 * @brief Returns the raw Vulkan binary semaphore handle.
	 * @return vk::Semaphore handle (borrowed, not owned).
	 */
	vk::Semaphore VulkanPromise::GetBinarySemaphore() const noexcept {
		return *m_binary_semaphore;
	}

	/**
	 * @brief Returns the raw Vulkan timeline semaphore handle, if available.
	 * @return vk::Semaphore handle, or nullptr if the promise uses a fence.
	 */
	vk::Semaphore VulkanPromise::GetTimelineSemaphore() const noexcept {
		if (TimelineSemaphore const* timeline_semaphore = std::get_if<TimelineSemaphore>(&m_cpu_sync)) {
			return *timeline_semaphore->impl;
		}
		return nullptr;
	}

	/**
	 * @brief Returns the raw Vulkan fence handle, if available.
	 * @return vk::Fence handle, or nullptr if the promise uses a timeline semaphore.
	 */
	vk::Fence VulkanPromise::GetFence() const noexcept {
		if (vk::UniqueFence const* fence = std::get_if<vk::UniqueFence>(&m_cpu_sync)) {
			return **fence;
		}
		return nullptr;
	}

	/**
	 * @brief Constructs a future from a promise and an optional timeline value.
	 *
	 * If a timeline value is provided, the future waits on the timeline semaphore;
	 * otherwise, it waits on the fence.
	 *
	 * @param promise Pointer to the associated VulkanPromise.
	 * @param timeline_value Optional timeline value for timeline semaphore synchronization.
	 */
	VulkanFuture::VulkanFuture(VulkanPromise* promise, std::optional<std::uint64_t> const& timeline_value)
		: PolymorphicFutureBase(this),
		VulkanCommon(this),
		m_logical_device_id(promise->GetOwnerDeviceID()),
		m_binary_semaphore(promise->GetBinarySemaphore()),
		m_cpu_sync(
			[promise, &timeline_value]() -> std::variant<TimelineSemaphore, Fence> {
				if (timeline_value) {
					return TimelineSemaphore{ promise->GetTimelineSemaphore(), *timeline_value };
				} else {
					return Fence{ promise->GetFence(), std::make_unique<Fence::ResetControlBlock>() };
				}
			  }()),
		m_value(std::move(promise->m_value)) { // Transfer ownership of the stored value from promise to future
	}

	/**
	 * @brief Blocks until the GPU work associated with this future completes.
	 *
	 * Uses the logical device to wait on the appropriate synchronization primitive.
	 */
	void VulkanFuture::Wait() {
		if (!m_logical_device_id) {
			return; // No device ID means nothing to wait for (e.g., future was empty)
		}

		// Retrieve the logical device object from its ID.
		VulkanLogicalDevice* logical_device = plastic::utility::QueryObject<VulkanLogicalDevice>(*m_logical_device_id);
		if (!logical_device) {
			return; // Device no longer exists?
		}

		std::visit(
			[logical_device](auto&& cpu_sync) {
				using CPUSync = std::decay_t<decltype(cpu_sync)>;
				if constexpr (std::same_as<CPUSync, VulkanFuture::TimelineSemaphore>) {
					// Prepare wait info for timeline semaphore.
					std::array semaphores = { cpu_sync.impl };
					std::array values = { cpu_sync.timeline_value };

					vk::SemaphoreWaitInfo info(
						{},					 // Flags (none)
						semaphores,			 // Semaphores to wait on
						values,				 // Corresponding wait values
						nullptr				 // Optional custom allocator
					);

					// Perform the wait on the logical device.
					logical_device->WaitForSemaphores(info);
				} else {

					auto& control_block = *cpu_sync.control_block;

					std::call_once(
						control_block.once,
						[&cpu_sync, logical_device, &control_block]() {
							logical_device->WaitForFence(cpu_sync.impl);
							logical_device->ResetFence(cpu_sync.impl);
							control_block.ok.test_and_set(std::memory_order::release);
							control_block.ok.notify_all();
						}
					);

					while (!control_block.ok.test(std::memory_order::relaxed)) {
						control_block.ok.wait(false, std::memory_order::relaxed);
					}

				}
			},
			m_cpu_sync
		);
	}

	/**
	 * @brief Returns the binary semaphore handle (borrowed).
	 * @return vk::Semaphore handle.
	 */
	vk::Semaphore VulkanFuture::GetBinarySemaphore() const noexcept {
		return m_binary_semaphore;
	}

	/**
	 * @brief Returns the timeline semaphore handle, if available.
	 * @return vk::Semaphore handle, or nullptr if this future uses a fence.
	 */
	vk::Semaphore VulkanFuture::GetTimelineSemaphore() const noexcept {
		if (TimelineSemaphore const* timeline_semaphore = std::get_if<TimelineSemaphore>(&m_cpu_sync)) {
			return timeline_semaphore->impl;
		}
		return nullptr;
	}

	/**
	 * @brief Returns the timeline value this future waits for, if applicable.
	 * @return Optional containing the timeline value, or std::nullopt if fence-based.
	 */
	std::optional<std::uint64_t> VulkanFuture::GetTimelineValue() const noexcept {
		if (TimelineSemaphore const* timeline_semaphore = std::get_if<TimelineSemaphore>(&m_cpu_sync)) {
			return timeline_semaphore->timeline_value;
		}
		return std::nullopt;
	}

	/**
	 * @brief Returns the fence handle, if available.
	 * @return vk::Fence handle, or nullptr if this future uses a timeline semaphore.
	 */
	vk::Fence VulkanFuture::GetFence() const noexcept {
		if (Fence const* fence = std::get_if<Fence>(&m_cpu_sync)) {
			return fence->impl;
		}
		return nullptr;
	}

} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)