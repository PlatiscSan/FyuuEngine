module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#endif // WIN32

module fyuu_rhi:d3d12_physical_device;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)
	static void EnableDebugLayer() {
#if !defined(_NDEBUG)
		// Always enable the debug layer before doing anything DX12 related
		// so all possible errors generated while creating DX12 objects
		// are caught by the debug layer.
		// Enabling the debug layer after creating the ID3D12Device will cause the runtime to remove the device. 
		Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
		debug_interface->EnableDebugLayer();
#if defined(GPU_BASED_VALIDATION)
		// GPU-based validation
		Microsoft::WRL::ComPtr<ID3D12Debug1> debug_controller1;
		ThrowIfFailed(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_controller1)));
		debug_controller1->SetEnableGPUBasedValidation(true);
#endif
		// enable DRED 
		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred_settings;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings)));

		// Turn on auto-breadcrumbs and page fault reporting.
		dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
	}

	static Microsoft::WRL::ComPtr<IDXGIFactory5> CreateFactory() {

		Microsoft::WRL::ComPtr<IDXGIFactory5> factory;

		UINT create_factory_flags = 0;
#if !defined(_NDEBUG)
		create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		// Rather than create the DXGI 1.5 factory interface directly, we create the
		// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
		// graphics debugging tools which will not support the 1.5 factory interface 
		// until a future update.
		{
			Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
			ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&factory4)));
			ThrowIfFailed(factory4.As(&factory));
		}

		return factory;

	}

	static Microsoft::WRL::ComPtr<IDXGIAdapter4> CreateAdapter(
		Microsoft::WRL::ComPtr<IDXGIFactory5> const& factory,
		RHIInitOptions const& init_options
	) {

		static std::once_flag global_init_flag;
		std::call_once(
			global_init_flag,
			[]() {
				EnableDebugLayer();
			}
		);

		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;

		SIZE_T max_dedicated_video_memory = 0;
		for (UINT i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 dxgi_adapter_desc1;
			adapter1->GetDesc1(&dxgi_adapter_desc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			// use DXGI_ADAPTER_FLAG_SOFTWARE to only select hardware devices
			if ((dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(adapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory)
			{
				max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
				if (SUCCEEDED(adapter1.As(&adapter))) {
					return adapter;
				}
			}
		}

		if (init_options.software_fallback) {
			if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1))) && SUCCEEDED(adapter1.As(&adapter))) {
				return adapter;
			}
		}

		throw std::runtime_error("No physical device available");

	}

	D3D12PhysicalDevice::D3D12PhysicalDevice(RHIInitOptions const& init_options)
		: m_factory(CreateFactory()),
		m_impl(CreateAdapter(m_factory, init_options)),
		m_log_callback(init_options.log_callback) {
	}


	Microsoft::WRL::ComPtr<IDXGIFactory5> D3D12PhysicalDevice::GetFactory() const noexcept {
		return m_factory;
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter4> D3D12PhysicalDevice::GetNative() const noexcept {
		return m_impl;
	}

	LogCallback fyuu_rhi::d3d12::D3D12PhysicalDevice::GetLogCallback() const noexcept {
		return m_log_callback;
	}

#endif // defined(_WIN32)

}