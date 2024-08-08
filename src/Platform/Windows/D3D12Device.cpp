#include "pch.h"
#include "D3D12Device.h"

using namespace Fyuu::windows_util;

struct D3D12DeviceInfo {
	DXGI_ADAPTER_DESC3 adapter_desc;
	ID3D12Device10* device;
};

static D3D12DeviceInfo EnumerateDevices(IDXGIFactory7* factory) {

	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
	D3D12DeviceInfo device_info{};
	std::array<D3D_FEATURE_LEVEL, 5> feature_levels = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	bool is_hardware_found = false;
	for (UINT i = 0; SUCCEEDED(factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); i++) {

		adapter->GetDesc3(&device_info.adapter_desc);
		if (device_info.adapter_desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
			continue;
		}

		for (auto const& feature_level : feature_levels) {
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&device_info.device)))) {
				is_hardware_found = true;
				break;
			}
		}
		if (is_hardware_found) {
			break;
		}

	}
	if (!is_hardware_found) {
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
		for (auto const& feature_level : feature_levels) {
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&device_info.device)))) {
				is_hardware_found = true;
				break;
			}
		}
	}
 
	if (!is_hardware_found) {
		throw std::runtime_error("Failed to enumerate devices");
	}
	return device_info;

}

Fyuu::D3D12Device::D3D12Device() {

	UINT creation_flags = 0;
	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	Microsoft::WRL::ComPtr<IDXGIFactory7> factory;

#if _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug6> m_debug_interface;
	Microsoft::WRL::ComPtr<IDXGIDebug1> m_dxgi_debug;

	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_interface)));
	m_debug_interface->EnableDebugLayer();

	Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgi_info_queue;
	ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)));

	dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
	dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

	dxgi_info_queue.Reset();

	creation_flags = DXGI_CREATE_FACTORY_DEBUG;

#endif // _DEBUG

	ThrowIfFailed(CreateDXGIFactory2(creation_flags, IID_PPV_ARGS(&factory)));
	D3D12DeviceInfo info = EnumerateDevices(factory.Get());
	m_device.Attach(info.device);
	m_device_name = WStringToString(info.adapter_desc.Description);

#if _DEBUG

	ComPtr<ID3D12InfoQueue1> info_queue;

	D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
	D3D12_INFO_QUEUE_FILTER filter{};

	ThrowIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

	filter.DenyList.NumSeverities = 1;
	filter.DenyList.pSeverityList = severities;
	info_queue->PushStorageFilter(&filter);

	info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

	info_queue.Reset();

#endif // _DEBUG



}

