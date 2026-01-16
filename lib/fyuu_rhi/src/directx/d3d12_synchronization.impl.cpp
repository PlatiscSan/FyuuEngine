module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <DescriptorHeap.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module fyuu_rhi:d3d12_synchronization;
import :d3d12_logical_device;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)
	
	Microsoft::WRL::ComPtr<ID3D12Fence> D3D12Fence::CreateFenceImpl(plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device) {
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(logical_device->GetNative()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	void D3D12Fence::WaitImpl() {
		if (m_impl->GetCompletedValue() != 1) {
			m_impl->SetEventOnCompletion(1, m_fence_event);

			WaitForSingleObject(m_fence_event, (std::numeric_limits<DWORD>::max)());
		}
	}

	void D3D12Fence::ResetImpl() {
		//m_owner->m_internal_queue.m_impl->Signal(m_impl.Get(), 0ull);
	}

	void D3D12Fence::SignalImpl() {
		//m_owner->m_internal_queue.m_impl->Signal(m_impl.Get(), 1ull);
	}

	D3D12Fence::D3D12Fence(plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, bool pre_signaled)
		: m_owner(plastic::utility::MakeReferred(logical_device)), 
		m_impl(D3D12Fence::CreateFenceImpl(logical_device)),
		m_fence_event(CreateEvent(nullptr, false, false, nullptr)) {
	}
#endif // defined(_WIN32)
}
