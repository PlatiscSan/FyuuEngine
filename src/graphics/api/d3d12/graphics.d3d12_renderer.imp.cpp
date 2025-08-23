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
#endif // WIN32

module graphics:d3d12_renderer;
import std;
import collective_resource;

static inline void Error(core::LoggerPtr const& logger, std::string_view error_message, std::source_location const& src = std::source_location::current()) {
	if (logger) {
		logger->Error(src, "DirectX12 device error, {}", error_message);
	}
}

namespace graphics::api::d3d12 {
#ifdef WIN32
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

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					// Don't select the Basic Render Driver adapter.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
					break;
				}
			}
		}

		if (!adapter) {
			for (UINT adapter_index = 0; SUCCEEDED(factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index) {
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					// Don't select the Basic Render Driver adapter.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
					break;
				}
			}
		}

		return adapter;

	}

	static std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetRenderTargetDescriptorHandles(
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

	static Microsoft::WRL::ComPtr<ID3D12Resource> GetRenderTarget(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle,
		std::uint32_t frame_index
	) {

		Microsoft::WRL::ComPtr<ID3D12Resource> render_target;

		ThrowIfFailed(swap_chain->GetBuffer(frame_index, IID_PPV_ARGS(&render_target)));
		device->CreateRenderTargetView(render_target.Get(), nullptr, rtv_handle);

		return render_target;

	}

	std::vector<FrameContext> FrameContext::CreateFrames(
		std::uint32_t frame_count,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		std::uint32_t resource_descriptor_count
	) {

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles = GetRenderTargetDescriptorHandles(device, rtv_heap);
		std::vector<FrameContext> frames;
		frames.reserve(frame_count);
		for (std::uint32_t i = 0; i < frame_count; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_handles[i];
			frames.emplace_back(rtv_handle, i, device, swap_chain, resource_descriptor_count);
		}

		return frames;
	}

	FrameContext::FrameContext(
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle,
		std::uint32_t frame_index,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		std::uint32_t resource_descriptor_count
	) : m_rtv_handle(rtv_handle),
		m_frame_index(frame_index),
		m_render_target(::graphics::api::d3d12::GetRenderTarget(device, swap_chain, m_rtv_handle, m_frame_index)),
		m_descriptor_pool(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024u),
		m_ready_command_lists(),
		m_ready_command_lists_mutex(),
		m_fence_value(0) {
	}

	FrameContext::FrameContext(FrameContext&& other) noexcept 
		: m_rtv_handle(other.m_rtv_handle),
		m_frame_index(other.m_frame_index),
		m_render_target(std::move(other.m_render_target)),
		m_descriptor_pool(std::move(other.m_descriptor_pool)),
		m_ready_command_lists(std::move(other.m_ready_command_lists)),
		m_ready_command_lists_mutex(),
		m_fence_value(other.m_fence_value) {
	}

	FrameContext& FrameContext::operator=(FrameContext&& other) noexcept {
		if (this != &other) {
			m_rtv_handle = other.m_rtv_handle;
			m_frame_index = other.m_frame_index;
			m_render_target = std::move(other.m_render_target);
			m_descriptor_pool = std::move(other.m_descriptor_pool);
			m_ready_command_lists = std::move(other.m_ready_command_lists);
			m_fence_value = other.m_fence_value;
		}
		return *this;
	}

	FrameContext::~FrameContext() noexcept {
		m_render_target.Reset();
		m_ready_command_lists.clear();
		m_fence_value = 0;
	}

	DescriptorResourcePool::UniqueDescriptorHandle 
		FrameContext::AcquireResourceDescriptorHandle() noexcept {
		return m_descriptor_pool.AcquireHandle();
	}

	std::uint64_t FrameContext::GetFenceValue() const noexcept {
		return m_fence_value;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE FrameContext::GetRTVHandle() const noexcept {
		return m_rtv_handle;
	}

	void FrameContext::CommitCommands(ID3D12CommandList* list) {
		std::lock_guard<std::mutex> lock(m_ready_command_lists_mutex);
		m_ready_command_lists.emplace_back(list);
	}

	std::vector<ID3D12CommandList*> FrameContext::FetchAllCommands() noexcept {
		std::lock_guard<std::mutex> lock(m_ready_command_lists_mutex);
		return std::move(m_ready_command_lists);
	}

	void FrameContext::ResetRenderTarget(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device, 
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain
	) {
		m_render_target = ::graphics::api::d3d12::GetRenderTarget(device, swap_chain, m_rtv_handle, m_frame_index);
		m_ready_command_lists.clear();
	}

	void FrameContext::SignalFence(std::uint64_t fence_value) noexcept {
		m_fence_value = fence_value;
	}

	void FrameContext::ReleaseRenderTarget() noexcept {
		m_render_target.Reset();
	}

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

	void D3D12RenderDevice::WaitForNextFrame() {

		/*
		*	after calling Present, GetCurrentBackBufferIndex() gets the next frame
		*/

		std::array waitable_objects = { HANDLE(m_swap_chain_waitable_object), HANDLE(nullptr) };
		std::uint32_t num_waitable_objects = 1u;

		FrameContext& next_frame = m_frames[m_next_frame_index];
		std::uint64_t fence_value = next_frame.GetFenceValue();
		if (fence_value != 0) {
			m_fence->SetEventOnCompletion(fence_value, m_fence_event);
			waitable_objects[1] = m_fence_event;
			++num_waitable_objects;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects.data(), true, INFINITE);

	}

	void D3D12RenderDevice::WaitForLastSubmittedFrame() {

		FrameContext& frame = m_frames[m_previous_frame_index];
		std::uint64_t fence_value = frame.GetFenceValue();
		if (fence_value == 0) {
			// no fence signaled.
			return;
		}

		if (m_fence->GetCompletedValue() >= fence_value) {
			return;
		}

		m_fence->SetEventOnCompletion(fence_value, m_fence_event);
		WaitForSingleObject(m_fence_event, INFINITE);

	}

	LRESULT D3D12RenderDevice::HandleResize(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		try {
			switch (msg) {
			case WM_SIZE:
				if (m_device && wparam != SIZE_MINIMIZED) {
					WaitForLastSubmittedFrame();
					for (auto& frame : m_frames) {
						frame.ReleaseRenderTarget();
					}
					ThrowIfFailed(
						m_swap_chain->ResizeBuffers(
							0,
							(UINT)LOWORD(lparam),
							(UINT)HIWORD(lparam),
							DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
						)
					);
					for (auto& frame : m_frames) {
						frame.ResetRenderTarget(m_device, m_swap_chain);
					}
					m_next_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
					m_previous_frame_index = 0;
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

	void D3D12RenderDevice::OnCommandReady(CommandReadyEvent const& e) {

		while (!m_allow_submission.test(std::memory_order::acquire)) {
			m_allow_submission.wait(false, std::memory_order::relaxed);
		}

		if (e.command_list) {
			m_frames[m_next_frame_index].CommitCommands(e.command_list.Get());
		}

	}

	D3D12RenderDevice::~D3D12RenderDevice() noexcept {
		WaitForLastSubmittedFrame();
		m_frames.clear();
		static_cast<platform::Win32Window*>(m_window)->DetachMsgProcessor(m_msg_processor_uuid);
		CloseHandle(m_fence_event);
	}

	Microsoft::WRL::ComPtr<ID3D12Device> D3D12RenderDevice::GetD3D12Device() const noexcept {
		return m_device;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12RenderDevice::GetCommandQueue() const noexcept {
		return m_command_queue;
	}

	std::size_t D3D12RenderDevice::FrameCount() const noexcept {
		return m_frames.size();
	}

	DescriptorResourcePool::UniqueDescriptorHandle 
		D3D12RenderDevice::AcquireResourceDescriptorHandle() noexcept {
		return m_frames[m_swap_chain->GetCurrentBackBufferIndex()].AcquireResourceDescriptorHandle();
	}

	DescriptorResourcePool D3D12RenderDevice::CreateDescriptorResourcePool(
		D3D12_DESCRIPTOR_HEAP_TYPE type, 
		std::uint32_t count
	) const {
		return DescriptorResourcePool(m_device, type, count);
	}

	void D3D12RenderDevice::Clear(float r, float g, float b, float a) {
		std::array const clear_color = { r, g, b, a };
		FrameContext& frame = m_frames[m_swap_chain->GetCurrentBackBufferIndex()];
		auto& command_object = static_cast<D3D12CommandObject&>(AcquireCommandObject());
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list = command_object.GetCommandList();
		command_list->ClearRenderTargetView(frame.GetRTVHandle(), clear_color.data(), 0, nullptr);
	}

	void D3D12RenderDevice::SetViewport(int x, int y, uint32_t width, uint32_t height) {

		auto& command_object = static_cast<D3D12CommandObject&>(AcquireCommandObject());
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list
			= command_object.GetCommandList();

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

		command_list->RSSetViewports(1, &vp);
		command_list->RSSetScissorRects(1, &rect);

	}

	bool D3D12RenderDevice::BeginFrame() try {

		// Handle window screen locked
		if ((m_swap_chain_occluded && m_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || 
			::IsIconic(static_cast<platform::Win32Window*>(m_window)->GetHWND())) {
			m_swap_chain_occluded = true;
			return false; // notify we should skip this frame
		}
		m_swap_chain_occluded = false;

		// wait for next frame

		WaitForNextFrame();
		FrameContext& next_frame = m_frames[m_next_frame_index];

		auto& command_object = static_cast<D3D12CommandObject&>(AcquireCommandObject());
		auto command_list = command_object.GetCommandList();

		command_object.Reset();
		command_object.StartRecording();

		// barrier：PRESENT -> RENDER_TARGET
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = next_frame.GetRenderTarget().Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		command_list->ResourceBarrier(1, &barrier);

		// clear render target

		std::array clear_color = { 0.00f, 0.00f, 0.00f, 1.00f };
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = next_frame.GetRTVHandle();

		command_list->ClearRenderTargetView(rtv_handle, clear_color.data(), 0, nullptr);
		command_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

		/*
		*	Allow submission to the frame
		*/

		m_allow_submission.test_and_set(std::memory_order::release);
		m_allow_submission.notify_all();

		return true;

	}
	catch (_com_error const& ex) {

		if (m_logger) {

			auto error_message = platform::TStringToString(ex.ErrorMessage());

			m_logger->Error(
				std::source_location::current(),
				"{}",
				error_message
			);

		}

		return false;

	}

	void D3D12RenderDevice::EndFrame() try {

		FrameContext& next_frame = m_frames[m_next_frame_index];

		// command object for the present thread
		auto& command_object = static_cast<D3D12CommandObject&>(AcquireCommandObject());
		auto command_list = command_object.GetCommandList();

		// barrier: RENDER_TARGET -> PRESENT

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = next_frame.GetRenderTarget().Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		command_list->ResourceBarrier(1, &barrier);

		auto [width, height] = m_window->GetWidthAndHeight();
		SetViewport(0, 0, width, height);

		// end the recording in the present thread.

		command_object.EndRecording();

		/*
		*	No more submission to this frame
		*/

		m_allow_submission.clear(std::memory_order::release);

		// now we can submit the command list for execution

		auto ready_command_lists = next_frame.FetchAllCommands();
		m_command_queue->ExecuteCommandLists(static_cast<std::uint32_t>(ready_command_lists.size()), ready_command_lists.data());

		// present

		HRESULT hr = m_swap_chain->Present(1, 0); // v-async
		m_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);

		// GPU/CPU sync

		std::uint64_t fence_value = ++m_fence_value;
		ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), fence_value));
		next_frame.SignalFence(fence_value);

		m_previous_frame_index = m_next_frame_index;
		m_next_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

	}
	catch (_com_error const& ex) {

		if (m_logger) {

			auto error_message = platform::TStringToString(ex.ErrorMessage());

			m_logger->Error(
				std::source_location::current(),
				"{}",
				error_message
			);

		}

	}

	API D3D12RenderDevice::GetAPI() const noexcept {
		return API::DirectX12;
	}

	ICommandObject& D3D12RenderDevice::AcquireCommandObject() {
		static thread_local std::vector<D3D12CommandObject> command_objects;
		static thread_local std::once_flag command_objects_initialized;
		std::call_once(
			command_objects_initialized, 
			[this]() {
				std::size_t frame_count = m_frames.size();
				command_objects.reserve(frame_count);
				for (std::size_t i = 0; i < frame_count; ++i) {
					command_objects.emplace_back(m_device, &m_message_bus);
				}
			}
		);
		return command_objects[m_swap_chain->GetCurrentBackBufferIndex()];
	}
#endif // WIN32
}
