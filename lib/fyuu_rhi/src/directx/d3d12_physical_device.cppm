module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#endif // WIN32

export module fyuu_rhi:d3d12_physical_device;
import std;
import plastic.any_pointer;
import :device;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)

	export class D3D12PhysicalDevice 
		: public plastic::utility::EnableSharedFromThis<D3D12PhysicalDevice>,
		public IPhysicalDevice {

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory5> m_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_impl;
		LogCallback m_log_callback;

	public:
		D3D12PhysicalDevice(RHIInitOptions const& init_options);

		Microsoft::WRL::ComPtr<IDXGIFactory5> GetFactory() const noexcept;

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetNative() const noexcept;

		LogCallback GetLogCallback() const noexcept;


	};
#endif // defined(_WIN32)

}