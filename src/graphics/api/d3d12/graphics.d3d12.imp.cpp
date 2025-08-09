module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

module graphics:d3d12;
import std;

#ifdef WIN32
static inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
#ifdef _DEBUG
#ifdef UNICODE
		auto str = std::wformat(L"**ERROR** Fatal Error with HRESULT of {}", hr);
#else
		auto str = std::format("**ERROR** Fatal Error with HRESULT of {}", hr);
#endif
		OutputDebugString(str.data());
		__debugbreak();
#endif
		throw _com_error(hr);
	}
}

static Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHardwareAdapter(
	Microsoft::WRL::ComPtr<IDXGIFactory1> const& factory,
	bool request_high_performance_adapter = true
) {

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapter_index = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapter_index,
				request_high_performance_adapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
				++adapter_index) {

			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (!adapter) {
		for (UINT adapter_index = 0; SUCCEEDED(factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index) {
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	return adapter;

}


static inline void Error(core::LoggerPtr const& logger, std::string_view error_message, std::source_location const& src = std::source_location::current()) {
	if (logger) {
		logger->Error(src, "DirectX12 device error, {}", error_message);
	}
}

namespace graphics::api::d3d12 {

	Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory() {
#ifndef NDEBUG

		// Enable the D3D12 debug layer.
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
				debug_controller->EnableDebugLayer();
			}
		}
#endif
		Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
		return factory;
	}

	Microsoft::WRL::ComPtr<ID3D12Device> CreateDevice(
		Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
		bool use_warp
	) {

		Microsoft::WRL::ComPtr<ID3D12Device> device;

		if (use_warp) {
			Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)));

			ThrowIfFailed(D3D12CreateDevice(
				warp_adapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&device)
			));
		}
		else {

			Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter = GetHardwareAdapter(factory);

			ThrowIfFailed(D3D12CreateDevice(
				hardware_adapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&device)
			));
		}

