module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include <directx/d3dx12.h>
#endif // WIN32

module d3d12_backend:renderer;
import rendering;
import :util;
import windows_utils;
import defer;
import concurrent_hash_map;
import static_hash_map;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	using ThreadAssociatedCommandObjects = concurrency::ConcurrentHashMap<
		std::thread::id,
		std::vector<D3D12CommandObject>
	>;

	using InstanceAssociatedCommandObjects = concurrency::ConcurrentHashMap<
		D3D12Renderer*,
		ThreadAssociatedCommandObjects
	>;

	static thread_local InstanceAssociatedCommandObjects s_command_objects;

	static D3D12_HEAP_TYPE HeapPoolTypeToD3D12HeapType(HeapPoolType type) noexcept {
		switch (type) {
		case fyuu_engine::windows::d3d12::HeapPoolType::Custom:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		case fyuu_engine::windows::d3d12::HeapPoolType::ReadBack:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
		case fyuu_engine::windows::d3d12::HeapPoolType::Upload:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;

		default:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		}
	}

	static D3D12_HEAP_FLAGS HeapPoolTypeToD3D12HeapFlags(HeapPoolType type) noexcept {
		switch (type) {
		case fyuu_engine::windows::d3d12::HeapPoolType::DepthStencil:
		case fyuu_engine::windows::d3d12::HeapPoolType::RenderTarget:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;

		case fyuu_engine::windows::d3d12::HeapPoolType::LargeTexture:
		case fyuu_engine::windows::d3d12::HeapPoolType::MediumTexture:
		case fyuu_engine::windows::d3d12::HeapPoolType::SmallTexture:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

		case fyuu_engine::windows::d3d12::HeapPoolType::LargeBuffer:
		case fyuu_engine::windows::d3d12::HeapPoolType::MediumBuffer:
		case fyuu_engine::windows::d3d12::HeapPoolType::SmallBuffer:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		default:
			return D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		}
	}

	static DXGI_FORMAT DataTypeToDXGIFormat(core::DataType type) {

		static concurrency::StaticHashMap<core::DataType, DXGI_FORMAT, 200> map = {

			{core::DataType::Unknown, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN},

			{core::DataType::Uint8, DXGI_FORMAT::DXGI_FORMAT_R8_UINT},
			{core::DataType::Uint8_2, DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT},
			{core::DataType::Uint8_3, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT},
			{core::DataType::Uint8_4, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT},

			{core::DataType::Uint16, DXGI_FORMAT::DXGI_FORMAT_R16_UINT},
			{core::DataType::Uint16_2, DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT},
			{core::DataType::Uint16_3, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT},
			{core::DataType::Uint16_4, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT},

			{core::DataType::Uint32, DXGI_FORMAT::DXGI_FORMAT_R32_UINT},
			{core::DataType::Uint32_2, DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT},
			{core::DataType::Uint32_3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT},
			{core::DataType::Uint32_4, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT},



			{core::DataType::Int8, DXGI_FORMAT::DXGI_FORMAT_R8_SINT},
			{core::DataType::Int8_2, DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT},
			{core::DataType::Int8_3, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT},
			{core::DataType::Int8_4, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT},

			{core::DataType::Int16, DXGI_FORMAT::DXGI_FORMAT_R16_SINT},
			{core::DataType::Int16_2, DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT},
			{core::DataType::Int16_3, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT},
			{core::DataType::Int16_4, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT},

			{core::DataType::Int32, DXGI_FORMAT::DXGI_FORMAT_R32_SINT},
			{core::DataType::Int32_2, DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT},
			{core::DataType::Int32_3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT},
			{core::DataType::Int32_4, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT},



			{core::DataType::Float16, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT},
			{core::DataType::Float16_2, DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT},
			{core::DataType::Float16_3, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT},
			{core::DataType::Float16_4, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT},

			{core::DataType::Float32, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT},
			{core::DataType::Float32_2, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT},
			{core::DataType::Float32_3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT},
			{core::DataType::Float32_4, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT},


		};

		return map.at(type);

	}

	static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;

		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

		return command_queue;

	}

	static Microsoft::WRL::ComPtr<IDXGISwapChain3> CreateSwapChain(
		WindowsWindow* window,
		Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> const& command_queue,
		std::uint32_t frame_count
	) {

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

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateRTVHeap(
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

	static std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetRenderTargetDescriptors(
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

	FrameContext::FrameContext(
		Microsoft::WRL::ComPtr<ID3D12Resource> const& buffer_, 
		D3D12_CPU_DESCRIPTOR_HANDLE render_target_descriptor_
	) : buffer(buffer_), 
		render_target_descriptor(render_target_descriptor_), 
		fence_value(0) {
	}

	bool D3D12Renderer::BeginFrameImpl() {
		// Handle window screen locked
		if ((m_swap_chain_occluded && m_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) ||
			::IsIconic(m_window->GetHWND())) {
			m_swap_chain_occluded = true;
			return false; // notify we should skip this frame
		}
		m_swap_chain_occluded = false;

		// wait for frame ready

		WaitForFrame();
		m_allow_submission.test_and_set(std::memory_order::release);
		m_allow_submission.notify_all();

		return true;

	}

	void D3D12Renderer::EndFrameImpl() {
		FrameContext& current_frame = m_frames[m_current_frame_index];

		/*
		*	No more submission to this frame
		*/

		m_allow_submission.clear(std::memory_order::release);

		
		// now we can submit the command list for execution

		std::vector<ID3D12CommandList*> ready_commands;
		ready_commands.reserve(current_frame.ready_commands.size());

		while (!current_frame.ready_commands.empty()) {
			ready_commands.emplace_back(current_frame.ready_commands.pop_front());
		}

		m_command_queue->ExecuteCommandLists(static_cast<std::uint32_t>(ready_commands.size()), ready_commands.data());


		// present

		HRESULT hr = m_swap_chain->Present(1, 0); // v-async
		m_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);

		// GPU/CPU sync

		std::uint64_t fence_value = m_fence_value++;
		ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), fence_value));
		current_frame.fence_value = fence_value;

		m_previous_frame_index = m_current_frame_index;
		m_current_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

	}

	D3D12CommandObject& D3D12Renderer::GetCommandObjectImpl() noexcept {

		ThreadAssociatedCommandObjects* thread_command_objects;

		{

			// the command objects associated to this render device instance
			auto render_device_command_objects = s_command_objects[this];

			thread_command_objects = &render_device_command_objects.Get();

		}

		try {
			auto& command_objects = thread_command_objects->UnsafeAt(std::this_thread::get_id());
			return command_objects[m_current_frame_index];
		}
		catch (std::out_of_range const&) {

			std::vector<D3D12CommandObject> command_objects;
			std::size_t frame_count = m_frames.size();
			command_objects.reserve(frame_count);
			for (std::size_t i = 0; i < frame_count; ++i) {
				command_objects.emplace_back(m_device, &m_message_bus);
			}

			auto [value_ptr, success] =
				thread_command_objects->try_emplace(std::this_thread::get_id(), std::move(command_objects));

			static thread_local util::Defer gc(
				[]() {
					auto modifier = s_command_objects.LockedModifier();
					for (auto& [instance, thread_command_objects] : modifier) {
						thread_command_objects.erase(std::this_thread::get_id());
					}
				}
			);

			return value_ptr->at(m_current_frame_index);

		}

	}

	D3D12HeapPool* D3D12Renderer::PickHeapPool(core::BufferDescriptor const& desc) {
		
		D3D12HeapPool* buffer_pool;

		std::array buffer_pools = {
			&m_heap_pools[HeapPoolType::SmallBuffer],
			&m_heap_pools[HeapPoolType::MediumBuffer],
			&m_heap_pools[HeapPoolType::LargeBuffer],
			&m_heap_pools[HeapPoolType::Upload],
			&m_heap_pools[HeapPoolType::ReadBack],
		};

		switch (desc.buffer_type) {
		case core::BufferType::Upload:
			buffer_pool = buffer_pools[3];
			break;

		case core::BufferType::Staging:
			buffer_pool = buffer_pools[4];
			break;

		default:
			
			for (std::size_t i = 0; i < 3u; ++i) {
				auto buffer_pool_ = buffer_pools[i];
				buffer_pool = desc.size < buffer_pool_->GetHeapSize() ?
					buffer_pool_ :
					nullptr;
			}

		}

		return buffer_pool;

	}

	UniqueD3D12Buffer D3D12Renderer::CreateBufferImpl(core::BufferDescriptor const& desc) {

		D3D12HeapPool* buffer_pool = PickHeapPool(desc);

		if (!buffer_pool) {
			throw std::out_of_range("No suitable pool");
		}

		std::optional<D3D12HeapChunk> chunk = buffer_pool->Allocate(desc.size, desc.alignment);

		D3D12_RESOURCE_STATES initial_state;

		D3D12_RESOURCE_DESC resource_desc{};
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Alignment = 0;
		resource_desc.Height = 1;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.Format = DataTypeToDXGIFormat(desc.data_type);
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		switch (desc.buffer_type) {
		case core::BufferType::Vertex:
		case core::BufferType::Index:
			resource_desc.Width = desc.size;
			initial_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			break;

		case core::BufferType::Constant:
			resource_desc.Width = (desc.size + 255) & ~255; // must be 256-byte aligned.
			initial_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			break;

		case core::BufferType::Upload:
			resource_desc.Width = desc.size;
			initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;
			break;

		case core::BufferType::Staging:
			resource_desc.Width = desc.size;
			initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
			break;

		default:
			throw std::invalid_argument("Unknown buffer type");
		}
 
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;

		ThrowIfFailed(m_device->CreatePlacedResource(
			chunk->heap.Get(),
			chunk->offset,
			&resource_desc,
			initial_state,
			nullptr,
			IID_PPV_ARGS(&resource)
		));

		return UniqueD3D12Buffer(D3D12Buffer(resource, std::move(chunk)), D3D12BufferCollector(buffer_pool));
	}

	D3D12RenderTarget D3D12Renderer::OutputTargetOfCurrentFrameImpl() {
		FrameContext& frame = m_frames[m_current_frame_index];
		return D3D12RenderTarget{ frame.buffer,frame.render_target_descriptor };
	}

	void D3D12Renderer::WaitForLastSubmittedFrame() {

		FrameContext& frame = m_frames[m_previous_frame_index];
		std::uint64_t fence_value = frame.fence_value;
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

	void D3D12Renderer::WaitForFrame() {

		std::array waitable_objects = { HANDLE(m_swap_chain_waitable_object), HANDLE(nullptr) };
		std::uint32_t num_waitable_objects = 1u;

		FrameContext& current_frame = m_frames[m_current_frame_index];
		std::uint64_t fence_value = current_frame.fence_value;
		if (fence_value != 0 && m_fence->GetCompletedValue() < fence_value) {
			m_fence->SetEventOnCompletion(fence_value, m_fence_event);
			waitable_objects[1] = m_fence_event;
			++num_waitable_objects;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects.data(), true, INFINITE);

	}

	void D3D12Renderer::CreateFrames(std::uint32_t frame_count) {

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> render_target_descriptors
			= GetRenderTargetDescriptors(m_device, m_render_target_heap);

		m_frames.reserve(frame_count);
		for (std::uint32_t i = 0; i < frame_count; ++i) {

			/*
			*	create back buffer and render target view
			*/

			Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
			D3D12_CPU_DESCRIPTOR_HANDLE render_target_descriptor = render_target_descriptors[i];

			ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
			m_device->CreateRenderTargetView(buffer.Get(), nullptr, render_target_descriptor);

			m_frames.emplace_back(buffer, render_target_descriptor);

		}

		m_current_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
		m_previous_frame_index = 0;

	}

	LRESULT D3D12Renderer::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		try {
			switch (msg) {
			case WM_SIZE:
				if (m_device && wparam != SIZE_MINIMIZED) {
					WaitForLastSubmittedFrame();
					auto frame_count = static_cast<std::uint32_t>(m_frames.size());
					m_frames.clear();
					ThrowIfFailed(
						m_swap_chain->ResizeBuffers(
							0,
							(UINT)LOWORD(lparam),
							(UINT)HIWORD(lparam),
							DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
						)
					);
					CreateFrames(frame_count);
					m_current_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
				}
				break;
			default:
				break;
			}
		}
		catch (_com_error const& ex) {
			auto error_message = TStringToString(ex.ErrorMessage());
			auto formatted_message = std::format("DirectX12 device error, {}", error_message);
			if (m_logger) {
				m_logger->Log(core::LogLevel::Error, formatted_message);
			}
		}

		/// pass to next layer
		return 0;

	}

	void D3D12Renderer::OnCommandReady(D3D12CommandListReady e) {
		
		while (!m_allow_submission.test(std::memory_order::acquire)) {
			m_allow_submission.wait(false, std::memory_order::relaxed);
		}

		if (ID3D12CommandList* list = e.list) {
			m_frames[m_current_frame_index].ready_commands.emplace_back(list);
		}

	}

	D3D12Renderer::D3D12Renderer(
		core::LoggerPtr const& logger,
		Microsoft::WRL::ComPtr<IDXGIFactory6> const& factory,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		util::PointerWrapper<WindowsWindow> const& window,
		std::uint32_t frame_count,
		HeapPoolSizeMap& heap_size_map
	) : Base(logger),
		m_device(device),
		m_command_queue(CreateCommandQueue(m_device)),
		m_window(util::MakeReferred(window)),
		m_swap_chain(CreateSwapChain(m_window.Get(), factory, m_command_queue, frame_count)),
		m_render_target_heap(CreateRTVHeap(m_device, frame_count)),
		m_fence(CreateFence(m_device)),
		m_fence_event(CreateFenceEvent()),
		m_swap_chain_waitable_object(m_swap_chain->GetFrameLatencyWaitableObject()),
		m_fence_value(0),
		m_command_ready_subscription(
			m_message_bus.Subscribe<D3D12CommandListReady>(
				[this](D3D12CommandListReady e) {
					OnCommandReady(e);
				}
			)
		) {

		for (auto& [heap_pool_type, pair] : heap_size_map) {
			auto& [heap_size, min_allocation] = pair;
			m_heap_pools.try_emplace(
				heap_pool_type,
				m_device,
				HeapPoolTypeToD3D12HeapType(heap_pool_type),
				HeapPoolTypeToD3D12HeapFlags(heap_pool_type),
				heap_size,
				min_allocation
			);
		}

		CreateFrames(frame_count);
		m_current_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

		m_msg_processor_uuid = m_window->AttachMsgProcessor(
			[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
				return WndProc(hwnd, msg, wparam, lparam);
			}
		);

	}

	D3D12Renderer::D3D12Renderer(D3D12Renderer&& other) noexcept
		: Base(std::move(other)),
		m_device(std::move(other.m_device)),
		m_command_queue(std::move(other.m_command_queue)),
		m_window(std::move(other.m_window)),
		m_swap_chain(std::move(other.m_swap_chain)),
		m_render_target_heap(std::move(other.m_render_target_heap)),
		m_fence(std::move(other.m_fence)),
		m_fence_event(std::exchange(other.m_fence_event, nullptr)),
		m_swap_chain_waitable_object(std::exchange(other.m_swap_chain_waitable_object, nullptr)),
		m_fence_value(std::exchange(other.m_fence_value, 0u)),
		m_frames(std::move(other.m_frames)),
		m_command_ready_subscription(
			m_message_bus.Subscribe<D3D12CommandListReady>(
				[this](D3D12CommandListReady e) {
					OnCommandReady(e);
				}
			)
		),
		m_heap_pools(std::move(other.m_heap_pools)) {

		other.m_command_ready_subscription = nullptr;

		m_window->DetachMsgProcessor(other.m_msg_processor_uuid);
		m_msg_processor_uuid = m_window->AttachMsgProcessor(
			[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
				return WndProc(hwnd, msg, wparam, lparam);
			}
		);

		{
			auto locked_command_objects_ptr = s_command_objects.find(&other);
			/*
			*	locked_command_objects_ptr owns the bucket lock, so call Unsafe version
			*/
			s_command_objects.BucketUnsafeEmplace(this, std::move(*locked_command_objects_ptr));
			s_command_objects.BucketUnsafeErase(&other);
		}

	}

	D3D12Renderer::~D3D12Renderer() {

		WaitForLastSubmittedFrame();
		m_allow_submission.clear();
		m_command_ready_subscription = nullptr;
		m_window->DetachMsgProcessor(m_msg_processor_uuid);
		m_frames.clear();
		m_heap_pools.clear();
		if (m_fence_event) {
			CloseHandle(m_fence_event);
			m_fence_event = nullptr;
		}

	}

	D3D12Renderer& D3D12Renderer::operator=(D3D12Renderer&& other) noexcept {
		if (this != &other) {

			/*
			*	clean up
			*/

			WaitForLastSubmittedFrame();
			for (auto& frame : m_frames) {
				frame.buffer->Release();
			}

			m_window->DetachMsgProcessor(m_msg_processor_uuid);
			m_heap_pools.clear();

			other.WaitForLastSubmittedFrame();
			other.m_command_ready_subscription = nullptr;

			/*
			*	move
			*/

			m_message_bus = std::move(other.m_message_bus);
			m_logger = std::move(other.m_logger);
			m_command_queue = std::move(other.m_command_queue);
			m_window = std::move(other.m_window);
			m_swap_chain = std::move(other.m_swap_chain);
			m_render_target_heap = std::move(other.m_render_target_heap);
			m_fence = std::move(other.m_fence);
			m_fence_event = std::exchange(other.m_fence_event, nullptr);
			m_swap_chain_waitable_object = std::exchange(other.m_swap_chain_waitable_object, nullptr);
			m_fence_value = std::exchange(other.m_fence_value, 0u);
			m_command_ready_subscription = m_message_bus.Subscribe<D3D12CommandListReady>(
				[this](D3D12CommandListReady e) {
					OnCommandReady(e);
				}
			);
			m_heap_pools = std::move(other.m_heap_pools);
			m_frames = std::move(other.m_frames);

			m_window->DetachMsgProcessor(other.m_msg_processor_uuid);
			m_msg_processor_uuid = m_window->AttachMsgProcessor(
				[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
					return WndProc(hwnd, msg, wparam, lparam);
				}
			);

			{
				auto locked_command_objects_ptr = s_command_objects.find(&other);
				/*
				*	locked_command_objects_ptr owns the bucket lock, so call Unsafe version
				*/
				s_command_objects.BucketUnsafeEmplace(this, std::move(*locked_command_objects_ptr));
				s_command_objects.BucketUnsafeErase(&other);
			}

		}
		return *this;
	}

	Microsoft::WRL::ComPtr<ID3D12Device> D3D12Renderer::GetDevice() const noexcept {
		return m_device;
	}

	Microsoft::WRL::ComPtr<IDXGIFactory6> D3D12RendererBuilder::CreateFactory() {

		Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
		std::uint32_t flag = 0;
#ifndef NDEBUG

		// Enable the D3D12 debug layer.
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
				debug_controller->EnableDebugLayer();
			}
		}

		flag = DXGI_CREATE_FACTORY_DEBUG;

