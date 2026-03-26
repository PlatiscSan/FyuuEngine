/* d3d12_future.impl.cpp */
/**
 * @file d3d12_future.impl.cpp
 * @brief Implementation of D3D12Promise and D3D12Future.
 *
 * This file contains the definitions of the D3D12 promise/future classes,
 * including fence management, event handling, and allocation strategies.
 */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <memory_resource>
#endif
#include "platform.hpp"
module fyuu_rhi:d3d12_future_impl;

#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;			  // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :d3d12_future;
import :types;
import :d3d12_throw;
import :d3d12_types;
import :d3d12_logical_device;
import :d3d12_future;
import :d3d12_command_queue;
import :d3d12_common;

namespace {
	using namespace fyuu_rhi::d3d12;

	/**
	 * @brief Memory pool for D3D12Future allocations.
	 *
	 * A synchronized pool reduces allocation overhead and avoids contention.
	 */
	std::pmr::synchronized_pool_resource m_future_pool{};

	/**
	 * @brief Polymorphic allocator for D3D12Future, using the pool above.
	 */
	std::pmr::polymorphic_allocator<D3D12Future> s_future_alloc{ &m_future_pool };
}

namespace fyuu_rhi::d3d12 {

	/**
	 * @brief Constructs a D3D12Promise and creates its fence.
	 * @param logical_device Logical device used to create the fence.
	 */
	D3D12Promise::D3D12Promise(D3D12LogicalDevice const& logical_device)
		: PolymorphicPromiseBase(this),
		D3D12Common(this),
		m_impl(logical_device.CreateFence()),
		m_next_fence_value(1u),
		m_future(nullptr) {
	}

	/**
	 * @brief Atomically exchanges the stored future with nullptr and returns it.
	 * @return The most recent future, or nullptr.
	 */
	std::shared_ptr<D3D12Future> D3D12Promise::GetFuture() noexcept {
		return m_future.exchange(nullptr, std::memory_order::acq_rel);
	}

	/**
	 * @brief Signals the fence on the command queue and creates a new future.
	 *
	 * Increments the fence value, signals the queue, and stores a newly allocated
	 * D3D12Future that will wait for that fence value.
	 *
	 * @param queue Command queue on which to signal.
	 */
	std::uint64_t D3D12Promise::CommandQueueSignal(D3D12CommandQueue* queue) noexcept {
		std::uint64_t fence_value = m_next_fence_value.fetch_add(1u, std::memory_order::acquire);
		queue->Signal(m_impl.Get(), fence_value);
		m_future.store(
			std::allocate_shared<D3D12Future>(s_future_alloc, this, fence_value),
			std::memory_order::release
		);
		return fence_value;
	}

	bool D3D12Promise::IsSignaled(std::uint64_t value) const noexcept {
		return m_impl->GetCompletedValue() >= value;
	}

	/**
	 * @brief Returns the ComPtr to the underlying fence.
	 * @return ComPtr<ID3D12Fence1>.
	 */
	Microsoft::WRL::ComPtr<ID3D12Fence1> D3D12Promise::GetNative() const noexcept {
		return m_impl;
	}

	/**
	 * @brief Returns the raw pointer to the underlying fence.
	 * @return ID3D12Fence1*.
	 */
	ID3D12Fence1* D3D12Promise::GetRawNative() const noexcept {
		return m_impl.Get();
	}

	/**
	 * @brief Constructs a D3D12Future from a promise and a fence value.
	 *
	 * Sets up an event that will be signaled when the fence reaches the given value.
	 *
	 * @param promise Pointer to the associated promise.
	 * @param value   Fence value to wait for.
	 */
	D3D12Future::D3D12Future(D3D12Promise* promise, std::uint64_t value)
		: PolymorphicFutureBase(this),
		D3D12Common(this),
		m_impl(promise->GetNative()),
		m_event_handle(CreateEventHandle()),
		m_fence_value(value),
		m_value(std::move(promise->m_value)) {
		ThrowIfFailed(m_impl->SetEventOnCompletion(m_fence_value, m_event_handle.get()));
	}

	/**
	 * @brief Blocks until the fence has reached the expected value.
	 *
	 * If the fence's completed value is already >= the target, returns immediately.
	 * Otherwise waits on the event handle.
	 */
	void D3D12Future::Wait() const {
		if (m_impl->GetCompletedValue() < m_fence_value) {
			WaitForSingleObjectEx(m_event_handle.get(), (std::numeric_limits<DWORD>::max)(), false);
		}
	}

}

#endif // defined(_WIN32)