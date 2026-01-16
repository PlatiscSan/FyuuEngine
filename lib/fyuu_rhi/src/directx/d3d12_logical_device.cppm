module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <wrl.h>
#endif // WIN32

export module fyuu_rhi:d3d12_logical_device;
import std;
import plastic.any_pointer;
import plastic.registrable;
import :d3d12_common;
import :device;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)

	export class D3D12LogicalDevice :
		public plastic::utility::Registrable<D3D12LogicalDevice>,
		public plastic::utility::EnableSharedFromThis<D3D12LogicalDevice>,
		public ILogicalDevice {
	private:
		using DeviceRemovedHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&UnregisterWait)>;

		LogCallback m_log_callback;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_impl;
		EventHandle m_device_removed_event;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_device_removed_fence;
		DeviceRemovedHandle m_device_removed_handler;
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_video_memory_alloc;

		static DeviceRemovedHandle RegisterDeviceRemovedHandler(D3D12LogicalDevice* logical_device);

	public:
		D3D12LogicalDevice(plastic::utility::AnyPointer<IDXGIAdapter4> const& adapter, LogCallback callback = nullptr);
		D3D12LogicalDevice(plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device);
		D3D12LogicalDevice(D3D12PhysicalDevice const& physical_device);
		D3D12LogicalDevice(D3D12LogicalDevice&& other) noexcept;

		Microsoft::WRL::ComPtr<ID3D12Device2> GetNative() const noexcept;

		ID3D12Device2* GetRawNative() const noexcept;

		Microsoft::WRL::ComPtr<D3D12MA::Allocator> GetVideoMemoryAllocator() const noexcept;

		D3D12MA::Allocator* GetRawVideoMemoryAllocator() const noexcept;

		LogCallback GetLogCallback() const noexcept;

	};
#endif // defined(_WIN32)
}