#ifndef NDEBUG

		// Enable the D3D12 debug layer.
		{
			Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue;
			device->QueryInterface(IID_PPV_ARGS(&info_queue));
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		}
#endif

		return device;

	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;

		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

		return command_queue;

	}

	Microsoft::WRL::ComPtr<IDXGISwapChain3> CreateSwapChain(
		platform::Win32Window* window,
		Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> const& command_queue,
		std::uint32_t frame_count
	) {

		if (!window) {
			throw std::invalid_argument("null window pointer");
		}

		auto [width, height] = window->GetWidthAndHeight();

		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{};
		swap_chain_desc.BufferCount = frame_count;
		swap_chain_desc.Width = width;
		swap_chain_desc.Height = height;
		swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain3;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			command_queue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
			window->GetHWND(),
			&swap_chain_desc,
			nullptr,
			nullptr,
			&swap_chain
		));


		ThrowIfFailed(swap_chain.As(&swap_chain3));
		swap_chain3->SetMaximumFrameLatency(frame_count);

		return swap_chain3;

	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateRTVHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::uint32_t frame_count
	) {
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{};
		rtv_heap_desc.NumDescriptors = frame_count;
		rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)));

		return rtv_heap;
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetRenderTargetDescriptors(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap
	) {

		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = rtv_heap->GetDesc();
		if (rtv_heap_desc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
			throw std::invalid_argument("Not RTV heap");
		}

		std::uint32_t increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(rtv_heap_desc.NumDescriptors);
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
		for (std::uint32_t i = 0; i < rtv_heap_desc.NumDescriptors; ++i, handle.ptr += increment) {
			handles[i] = handle;
		}

		return handles;

	}

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> CreateFrameResource(
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		UINT m_rtv_descriptor_size,
		std::uint32_t frame_count
	) {

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> render_targets(frame_count, nullptr);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtv_heap->GetCPUDescriptorHandleForHeapStart());

		for (std::uint32_t i = 0; i < frame_count; ++i) {
			auto& render_target = render_targets[i];
			ThrowIfFailed(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_target)));
			device->CreateRenderTargetView(render_target.Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtv_descriptor_size);
		}

		return render_targets;

	}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
		return command_allocator;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& command_allocator
	) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			command_allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&command_list)));
		ThrowIfFailed(command_list->Close());
		return command_list;
	}

	Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		return fence;
	}

	HANDLE CreateFenceEvent() {
		HANDLE fence_event = CreateEvent(nullptr, false, false, nullptr);
		if (!fence_event) {
			throw std::runtime_error("Fence event creation failed");
		}
		return fence_event;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateSRVHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::uint32_t count
	) {

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap;
		D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
		srv_heap_desc.NumDescriptors = count;
		srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_heap)));

		return srv_heap;

	}

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> CreateRenderTargets(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> const& render_target_descriptors,
		std::size_t frame_count
	) {
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> render_targets;
		for (std::uint32_t i = 0; i < frame_count; ++i) {
			Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer;
			swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
			device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_descriptors[i]);
			render_targets.emplace_back(std::move(back_buffer));
		}
		return render_targets;
	}

	std::deque<graphics::api::d3d12::DescriptorHandle> GetDescriptorHandles(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& heap
	) {

		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		std::deque<graphics::api::d3d12::DescriptorHandle> handles;
		std::uint32_t increment = device->GetDescriptorHandleIncrementSize(desc.Type);
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = heap->GetCPUDescriptorHandleForHeapStart();

		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = { 0 };
		bool is_shader_visible = (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0;

		if (is_shader_visible) {
			gpu_handle = heap->GetGPUDescriptorHandleForHeapStart();
		}

		for (std::uint32_t i = 0; i < desc.NumDescriptors; ++i) {
			handles.emplace_back(graphics::api::d3d12::DescriptorHandle{ cpu_handle, gpu_handle });
			cpu_handle.ptr += increment;
			if (is_shader_visible) {
				gpu_handle.ptr += increment;
			}
		}
		return handles;

	}

	std::vector<graphics::api::d3d12::FrameContext> CreateFrameContexts(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::size_t frame_count
	) {
		std::vector<graphics::api::d3d12::FrameContext> contexts;
		for (std::size_t i = 0; i < frame_count; ++i) {
			contexts.emplace_back(device);
		}
		return contexts;
	}


	FrameContext::FrameContext(Microsoft::WRL::ComPtr<ID3D12Device> const& device)
		: command_allocator(CreateCommandAllocator(device)),
		fence_value(0) {
	}

	FrameContext& D3D12RenderDevice::WaitForNextFrameResource() {

		std::size_t next_frame_index = (m_frame_index + 1) % m_frame_contexts.size();

		HANDLE waitable_objects[] = { m_swap_chain_waitable_object, nullptr };
		std::uint32_t num_waitable_objects = 1u;

		FrameContext& frame_context = m_frame_contexts[next_frame_index];
		std::uint64_t fence_value = frame_context.fence_value;
		if (fence_value != 0) {
			// no fence was signaled
			frame_context.fence_value = 0;
			m_fence->SetEventOnCompletion(fence_value, m_fence_event);
			waitable_objects[1] = m_fence_event;
			++num_waitable_objects;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects, true, INFINITE);
		return frame_context;

	}

	void D3D12RenderDevice::WaitForLastSubmittedFrame() {
		FrameContext& frame_context = m_frame_contexts[m_frame_index];
		std::uint64_t fence_value = frame_context.fence_value;
		if (fence_value == 0) {
			// no fence signaled.
			return;
		}
		frame_context.fence_value = 0;
		if (m_fence->GetCompletedValue() >= fence_value) {
			return;
		}
		m_fence->SetEventOnCompletion(fence_value, m_fence_event);
		WaitForSingleObject(m_fence_event, INFINITE);
	}

	void D3D12RenderDevice::CleanUpRenderTargets() {
		WaitForLastSubmittedFrame();
		m_render_targets.clear();
	}

	LRESULT D3D12RenderDevice::HandleResize(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		try {
			switch (msg) {
			case WM_SIZE:
				if (m_device && wparam != SIZE_MINIMIZED) {
					WaitForLastSubmittedFrame();
					CleanUpRenderTargets();
					ThrowIfFailed(
						m_swap_chain->ResizeBuffers(
							0,
							(UINT)LOWORD(lparam),
							(UINT)HIWORD(lparam),
							DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
						)
					);
					m_render_targets = CreateRenderTargets(m_device, m_swap_chain, m_rtv_descriptors, m_frame_contexts.size());
				}
				break;
			default:
				break;
			}
		}
		catch (_com_error const& ex) {
			auto error_message = platform::TStringToString(ex.ErrorMessage());
			Error(m_logger, error_message);
		}

		return 0;

	}


	D3D12RenderDevice::~D3D12RenderDevice() noexcept {
		WaitForLastSubmittedFrame();
		m_render_targets.clear();
		static_cast<platform::Win32Window*>(m_window)->DetachMsgProcessor(m_msg_processor_uuid);
		CloseHandle(m_fence_event);
	}


	Microsoft::WRL::ComPtr<ID3D12Device> D3D12RenderDevice::GetD3D12Device() const noexcept {
		return m_device;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12RenderDevice::GetCommandQueue() const noexcept {
		return m_command_queue;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12RenderDevice::GetSRVHeap() const noexcept {
		return m_srv_heap;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> D3D12RenderDevice::GetCommandList() const noexcept {
		return m_command_list;
	}

	std::size_t D3D12RenderDevice::FrameCount() const noexcept {
		return m_render_targets.size();
	}

	DescriptorHandle D3D12RenderDevice::AcquireDescriptorHandle() noexcept {
		auto front = std::move(m_descriptor_handles.front());
		m_descriptor_handles.pop_front();
		return front;
	}

	void D3D12RenderDevice::ReturnDescriptorHandle(DescriptorHandle& handle) {
		(void)m_descriptor_handles.emplace_back(std::move(handle));
	}

	void D3D12RenderDevice::Clear(float r, float g, float b, float a) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
			m_rtv_heap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<std::uint32_t>(m_frame_index),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		);

		float const clear_color[] = { r, g, b, a };
		m_command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
	}

	void D3D12RenderDevice::SetViewport(int x, int y, uint32_t width, uint32_t height) {

		D3D12_VIEWPORT vp = {
			static_cast<float>(x),
			static_cast<float>(y),
			static_cast<float>(width),
			static_cast<float>(height),
			0.0f, 1.0f
		};

		D3D12_RECT rect = {
			static_cast<LONG>(x),
			static_cast<LONG>(y),
			static_cast<LONG>(x + width),
			static_cast<LONG>(y + height)
		};

		m_command_list->RSSetViewports(1, &vp);
		m_command_list->RSSetScissorRects(1, &rect);

	}

	void D3D12RenderDevice::SetClearColor(float r, float g, float b, float a) {
		m_clear_color = { r,g,b,a };
	}

	bool D3D12RenderDevice::BeginFrame() {

		// Handle window screen locked
		if ((m_swap_chain_occluded && m_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(static_cast<platform::Win32Window*>(m_window)->GetHWND())) {
			m_swap_chain_occluded = true;
			return false; // notify we should skip this frame
		}
		m_swap_chain_occluded = false;

		// wait for next frame

		FrameContext& frame_context = WaitForNextFrameResource();
		m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
		frame_context.command_allocator->Reset();

		// barrier：PRESENT -> RENDER_TARGET
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_render_targets[m_frame_index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		m_command_list->Reset(frame_context.command_allocator.Get(), nullptr);
		m_command_list->ResourceBarrier(1, &barrier);

		// clear render target
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
			m_rtv_heap->GetCPUDescriptorHandleForHeapStart(),
			static_cast<std::uint32_t>(m_frame_index),
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		);

		m_command_list->ClearRenderTargetView(rtv_handle, m_clear_color.data(), 0, nullptr);
		m_command_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

		return true;

	}

	void D3D12RenderDevice::EndFrame() {

		// barrier: RENDER_TARGET -> PRESENT

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_render_targets[m_frame_index].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_command_list->ResourceBarrier(1, &barrier);

		auto [width, height] = m_window->GetWidthAndHeight();
		SetViewport(0, 0, width, height);

		m_command_list->Close();

		ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
		m_command_queue->ExecuteCommandLists(1, cmd_lists);

		// present

		HRESULT hr = m_swap_chain->Present(1, 0); // v-async
		m_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);

		// GPU/CPU sync

		UINT64 const fence_value = m_fence_last_signaled_value + 1;
		m_command_queue->Signal(m_fence.Get(), fence_value);
		m_fence_last_signaled_value = fence_value;
		m_frame_contexts[m_frame_index].fence_value = fence_value;

	}

	API D3D12RenderDevice::GetAPI() const noexcept {
		return API::DirectX12;
	}

}

#endif // WIN32