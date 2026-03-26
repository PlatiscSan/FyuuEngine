/* vulkan_future.cppm */
/**
 * @file vulkan_future.cppm
 * @brief Vulkan module partition providing Promise and Future implementations.
 *
 * This module partition defines Vulkan-specific Promise and Future classes,
 * enabling synchronization of GPU command queues and retrieval of asynchronous results.
 */
module;
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <optional>
#include <variant>
#include <memory_resource>
#include <mutex>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_future;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_logical_device;

namespace {
	/**
	 * @brief Internal interface for type-erased storage of a value set on a promise.
	 *
	 * The Get() method copies the stored value into the provided buffer.
	 * Used by VulkanPromise to store the promised value and by VulkanFuture to retrieve it.
	 */
	struct IValue {
		/**
		 * @brief Copies the stored value into the provided buffer.
		 * @param buffer Pointer to the destination memory. Must be large enough to hold the value.
		 */
		virtual void Get(void* buffer) = 0;
		virtual ~IValue() noexcept = default;
	};
}

namespace fyuu_rhi::vulkan {

	/**
	 * @brief Vulkan-specific promise that can be fulfilled by a GPU operation.
	 *
	 * Holds Vulkan semaphores (binary and timeline) to synchronize GPU work.
	 * The promise can be used to create a future (VulkanFuture) that will be signaled
	 * when a command queue submission completes.
	 */
	export class VulkanPromise
		: public PolymorphicPromiseBase,
		  public VulkanCommon<VulkanPromise> {
		friend class VulkanCommandQueueSignal;
		friend class VulkanFuture;

	private:
		/**
		 * @brief Internal structure representing a timeline semaphore with its next value.
		 */
		struct TimelineSemaphore {
			vk::UniqueSemaphore impl;	  ///< Vulkan timeline semaphore handle.
			std::atomic_uint64_t next_value; ///< Next timeline value to use for signaling.

			/**
			 * @brief Constructs a timeline semaphore from a logical device.
			 * @param logical_device The logical device used to create the semaphore.
			 */
			TimelineSemaphore(VulkanLogicalDevice const& logical_device);

			/**
			 * @brief Move constructor.
			 * @param other Another TimelineSemaphore to move from.
			 */
			TimelineSemaphore(TimelineSemaphore&& other) noexcept;
		};

		std::optional<std::size_t> m_logical_device_id; ///< ID of the logical device that owns this promise.
		vk::UniqueSemaphore m_binary_semaphore;		 ///< Binary semaphore for simple wait/signal operations.

		/**
		 * @brief CPU-side synchronization primitive.
		 *
		 * Can be either a timeline semaphore (with next value) or a fence.
		 */
		std::variant<TimelineSemaphore, vk::UniqueFence> m_cpu_sync;

		std::atomic<std::shared_ptr<VulkanFuture>> m_future; ///< Future object associated with the next signal.
		std::unique_ptr<IValue> m_value;					  ///< Type-erased value to be delivered to the future.

	public:
		/**
		 * @brief Tag type to select fence-based synchronization in constructor.
		 */
		struct UseFence {};

		/**
		 * @brief Constructs a promise using timeline semaphores for synchronization.
		 * @param logical_device The logical device used to create the semaphores.
		 */
		explicit VulkanPromise(VulkanLogicalDevice const& logical_device);

		/**
		 * @brief Constructs a promise using a fence for synchronization.
		 * @param logical_device The logical device used to create the semaphore and fence.
		 * @param UseFence Tag to select fence-based constructor.
		 */
		VulkanPromise(VulkanLogicalDevice const& logical_device, UseFence);

		/**
		 * @brief Move constructor.
		 * @param other Another VulkanPromise to move from.
		 */
		VulkanPromise(VulkanPromise&& other) noexcept;

		/**
		 * @brief Signals the GPU work completion by submitting to a command queue.
		 *
		 * This function adds the binary semaphore and either a timeline semaphore or fence
		 * to the queue's submit info, and creates a new VulkanFuture associated with the signal.
		 *
		 * @param queue The Vulkan command queue on which to signal.
		 */
		void CommandQueueSignal(VulkanCommandQueue* queue);

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
		std::optional<std::size_t> SwapChainBackBufferSignal(VulkanLogicalDevice* logical_device, VulkanSwapChain* swap_chain, bool block = false);

		void PresentingSignal(vk::SwapchainPresentFenceInfoEXT& info);

		/**
		 * @brief Retrieves the future associated with the next signaled operation.
		 *
		 * After this call, the promise's internal future pointer is reset to nullptr.
		 *
		 * @return Shared pointer to the VulkanFuture, or nullptr if none exists.
		 */
		std::shared_ptr<VulkanFuture> GetFuture() noexcept;

		/**
		 * @brief Returns the ID of the logical device that owns this promise.
		 * @return Optional containing the device ID, or std::nullopt if not owned.
		 */
		std::optional<std::size_t> GetOwnerDeviceID() const noexcept;

		/**
		 * @brief Returns the raw Vulkan binary semaphore handle.
		 * @return vk::Semaphore handle (borrowed, not owned).
		 */
		vk::Semaphore GetBinarySemaphore() const noexcept;

		/**
		 * @brief Returns the raw Vulkan timeline semaphore handle, if available.
		 * @return vk::Semaphore handle, or nullptr if the promise uses a fence.
		 */
		vk::Semaphore GetTimelineSemaphore() const noexcept;

		/**
		 * @brief Returns the raw Vulkan fence handle, if available.
		 * @return vk::Fence handle, or nullptr if the promise uses a timeline semaphore.
		 */
		vk::Fence GetFence() const noexcept;

		/**
		 * @brief Stores a value of arbitrary type inside the promise.
		 *
		 * The value is type-erased by wrapping it in a derived IValue and will be moved
		 * into the future when the promise is signaled.
		 *
		 * @tparam Value Type of the value to store.
		 * @param value The value to store (moved into the promise).
		 */
		template <class Value>
		void SetValue(Value&& value) {
			struct ValueImpl final : public IValue {
				Value value;
				void Get(void* buffer) override {
					*static_cast<Value*>(buffer) = std::move(value);
				}
			};
			m_value = std::make_unique<ValueImpl>(std::forward<Value>(value));
		}
	};

