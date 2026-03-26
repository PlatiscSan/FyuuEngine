/* webgpu_future.cppm */
/**
 * @file webgpu_future.cppm
 * @brief WebGPU module partition providing Promise and Future implementations.
 *
 * This module partition defines WebGPU-specific Promise and Future classes
 * using Dawn's C++ bindings. It enables synchronization of GPU command queue
 * submissions and retrieval of asynchronous results via the OnSubmittedWorkDone
 * callback mechanism.
 */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <future>
#include <memory_resource>
#endif
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_future;

#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :webgpu_common;
import :webgpu_logical_device;

namespace {
	/**
	 * @brief Internal interface for type-erased storage of a value set on a promise.
	 *
	 * The Get() method copies the stored value into the provided buffer.
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

namespace fyuu_rhi::webgpu {

	/**
	 * @brief WebGPU-specific promise that can be fulfilled by a GPU operation.
	 *
	 * Manages a WebGPU device (for waiting) and a type-erased value to be delivered
	 * to the associated future. The promise uses the OnSubmittedWorkDone callback
	 * mechanism of wgpu::Queue to detect completion.
	 */
	export class WebGPUPromise
		: public PolymorphicPromiseBase,
		  public WebGPUCommon<WebGPUPromise> {
		friend class WebGPUCommandQueueSignal;
		friend class WebGPUFuture;
	private:
		wgpu::Device m_device;							///< The logical device (needed for ProcessEvents).
		std::atomic<std::shared_ptr<WebGPUFuture>> m_future; ///< Most recently created future.
		std::unique_ptr<IValue> m_value;					///< Type-erased value set on this promise.

	public:
		/**
		 * @brief Constructs a promise using the given logical device.
		 * @param logical_device Reference to the WebGPULogicalDevice that provides the device.
		 */
		explicit WebGPUPromise(WebGPULogicalDevice const& logical_device);

		/**
		 * @brief Retrieves and clears the most recent future associated with this promise.
		 * @return Shared pointer to the most recent WebGPUFuture, or nullptr if none exists.
		 */
		std::shared_ptr<WebGPUFuture> GetFuture() noexcept;

		/**
		 * @brief Signals the promise on a command queue and creates a new future.
		 *
		 * This function creates a WebGPUCommandQueueSignal callable and invokes it
		 * on the provided queue, which registers an OnSubmittedWorkDone callback
		 * and stores the resulting future.
		 *
		 * @param queue Pointer to the WebGPUCommandQueue on which to signal.
		 */
		void CommandQueueSignal(WebGPUCommandQueue* queue) noexcept;

		/**
		 * @brief Returns the native Dawn device.
		 * @return wgpu::Device handle.
		 */
		wgpu::Device GetDevice() const noexcept;

		/**
		 * @brief Sets the value that will be made available through the future.
		 *
		 * The value is type-erased by wrapping it in a derived IValue.
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
	 * @brief WebGPU-specific future that holds the result of an asynchronous GPU operation.
	 *
	 * Waits on a WebGPU completion callback and provides access to the promised value.
	 */
	export class WebGPUFuture
		: public PolymorphicFutureBase,
		  public WebGPUCommon<WebGPUFuture> {
	private:
		wgpu::Device m_device;							  ///< Device for event processing.
		std::future<void> m_completion_future;			  ///< Future for callback synchronization.
		std::unique_ptr<IValue> m_value;					///< Type-erased promised value.

	public:
		/**
		 * @brief Constructs a future from a promise and a completion std::future.
		 *
		 * The completion future is obtained from the std::promise<void> set in the
		 * OnSubmittedWorkDone callback.
		 *
		 * @param promise Pointer to the associated WebGPUPromise.
		 * @param completion_future Rvalue reference to the std::future<void> that signals completion.
		 */
		WebGPUFuture(WebGPUPromise* promise, std::future<void>&& completion_future);

		/**
		 * @brief Blocks until the GPU operation completes.
		 *
		 * Calls wait() on the underlying std::future<void>.
		 */
		void Wait() const;

		/**
		 * @brief Retrieves the promised value (moves it out of the future) and clears the stored value.
		 *
		 * The caller must specify the expected type. Blocks until the value is ready.
		 *
		 * @tparam Value Expected type of the promised value.
		 * @return The promised value.
		 */
		template <class Value>
		Value Get() {
			Wait(); // Ensure completion before accessing value
			Value buffer;
			if (m_value) {
				m_value->Get(&buffer);
				m_value = nullptr;
			}
			return buffer;
		}
	};
}