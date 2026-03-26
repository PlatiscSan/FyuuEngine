/* d3d12_command_queue.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <mutex>
#include <deque>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_command_queue;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :types;
import :d3d12_future;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
	export class D3D12CommandQueue
		: public PolymorphicCommandQueueBase,
		public D3D12Common<D3D12CommandQueue> {
	private:
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_impl;
		D3D12Promise m_internal_promise;
		
	public:
		D3D12CommandQueue(
			D3D12LogicalDevice const& logical_device,
			CommandObjectType queue_type,
			QueuePriority priority = QueuePriority::High,
			std::wstring_view debug_name = {}
		);

		void WaitUntilCompleted();

		void Signal(ID3D12Fence* fence, std::uint64_t fence_value);

		void ExecuteCommand(D3D12CommandBuffer& command_buffer, D3D12Promise& promise, std::shared_ptr<D3D12Future> const& future = nullptr);

		std::shared_ptr<D3D12Future> ExecuteCommand(D3D12CommandBuffer& command_buffer, std::shared_ptr<D3D12Future> const& future = nullptr);

		ID3D12CommandQueue* GetRawNative() const noexcept;

	};
}

#endif // defined(_WIN32)