	/**
	 * @brief Vulkan-specific future that holds the result of an asynchronous GPU operation.
	 *
	 * Allows waiting for GPU work to complete and retrieving the result value.
	 */
	export class VulkanFuture
		: public PolymorphicFutureBase,
		  public VulkanCommon<VulkanFuture> {
	private:
		/**
		 * @brief Internal structure representing a timeline semaphore and the value to wait for.
		 */
		struct TimelineSemaphore {
			vk::Semaphore impl;		  ///< Vulkan timeline semaphore handle (borrowed).
			std::uint64_t timeline_value; ///< Value the semaphore must reach.
		};

		struct Fence {
			struct ResetControlBlock {
				std::once_flag once;
				std::atomic_flag ok;
			};
			vk::Fence impl;
			std::unique_ptr<ResetControlBlock> control_block;
		};

		std::optional<std::size_t> m_logical_device_id; ///< ID of the logical device used for waiting.
		vk::Semaphore m_binary_semaphore;			   ///< Binary semaphore handle (borrowed from promise).

		/**
		 * @brief CPU-side synchronization primitive to wait on.
		 *
		 * Can be either a timeline semaphore (with a value) or a fence.
		 */
		std::variant<TimelineSemaphore, Fence> m_cpu_sync;

		std::unique_ptr<IValue> m_value; ///< Type-erased result value (moved from the promise).

	public:
		/**
		 * @brief Constructs a future from a promise and an optional timeline value.
		 *
		 * If a timeline value is provided, the future waits on the timeline semaphore;
		 * otherwise, it waits on the fence.
		 *
		 * @param promise Pointer to the associated VulkanPromise.
		 * @param timeline_value Optional timeline value for timeline semaphore synchronization.
		 */
		VulkanFuture(VulkanPromise* promise, std::optional<std::uint64_t> const& timeline_value = std::nullopt);

		/**
		 * @brief Blocks until the GPU work associated with this future completes.
		 *
		 * Uses the logical device to wait on the appropriate synchronization primitive.
		 */
		void Wait();

		/**
		 * @brief Returns the binary semaphore handle (borrowed).
		 * @return vk::Semaphore handle.
		 */
		vk::Semaphore GetBinarySemaphore() const noexcept;

		/**
		 * @brief Returns the timeline semaphore handle, if available.
		 * @return vk::Semaphore handle, or nullptr if this future uses a fence.
		 */
		vk::Semaphore GetTimelineSemaphore() const noexcept;

		/**
		 * @brief Returns the timeline value this future waits for, if applicable.
		 * @return Optional containing the timeline value, or std::nullopt if fence-based.
		 */
		std::optional<std::uint64_t> GetTimelineValue() const noexcept;

		/**
		 * @brief Returns the fence handle, if available.
		 * @return vk::Fence handle, or nullptr if this future uses a timeline semaphore.
		 */
		vk::Fence GetFence() const noexcept;

		/**
		 * @brief Retrieves the result value of type Value.
		 *
		 * The value is moved out of the future, and the internal storage is cleared.
		 *
		 * @tparam Value Expected type of the promised value.
		 * @return The promised value.
		 */
		template <class Value>
		Value Get() {
			Wait(); // Ensure completion before accessing value
			Value buffer;
			m_value->Get(&buffer);
			m_value = nullptr;
			return buffer;
		}
	};
}
	
#endif // !defined(__APPLE__)