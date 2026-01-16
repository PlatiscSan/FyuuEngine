module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <DescriptorHeap.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

export module fyuu_rhi:d3d12_synchronization;
import :synchronization;
import :d3d12_common;
import std;
import plastic.any_pointer;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)

	export class D3D12Fence
		: public IFence {
		friend class IFence;

	private:
		plastic::utility::AnyPointer<D3D12LogicalDevice> m_owner;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_impl;
		HANDLE m_fence_event;

		Microsoft::WRL::ComPtr<ID3D12Fence> CreateFenceImpl(plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device);

		void WaitImpl();

		void ResetImpl();

		void SignalImpl();

	public:
		D3D12Fence(plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, bool pre_signaled);

	};
#endif // defined(_WIN32)
}