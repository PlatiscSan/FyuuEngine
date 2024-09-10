#include "pch.h"
#include "D3D12Core.h"

using namespace Fyuu::graphics::d3d12;
using namespace Microsoft::WRL;

static ComPtr<ID3D12Device10> s_device;

void Fyuu::graphics::d3d12::InitializeD3D12Device() {

	UINT creation_flags = 0;
	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	ComPtr<IDXGIFactory7> factory;

#if _DEBUG

	ComPtr<ID3D12Debug6> debug_interface;
	ComPtr<IDXGIDebug1> dxgi_debug;

	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
	debug_interface->EnableDebugLayer();

	ComPtr<IDXGIInfoQueue> dxgi_info_queue;
	ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)));

	dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
	dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

	dxgi_info_queue.Reset();

	creation_flags = DXGI_CREATE_FACTORY_DEBUG;

#endif // _DEBUG

	ThrowIfFailed(CreateDXGIFactory2(creation_flags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter4> adapter;
	DXGI_ADAPTER_DESC3 adapter_desc{};
	std::array<D3D_FEATURE_LEVEL, 5> feature_levels = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	for (UINT i = 0; SUCCEEDED(factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); i++) {

		adapter->GetDesc3(&adapter_desc);
		if (adapter_desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
			continue;
		}

		for (auto const& feature_level : feature_levels) {
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&s_device)))) {
				break;
			}
		}
		if (s_device) {
			break;
		}

	}
	if (!s_device) {
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&s_device)));
		for (auto const& feature_level : feature_levels) {
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&s_device)))) {
				break;
			}
		}
	}

	if (!s_device) {
		throw std::runtime_error("Failed to enumerate devices");
	}

#if _DEBUG

	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> info_queue;

	std::array<D3D12_MESSAGE_SEVERITY, 1> severities = { D3D12_MESSAGE_SEVERITY_INFO };
	D3D12_INFO_QUEUE_FILTER filter{};

	ThrowIfFailed(s_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

	filter.DenyList.NumSeverities = 1;
	filter.DenyList.pSeverityList = severities.data();
	info_queue->PushStorageFilter(&filter);

	info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

	info_queue.Reset();

#endif // _DEBUG

}

Microsoft::WRL::ComPtr<ID3D12Device10> const& Fyuu::graphics::d3d12::D3D12Device() noexcept {
	return s_device;
}
