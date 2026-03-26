/* d3d12_command_queue.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <mutex>
#include <array>
#include <deque>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
module fyuu_rhi:d3d12_command_queue_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_command_queue;
import :types;
import :d3d12_future;
import :d3d12_logical_device;
import :d3d12_throw;
import :d3d12_common;
import :d3d12_types;
import :d3d12_command_buffer;

namespace fyuu_rhi::d3d12 {

	D3D12CommandQueue::D3D12CommandQueue(
		D3D12LogicalDevice const& logical_device,
		CommandObjectType queue_type,
		QueuePriority priority,
		std::wstring_view debug_name
	) : PolymorphicCommandQueueBase(this),
		D3D12Common(this),
		m_impl(
			[&logical_device, queue_type, priority, debug_name]() {
				
				D3D12_COMMAND_LIST_TYPE d3d12_cmd_list_type = ToD3D12CommandListType(queue_type);
				D3D12_COMMAND_QUEUE_PRIORITY d3d12_priority;

				switch (priority) {
				case fyuu_rhi::QueuePriority::High:
					d3d12_priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
					break;
				case fyuu_rhi::QueuePriority::Medium:
					d3d12_priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
					break;
				case fyuu_rhi::QueuePriority::Low:
					d3d12_priority = D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME;
					break;
				default:
					d3d12_priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
					break;
				}

				return logical_device.CreateCommandQueue(d3d12_cmd_list_type, d3d12_priority, debug_name);

			}()),
		m_internal_promise(logical_device) {
		
	}

	void fyuu_rhi::d3d12::D3D12CommandQueue::WaitUntilCompleted() {
		m_internal_promise.CommandQueueSignal(this);
		auto future = m_internal_promise.GetFuture();
		future->Wait();
	}

	void D3D12CommandQueue::Signal(ID3D12Fence* fence, std::uint64_t fence_value) {
		ThrowIfFailed(m_impl->Signal(fence, fence_value));
	}

	void D3D12CommandQueue::ExecuteCommand(D3D12CommandBuffer& command_buffer, D3D12Promise& promise, std::shared_ptr<D3D12Future> const& future) {
		
		if (future) {
			future->Wait();
		}

		ID3D12CommandList* command_lists[] = { command_buffer.GetRawNative() };
		m_impl->ExecuteCommandLists(static_cast<UINT>(std::size(command_lists)), command_lists);

		// promise signaling
		promise.CommandQueueSignal(this);

	}

	std::shared_ptr<D3D12Future> D3D12CommandQueue::ExecuteCommand(D3D12CommandBuffer& command_buffer, std::shared_ptr<D3D12Future> const& future) {
		ExecuteCommand(command_buffer, m_internal_promise, future);
		return m_internal_promise.GetFuture();
	}

	ID3D12CommandQueue* D3D12CommandQueue::GetRawNative() const noexcept {
		return m_impl.Get();
	}

}

#endif // defined(_WIN32)