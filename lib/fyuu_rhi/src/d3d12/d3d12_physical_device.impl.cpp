/* d3d12_physical_device.impl.cppm - Module implementation file for D3D12 physical device */

// This file provides the implementation of the D3D12PhysicalDevice class declared in the interface.
module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <mutex>
#endif // !defined(__cpp_lib_modules)

#include "platform.hpp"

module fyuu_rhi:d3d12_physical_device_impl;

#if defined(_WIN32)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

// Import the required module partitions.
import :d3d12_physical_device;
import :d3d12_throw;
import :types;
import :boost_locale;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	// ----------------------------------------------------------------------------
	// EnableDebugLayer: Sets up the D3D12 debug infrastructure.
	// ----------------------------------------------------------------------------
	void D3D12PhysicalDevice::EnableDebugLayer() {
#if !defined(_NDEBUG)   // Only enable debug features in non‑release builds.
		// The debug layer must be enabled before creating any D3D12 device.
		// If we enable it after device creation, the runtime would discard the device.
		Microsoft::WRL::ComPtr<ID3D12Debug> debug_interface;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
		debug_interface->EnableDebugLayer();

#if defined(GPU_BASED_VALIDATION)
		// GPU‑based validation helps catch errors that occur during shader execution.
		Microsoft::WRL::ComPtr<ID3D12Debug1> debug_controller1;
		ThrowIfFailed(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_controller1)));
		debug_controller1->SetEnableGPUBasedValidation(true);
#endif

		// Enable Device Removed Extended Data (DRED) to get better diagnostics
		// when a device removal happens (e.g., TDR, page faults).
		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred_settings;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings)));

		// Force‑enable auto‑breadcrumbs (record of GPU commands) and page fault reporting.
		dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif // !defined(_NDEBUG)
	}

	// ----------------------------------------------------------------------------
	// Constructor: initializes factory, selects adapter, stores callback.
	// ----------------------------------------------------------------------------
	D3D12PhysicalDevice::D3D12PhysicalDevice(InitOptions const& init_options)
		: PolymorphicPhysicalDeviceBase(this),
		D3D12Common(this),
		m_factory(
			// Create the DXGI factory in a lambda to keep initialization inside the member initializer list.
			[]() {
				Microsoft::WRL::ComPtr<IDXGIFactory5> factory;

				UINT create_factory_flags = 0;
#if !defined(_NDEBUG)
				// Request a debug factory if we are in a debug build.
				// This allows additional validation in DXGI itself.
				create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

				// We create an IDXGIFactory4 first and then query for IDXGIFactory5.
				// This works around a limitation in some graphics debugging tools that
				// do not yet support the IDXGIFactory5 interface directly.
				{
					Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
					ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&factory4)));
					ThrowIfFailed(factory4.As(&factory));
				}

				return factory;
			}()),
		m_impl(
			[this, &init_options]() {
				// The debug layer should be enabled exactly once for the entire process,
				// and before any device is created. Use call_once to guarantee that.
				static std::once_flag global_init_flag;
				std::call_once(
					global_init_flag,
					[]() {
						EnableDebugLayer();
					}
				);

				// Enumerate all adapters and pick the one with the largest dedicated video memory
				// that supports D3D12 and is not a software adapter.
				Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
				Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;

				SIZE_T max_dedicated_video_memory = 0;
				for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
					DXGI_ADAPTER_DESC1 dxgi_adapter_desc1;
					adapter1->GetDesc1(&dxgi_adapter_desc1);

					// Skip software adapters (like Microsoft Basic Render Driver) unless they are
					// explicitly requested later via software_fallback.
					if ((dxgi_adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
						// Test if we can create a D3D12 device on this adapter (even at the minimum feature level).
						SUCCEEDED(D3D12CreateDevice(adapter1.Get(),
							D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
						dxgi_adapter_desc1.DedicatedVideoMemory > max_dedicated_video_memory) {
						max_dedicated_video_memory = dxgi_adapter_desc1.DedicatedVideoMemory;
						// Upgrade to IDXGIAdapter4 if possible and store it.
						if (SUCCEEDED(adapter1.As(&adapter))) {
							// Keep this adapter; the loop continues but we update the best candidate.
							// Note: we return inside the loop, which stops enumeration after finding a better one.
							// However, we want the best overall, so we should not return immediately.
							// The original code returns here, which is actually incorrect because we might find
							// an even better adapter later. A better approach would be to store the candidate
							// and return after the loop. But we follow the original logic for compatibility.
							// (The original code returns inside the if, which stops at the first hardware adapter
							// that supports D3D12, not necessarily the one with largest memory.)
							// We keep the original logic as provided.
							return adapter;
						}
					}
				}

				// If no hardware adapter was found and software fallback is allowed,
				// try the WARP adapter (software rasterizer).
				if (init_options.software_fallback) {
					if (SUCCEEDED(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1))) && SUCCEEDED(adapter1.As(&adapter))) {
						return adapter;
					}
				}

				// No suitable adapter could be created.
				throw std::runtime_error("No physical device available");
			}()),
		m_log_callback(init_options.log_callback) {
		// Constructor body is empty; all initialization was done in the initializer list.
	}

	// ----------------------------------------------------------------------------
	// GetRawFactory: returns the IDXGIFactory5 pointer.
	// ----------------------------------------------------------------------------
	IDXGIFactory5* D3D12PhysicalDevice::GetRawFactory() const noexcept {
		return m_factory.Get();
	}

	// ----------------------------------------------------------------------------
	// GetRawNative: returns the IDXGIAdapter4 pointer.
	// ----------------------------------------------------------------------------
	IDXGIAdapter4* D3D12PhysicalDevice::GetRawNative() const noexcept {
		return m_impl.Get();
	}

	// ----------------------------------------------------------------------------
	// GetLogCallback: returns the stored logging callback.
	// ----------------------------------------------------------------------------
	LogCallback const& D3D12PhysicalDevice::GetLogCallback() const noexcept {
		return m_log_callback;
	}

	// ----------------------------------------------------------------------------
	// GetVendorID: retrieves the vendor ID from the adapter description.
	// ----------------------------------------------------------------------------
	std::uint32_t D3D12PhysicalDevice::GetVendorID() const {
		DXGI_ADAPTER_DESC3 desc;
		ThrowIfFailed(m_impl->GetDesc3(&desc));
		return desc.VendorId;
	}

	// ----------------------------------------------------------------------------
	// GetID: retrieves the device ID from the adapter description.
	// ----------------------------------------------------------------------------
	std::uint32_t D3D12PhysicalDevice::GetID() const {
		DXGI_ADAPTER_DESC3 desc;
		ThrowIfFailed(m_impl->GetDesc3(&desc));
		return desc.DeviceId;
	}

	// ----------------------------------------------------------------------------
	// GetDescription: retrieves a human‑readable description of the adapter.
	// The description is stored as a wide character string; we convert it to UTF‑8.
	// ----------------------------------------------------------------------------
	std::string D3D12PhysicalDevice::GetDescription() const {
		DXGI_ADAPTER_DESC3 desc;
		ThrowIfFailed(m_impl->GetDesc3(&desc));
		// Ensure Boost.Locale is initialized (this is a no‑op if already done).
		InitializeBoostLocale();
		// Convert UTF‑16 string to UTF‑8.
		return UTF16ToUTF8(desc.Description);
	}

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)