module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <DescriptorHeap.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

export module fyuu_rhi:d3d12_swap_chain;
import std;
import plastic.any_pointer;
import :swap_chain;
import :d3d12_common;
import :d3d12_command_object;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)

	export class D3D12Surface 
		: public plastic::utility::EnableSharedFromThis<D3D12Surface>,
		public ISurface {
		friend class BaseSurface;

	private:
		HWND m_window_handle;

	public:
		D3D12Surface(
			plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device, 
			HWND window_handle
		) noexcept;

		D3D12Surface(
			D3D12PhysicalDevice const& physical_device,
			HWND window_handle
		) noexcept;

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeightImpl() const;

		HWND GetNative() const noexcept;

	};

	export class D3D12SwapChain 
		: public plastic::utility::EnableSharedFromThis<D3D12SwapChain>,
		public ISwapChain {
		friend class ISwapChain;
	private:

		struct SwapChain {
			Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgi_swap_chain;
			std::uint32_t buffer_size;
			bool is_tearing_supported;
			bool v_sync = true;
		};

		SwapChain m_impl;

		static SwapChain CreateSwapChainImpl(
			plastic::utility::AnyPointer<IDXGIFactory5> const& factory,
			plastic::utility::AnyPointer<ID3D12CommandQueue> const& command_queue,
			HWND window_handle,
			std::uint32_t buffer_size
		);

		void UpdateRenderTargetViews();

		void ResizeImpl(std::uint32_t width, std::uint32_t height);

		void PresentImpl();

		void EnableVSyncImpl(bool mode);

	public:
		D3D12SwapChain(
			plastic::utility::AnyPointer<IDXGIFactory5> const& factory,
			plastic::utility::AnyPointer<ID3D12CommandQueue> const& command_queue,
			HWND window_handle,
			std::uint32_t buffer_size = 3u
		);

		/// @brief 
		/// @param physical_device 
		/// @param logical_device not used in this API
		/// @param command_queue 
		/// @param surface 
		/// @param buffer_size 
		D3D12SwapChain(
			plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device,
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
			plastic::utility::AnyPointer<D3D12CommandQueue> const& command_queue,
			plastic::utility::AnyPointer<D3D12Surface> const& surface,
			std::uint32_t buffer_size = 3u
		);

		D3D12SwapChain(
			D3D12PhysicalDevice const& physical_device,
			D3D12LogicalDevice const& logical_device,
			D3D12CommandQueue const& command_queue,
			D3D12Surface const& surface,
			std::uint32_t buffer_size = 3u
		);


	};
#endif // defined(_WIN32)
}