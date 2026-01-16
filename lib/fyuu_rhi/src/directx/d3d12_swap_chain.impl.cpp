module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <DescriptorHeap.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module fyuu_rhi:d3d12_swap_chain;
import :d3d12_physical_device;
import :d3d12_logical_device;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)

	D3D12Surface::D3D12Surface(
		plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device, 
		HWND window_handle
	) noexcept : m_window_handle(window_handle) {
	}

	D3D12Surface::D3D12Surface(D3D12PhysicalDevice const& physical_device, HWND window_handle) noexcept
		: m_window_handle(window_handle) {
	}

	std::pair<std::uint32_t, std::uint32_t> D3D12Surface::GetWidthAndHeightImpl() const {

		RECT client_rect{};
		GetClientRect(m_window_handle, &client_rect);

		return { client_rect.right - client_rect.left, client_rect.bottom - client_rect.top };

	}

	HWND D3D12Surface::GetNative() const noexcept {
		return m_window_handle;
	}

	D3D12SwapChain::SwapChain D3D12SwapChain::CreateSwapChainImpl(
		plastic::utility::AnyPointer<IDXGIFactory5> const& factory,
		plastic::utility::AnyPointer<ID3D12CommandQueue> const& command_queue, 
		HWND window_handle,
		std::uint32_t buffer_size
	) {

		Microsoft::WRL::ComPtr<IDXGISwapChain4> impl;
		BOOL allow_tearing;

		HRESULT result = factory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allow_tearing,
			sizeof(allow_tearing)
		);

		if (FAILED(result)) {
			allow_tearing = FALSE;
		}

		RECT client_rect{};
		GetClientRect(window_handle, &client_rect);

		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
		swap_chain_desc.Width = client_rect.right - client_rect.left;
		swap_chain_desc.Height = client_rect.bottom - client_rect.top;
		swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.Stereo = false;
		swap_chain_desc.SampleDesc = { 1, 0 };
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.BufferCount = buffer_size;
		swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;               // what hapens when the back buffer is not the size of the target
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // for going as fast as possible (no vsync), discard frames in-flight to reduce latency
		swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		// It is recommended to always allow tearing if tearing support is available.
		swap_chain_desc.Flags = allow_tearing == TRUE ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1;

		ThrowIfFailed(
			factory->CreateSwapChainForHwnd(
				command_queue.Get(),
				window_handle,
				&swap_chain_desc,
				nullptr,
				nullptr,
				&swap_chain1
			)
		);

		//	Disable the Alt+Enter full screen toggle feature. Switching to full screen will be handled manually.
		ThrowIfFailed(factory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swap_chain1.As(&impl));

		return { std::move(impl), buffer_size, allow_tearing == TRUE };

	}

	void D3D12SwapChain::UpdateRenderTargetViews() {

		//// deallocate resources
		//DXGI_SWAP_CHAIN_DESC1 desc;
		//m_impl->GetDesc1(&desc);
		//m_back_buffer_textures.clear();
		//// note: we do not need to manually release the descriptor indices, because 
		//// the framebuffers are wrapped in Texture handles and 
		//// automatically release their own IDs when they are destroyed

		//for (std::uint32_t i = 0; i < m_buffer_size; ++i) {

		//	auto rtv_handle = m_owner->m_rtv_heap->Allocate();

		//	Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer;
		//	ThrowIfFailed(m_impl->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));
		//	back_buffer->SetName(L"SwapChain Buffer");

		//	m_owner->m_impl->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle->cpu_handle);

		//	m_back_buffer_textures.emplace_back(m_owner, back_buffer, std::move(rtv_handle));
		//	m_back_buffers.emplace_back(std::move(back_buffer));

		//}

	}

	void D3D12SwapChain::ResizeImpl(std::uint32_t width, std::uint32_t height) {

		//// Don't allow 0 size swap chain back buffers.
		//width = (std::max)(1u, width);
		//height = (std::max)(1u, height);

		//// Flush the GPU queue to make sure the swap chain's back buffers
		//// are not being referenced by an in-flight command list.

		//m_owner->m_internal_queue.Flush();
		//m_back_buffer_textures.clear(); // need to release all existing references to the textures before can resize

		//UINT current_idx = m_impl->GetCurrentBackBufferIndex();

		//// Any references to the back buffers must be released
		//// before the swap chain can be resized.
		//m_back_buffers.clear();

		//DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		//ThrowIfFailed(m_impl->GetDesc(&swap_chain_desc));
		//ThrowIfFailed(
		//	m_impl->ResizeBuffers(
		//		m_buffer_size, 
		//		width, height, 
		//		swap_chain_desc.BufferDesc.Format, 
		//		swap_chain_desc.Flags
		//	)
		//);

		//UpdateRenderTargetViews();

	}

	void D3D12SwapChain::PresentImpl() {
		UINT sync_interval = m_impl.v_sync ? 1 : 0;
		UINT present_flags = (m_impl.is_tearing_supported && !m_impl.v_sync) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		m_impl.dxgi_swap_chain->Present(sync_interval, present_flags);
	}

	void D3D12SwapChain::EnableVSyncImpl(bool mode) {
		m_impl.v_sync = mode;
	}

	D3D12SwapChain::D3D12SwapChain(
		plastic::utility::AnyPointer<IDXGIFactory5> const& factory,
		plastic::utility::AnyPointer<ID3D12CommandQueue> const& command_queue, 
		HWND window_handle, 
		std::uint32_t buffer_size
	) : m_impl(D3D12SwapChain::CreateSwapChainImpl(factory, command_queue, window_handle, buffer_size)) {
	}

	D3D12SwapChain::D3D12SwapChain(
		plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device,
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
		plastic::utility::AnyPointer<D3D12CommandQueue> const& command_queue,
		plastic::utility::AnyPointer<D3D12Surface> const& surface,
		std::uint32_t buffer_size
	) : D3D12SwapChain(physical_device->GetFactory(), command_queue->GetNative(), surface->GetNative(), buffer_size) {

	}

	D3D12SwapChain::D3D12SwapChain(
		D3D12PhysicalDevice const& physical_device, 
		D3D12LogicalDevice const& logical_device, 
		D3D12CommandQueue const& command_queue, 
		D3D12Surface const& surface, 
		std::uint32_t buffer_size
	) : D3D12SwapChain(physical_device.GetFactory(), command_queue.GetNative(), surface.GetNative(), buffer_size) {
	}



#endif // defined(_WIN32)
}