#endif
		ThrowIfFailed(CreateDXGIFactory2(flag, IID_PPV_ARGS(&factory)));

		return factory;

	}

	Microsoft::WRL::ComPtr<IDXGIAdapter1> D3D12RendererBuilder::QueryHardwareAdapter() {

		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter = nullptr;

		for (
			UINT adapter_index = 0;
			SUCCEEDED(m_factory->EnumAdapterByGpuPreference(
				adapter_index,
				m_use_high_performance_adapter ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
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

		if (!adapter) {
			for (UINT adapter_index = 0; SUCCEEDED(m_factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index) {
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

	Microsoft::WRL::ComPtr<ID3D12Device> D3D12RendererBuilder::CreateDevice() {
		
		Microsoft::WRL::ComPtr<ID3D12Device> device;

		if (m_use_software) {
			Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
			ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)));

			ThrowIfFailed(D3D12CreateDevice(
				warp_adapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&device)
			));
		}
		else {

			Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter = QueryHardwareAdapter();

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

	static HeapPoolSizeMap CreateDefaultMap() {
		HeapPoolSizeMap map;
		map.try_emplace(
			HeapPoolType::SmallBuffer,
			4 * 1024 * 1024,	// 4MB heap size
			64 * 1024			// 64KB minimum allocation
		);
		map.try_emplace(
			HeapPoolType::MediumBuffer,
			16 * 1024 * 1024,	// 16MB heap size
			256 * 1024			// 256KB minimum allocation
		);
		map.try_emplace(
			HeapPoolType::LargeBuffer,
			64 * 1024 * 1024,	// 64MB heap size
			1 * 1024 * 1024		// 1M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::SmallTexture,
			4 * 1024 * 1024,	// 16MB heap size
			64 * 1024			// 64KB minimum allocation
		);
		map.try_emplace(
			HeapPoolType::MediumTexture,
			64 * 1024 * 1024,	// 64MB heap size
			1 * 1024 * 1024		// 1M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::LargeTexture,
			256 * 1024 * 1024,	// 256MB heap size
			4 * 1024 * 1024		// 4M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::RenderTarget,
			64 * 1024 * 1024,	// 64MB heap size
			4 * 1024 * 1024		// 4M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::MediumTexture,
			64 * 1024 * 1024,	// 64MB heap size
			1 * 1024 * 1024		// 1M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::DepthStencil,
			64 * 1024 * 1024,	// 64MB heap size
			1 * 1024 * 1024		// 1M minimum allocation
		);
		map.try_emplace(
			HeapPoolType::Upload,
			16 * 1024 * 1024,	// 16MB heap size
			64 * 1024			// 64KB minimum allocation
		);
		map.try_emplace(
			HeapPoolType::ReadBack,
			16 * 1024 * 1024,	// 16MB heap size
			64 * 1024			// 64KB minimum allocation
		);
		map.try_emplace(
			HeapPoolType::Custom,
			64 * 1024 * 1024,	// 64MB heap size
			1 * 1024 * 1024		// 1MB minimum allocation
		);

		return map;

	}

	std::shared_ptr<D3D12Renderer> D3D12RendererBuilder::BuildImpl() {

		if (!m_window) {
			return nullptr;
		}

		Microsoft::WRL::ComPtr<ID3D12Device> device = CreateDevice();

		return std::make_shared<D3D12Renderer>(m_logger, m_factory, device, m_window, m_frame_count, m_heap_pool_size_map);

	}

	void D3D12RendererBuilder::BuildImpl(std::optional<D3D12Renderer>& buffer) {
		if (!m_window) {
			return;
		}

		Microsoft::WRL::ComPtr<ID3D12Device> device = CreateDevice();
		buffer.emplace(m_logger, m_factory, device, m_window, m_frame_count, m_heap_pool_size_map);
	}

	void D3D12RendererBuilder::BuildImpl(std::span<std::byte> buffer) {
		if (!m_window || buffer.size() < sizeof(D3D12Renderer)) {
			return;
		}

		Microsoft::WRL::ComPtr<ID3D12Device> device = CreateDevice();
		new (buffer.data()) D3D12Renderer(m_logger, m_factory, device, m_window, m_frame_count, m_heap_pool_size_map);
	}

	D3D12RendererBuilder::D3D12RendererBuilder() 
		: m_factory(CreateFactory()),
		m_heap_pool_size_map(CreateDefaultMap()) {
	}

	D3D12RendererBuilder& D3D12RendererBuilder::UseSoftware(bool use_software) noexcept {
		m_use_software = use_software;
		m_use_high_performance_adapter = false;
		return *this;
	}

	D3D12RendererBuilder& D3D12RendererBuilder::UseHighPerformanceAdapter(bool use_high_performance_adapter) noexcept {
		m_use_software = false;
		m_use_high_performance_adapter = use_high_performance_adapter;
		return *this;
	}

	D3D12RendererBuilder& D3D12RendererBuilder::SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window) {
		m_window = util::MakeReferred(window, true);
		return *this;
	}

	D3D12RendererBuilder& D3D12RendererBuilder::SetFrameCount(std::uint32_t frame_count) noexcept {
		m_frame_count = frame_count;
		return *this;
	}

	D3D12RendererBuilder& D3D12RendererBuilder::SetHeapPoolSizeMap(HeapPoolSizeMap const& map) noexcept {
		m_heap_pool_size_map = map;
		return *this;
	}

#endif // WIN32

}