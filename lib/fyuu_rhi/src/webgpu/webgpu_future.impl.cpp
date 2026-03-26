/* webgpu_future_impl.cpp */
/**
 * @file webgpu_future_impl.cpp
 * @brief Implementation of WebGPUPromise and WebGPUFuture.
 *
 * This file contains the definitions of the WebGPU promise/future classes,
 * including the callback handling and allocation strategies.
 */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <atomic>
#include <future>
#include <memory_resource>
#include <format>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_future_impl;

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_future;
import :types;
import :webgpu_common;
import :webgpu_logical_device;
import :webgpu_command_queue;

namespace {
	using namespace fyuu_rhi::webgpu;

	/**
	 * @brief Memory pool for WebGPUFuture allocations.
	 *
	 * A synchronized pool reduces allocation overhead and avoids contention.
	 */
	static std::pmr::synchronized_pool_resource m_future_pool{};

	/**
	 * @brief Polymorphic allocator for WebGPUFuture, using the pool above.
	 */
	static std::pmr::polymorphic_allocator<WebGPUFuture> s_future_alloc{ &m_future_pool };
}

namespace fyuu_rhi::webgpu {

	/**
	 * @brief Constructs a WebGPUPromise.
	 *
	 * Stores the native device from the logical device for later use in waiting.
	 *
	 * @param logical_device The logical device providing the wgpu::Device.
	 */
	WebGPUPromise::WebGPUPromise(WebGPULogicalDevice const& logical_device)
		: PolymorphicPromiseBase(this), WebGPUCommon(this), m_device(logical_device.GetNative()), m_future(nullptr) {
	}

	/**
	 * @brief Atomically exchanges the stored future with nullptr and returns it.
	 * @return The most recent future, or nullptr.
	 */
	std::shared_ptr<WebGPUFuture> WebGPUPromise::GetFuture() noexcept {
		return m_future.exchange(nullptr, std::memory_order::acq_rel);
	}

	/**
	 * @brief Signals the promise on a command queue by registering an OnSubmittedWorkDone callback.
	 *
	 * This function:
	 * 1. Retrieves the native wgpu::Queue from the command queue.
	 * 2. Creates a std::promise<void> to be set when the callback fires.
	 * 3. Registers a callback that sets the promise (or an exception) on completion.
	 * 4. Obtains the std::future<void> from the promise.
	 * 5. Allocates a WebGPUFuture with that future and stores it atomically.
	 *
	 * @param queue Pointer to the WebGPUCommandQueue on which to signal.
	 */
	void WebGPUPromise::CommandQueueSignal(WebGPUCommandQueue* queue) noexcept {
		// Get the actual wgpu::Queue from the command queue object.
		wgpu::Queue wgpu_queue = queue->GetNative();

		auto promise = std::make_shared<std::promise<void>>();

		wgpu_queue.OnSubmittedWorkDone(
			wgpu::CallbackMode::WaitAnyOnly,
			[promise](wgpu::QueueWorkDoneStatus status, wgpu::StringView message) {
				if (status == wgpu::QueueWorkDoneStatus::Success) {
					promise->set_value();
				} 
				else {
					try {
						throw std::runtime_error(std::format("WebGPU work completed with error status: {}", static_cast<int>(status)));
					} catch (...) {
						promise->set_exception(std::current_exception());
					}
				}
			}
		);

		// Atomically store the newly created future in the promise.
		m_future.store(
			std::allocate_shared<WebGPUFuture>(
				s_future_alloc,
				this,
				promise->get_future()
			),
			std::memory_order::release
		);
	}

	/**
	 * @brief Returns the native Dawn device.
	 * @return wgpu::Device handle.
	 */
	wgpu::Device WebGPUPromise::GetDevice() const noexcept {
		return m_device;
	}

	/**
	 * @brief Constructs a WebGPUFuture.
	 *
	 * Transfers the promised value from the promise and stores the completion future.
	 *
	 * @param promise Pointer to the associated WebGPUPromise.
	 * @param completion_future Rvalue reference to the std::future<void> that signals completion.
	 */
	WebGPUFuture::WebGPUFuture(WebGPUPromise* promise, std::future<void>&& completion_future)
		: PolymorphicFutureBase(this),
		  WebGPUCommon(this),
		  m_device(promise->GetDevice()),
		  m_completion_future(std::move(completion_future)),
		  m_value(std::move(promise->m_value)) {
	}

	/**
	 * @brief Blocks until the GPU operation completes.
	 *
	 * Calls wait() on the underlying std::future<void>. If the future holds an exception,
	 * it will be rethrown.
	 */
	void WebGPUFuture::Wait() const {
		m_completion_future.wait();
	}

} // namespace fyuu_rhi::webgpu