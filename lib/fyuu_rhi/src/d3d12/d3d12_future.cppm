/* d3d12_future.cppm */
/**
 * @file d3d12_future.cppm
 * @brief D3D12 module partition providing Promise and Future implementations.
 *
 * This module partition defines D3D12-specific Promise and Future classes,
 * enabling synchronization of GPU command queues and retrieval of asynchronous results.
 */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <memory_resource>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_future;

#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;	
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_types;
import :d3d12_logical_device;
import :d3d12_common;

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

namespace fyuu_rhi::d3d12 {

	/**
	 * @brief D3D12-specific promise that can be fulfilled by a GPU operation.
	 *
	 * Manages a D3D12 fence and a type-erased value to be delivered to the associated future.
	 */
	export class D3D12Promise
		: public PolymorphicPromiseBase,
		public D3D12Common<D3D12Promise> {
		friend class D3D12CommandQueueSignal;
		friend class D3D12Future;
	private:
		Microsoft::WRL::ComPtr<ID3D12Fence1> m_impl;		  ///< Underlying D3D12 fence.
		std::atomic_uint64_t m_next_fence_value;			  ///< Next fence value to use for signaling.
		std::atomic<std::shared_ptr<D3D12Future>> m_future;	///< Most recently created future associated with this promise.
		std::unique_ptr<IValue> m_value;						///< Type-erased value set on this promise.

	public:
		/**
		 * @brief Constructs a promise using the given logical device.
		 * @param logical_device Reference to a D3D12LogicalDevice used to create the fence.
		 */
		explicit D3D12Promise(D3D12LogicalDevice const& logical_device);

		/**
		 * @brief Retrieves and clears the most recent future associated with this promise.
		 * @return Shared pointer to the most recent D3D12Future, or nullptr if none exists.
		 */
		std::shared_ptr<D3D12Future> GetFuture() noexcept;

		/**
		 * @brief Signals the fence on the given command queue and creates a new future.
		 * @param queue Pointer to the D3D12CommandQueue on which to signal.
		 * @return fence value.
		 */
		std::uint64_t CommandQueueSignal(D3D12CommandQueue* queue) noexcept;

		bool IsSignaled(std::uint64_t value) const noexcept;

		/**
		 * @brief Returns a ComPtr to the native D3D12 fence.
		 * @return ComPtr<ID3D12Fence1> holding the fence.
		 */
		Microsoft::WRL::ComPtr<ID3D12Fence1> GetNative() const noexcept;

		/**
		 * @brief Returns a raw pointer to the native D3D12 fence.
		 * @return Raw ID3D12Fence1 pointer.
		 */
		ID3D12Fence1* GetRawNative() const noexcept;

		/**
		 * @brief Sets the value that will be made available through the future.
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
	 * @brief D3D12-specific future that holds the result of an asynchronous GPU operation.
	 *
	 * Waits on a D3D12 fence and provides access to the promised value.
	 */
	export class D3D12Future
		: public PolymorphicFutureBase,
		public D3D12Common<D3D12Future> {
	private:
		Microsoft::WRL::ComPtr<ID3D12Fence1> m_impl;	///< Fence to wait on.
		EventHandle m_event_handle;						///< Event signaled when the fence reaches the expected value.
		std::uint64_t m_fence_value;					///< Fence value that indicates completion.
		std::unique_ptr<IValue> m_value;				///< Type-erased promised value.

	public:
		/**
		 * @brief Constructs a future from a promise and the specific fence value to wait for.
		 * @param promise Pointer to the associated D3D12Promise.
		 * @param value	Fence value that must be reached for completion.
		 */
		D3D12Future(D3D12Promise* promise, std::uint64_t value);

		/**
		 * @brief Blocks until the fence has reached the expected value.
		 */
		void Wait() const;

		/**
		 * @brief Retrieves the promised value (moves it out of the future) and clears the stored value.
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
	
#endif // defined(_WIN32)