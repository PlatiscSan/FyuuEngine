module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <stdexcept>
#include <utility>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <d3d12.h>
#include <wrl.h>
#endif // defined(_WIN32)

module fyuu_rhi:d3d12_scheduler;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_traits;
import :d3d12_utility;

namespace {

	D3D12_COMMAND_LIST_TYPE GetQueueType(fyuu_rhi::execution::SchedulerFlags const& flags) {
		using Flag = fyuu_rhi::execution::SchedulerFlagBits;
		if (flags.Test(Flag::Graphics)) {
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}
		if (flags.Test(Flag::Compute)) {
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		}
		if (flags.Test(Flag::Copy)) {
			return D3D12_COMMAND_LIST_TYPE_COPY;
		}
		throw std::invalid_argument("CreateScheduler(): no execution capability was requested");
	}

	void ValidateSchedulerFlags(fyuu_rhi::execution::SchedulerFlags const& flags) {
		using Flag = fyuu_rhi::execution::SchedulerFlagBits;
		if (flags.Test(Flag::Present) && !flags.Test(Flag::Graphics)) {
			throw std::invalid_argument("CreateScheduler(): presentation requires graphics capability");
		}
	}

}

namespace fyuu_rhi::d3d12 {

	Backend::Scheduler Backend::CreateScheduler(
		LogicalDevice const& ld,
		SchedulerDescriptor const& descriptor
	) {
		ValidateSchedulerFlags(descriptor.flags);

		D3D12_COMMAND_QUEUE_DESC queue_descriptor = {
			.Type = GetQueueType(descriptor.flags),
			.Priority = descriptor.priority >= 0.75f ?
				D3D12_COMMAND_QUEUE_PRIORITY_HIGH :
				D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0u
		};

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
		HRESULT result = ld.impl->CreateCommandQueue(
			&queue_descriptor,
			IID_PPV_ARGS(&queue)
		);
		ThrowIfFailed(result);

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		result = ld.impl->CreateFence(
			0u,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&fence)
		);
		ThrowIfFailed(result);

		return std::make_shared<D3D12Scheduler>(
			std::move(queue),
			std::move(fence),
			descriptor.flags
		);
	}

}
#endif // defined(_WIN32)
