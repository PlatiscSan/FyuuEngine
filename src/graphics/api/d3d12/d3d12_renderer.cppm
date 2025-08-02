module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

export module d3d12_renderer;
export import renderer_interface;
export import win32_window;
import std;
import concurrent_hash_map;

namespace graphics::api::d3d12 {
#ifdef _WIN32
	static inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
#ifdef _DEBUG
#ifdef UNICODE
			auto str = std::wformat(L"**ERROR** Fatal Error with HRESULT of {}", hr);
#else
			auto str = std::format(L"**ERROR** Fatal Error with HRESULT of {}", hr);
#endif
			OutputDebugString(str.data());
			__debugbreak();
#endif
			throw _com_error(hr);
		}
	}

	class D3D12Shader : public IShader {
	private:
		friend class D3D12RenderDevice;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

	public:
		D3D12Shader(Microsoft::WRL::ComPtr<ID3D12PipelineState> const& pso)
			: m_pso(pso) {
		}

		void Bind() const override {

		}


	};

	export class D3D12RenderDevice : public IRenderDevice {
	private:
		struct ConstantBuffer {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			std::uint8_t* mapped_ptr = nullptr;
			std::uint32_t size = 0u;
		};

		Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		std::uint32_t m_rtv_descriptor_size;
		std::uint32_t m_frame_index;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_render_targets;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fence_event;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srv_heap;
		std::uint32_t m_srv_descriptor_size;
		std::uint32_t m_next_srv_index = 0;
		std::vector<ConstantBuffer> m_constant_buffers;
		std::uint64_t m_fence_value;
		concurrency::ConcurrentHashMap<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pso_cache;

		Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory() {
#if defined(_DEBUG)
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

		static void GetHardwareAdapter(
			Microsoft::WRL::ComPtr<IDXGIFactory1> const& factory,
			Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter,
			bool request_high_performance_adapter = true
		) {

			adapter = nullptr;

			Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
			if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
			{
				for (
					UINT adapterIndex = 0;
					SUCCEEDED(factory6->EnumAdapterByGpuPreference(
						adapterIndex,
						request_high_performance_adapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
						IID_PPV_ARGS(&adapter)));
						++adapterIndex) {

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
				for (UINT adapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
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

		}

		static Microsoft::WRL::ComPtr<ID3D12Device> CreateDevice(
			Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
			bool use_warp = false
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
				Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
				GetHardwareAdapter(factory, hardware_adapter);

				ThrowIfFailed(D3D12CreateDevice(
					hardware_adapter.Get(),
					D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&device)
				));
			}

			return device;

		}

		static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {

			Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;

			D3D12_COMMAND_QUEUE_DESC queue_desc = {};
			queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

			return command_queue;

		}

		static Microsoft::WRL::ComPtr<IDXGISwapChain3> CreateSwapChain(
			platform::Win32Window& window,
			Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> const& command_queue,
			std::uint32_t frame_count = 2u
		) {

			auto hwnd = reinterpret_cast<HWND>(window.Native());
			auto [width, height] = window.GetWidthAndHeight();

			DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
			swap_chain_desc.BufferCount = frame_count;
			swap_chain_desc.Width = width;
			swap_chain_desc.Height = height;
			swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swap_chain_desc.SampleDesc.Count = 1;

			Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
			Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain3;
			ThrowIfFailed(factory->CreateSwapChainForHwnd(
				command_queue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
				hwnd,
				&swap_chain_desc,
				nullptr,
				nullptr,
				&swap_chain
			));

			// This sample does not support fullscreen transitions.
			ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
			ThrowIfFailed(swap_chain.As(&swap_chain3));

			return swap_chain3;

		}

		static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateRTVHeap(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			std::uint32_t frame_count = 2u
		) {
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
			D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
			rtv_heap_desc.NumDescriptors = frame_count;
			rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)));

			return rtv_heap;
		}

		static std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> CreateFrameResource(
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap,
			Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			UINT m_rtv_descriptor_size,
			std::uint32_t frame_count = 2u
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

		static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
			return command_allocator;
		}

		static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
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

		static Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
			return fence;
		}

		static HANDLE CreateFenceEvent() {
			HANDLE fence_event = CreateEvent(nullptr, false, false, nullptr);
			if (!fence_event) {
				throw std::runtime_error("Fence event creation failed");
			}
			return fence_event;
		}

		static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateSRVHeap(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_heap;
			D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
			srv_heap_desc.NumDescriptors = 128u;
			srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_heap)));

			return srv_heap;
		}

		static std::vector<ConstantBuffer> CreateConstantBuffers(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {

			std::vector<ConstantBuffer> constant_buffers(4u);
			CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
			auto rsc_desc = CD3DX12_RESOURCE_DESC::Buffer(256); // initialized with 256 bytes..

			for (auto& cb : constant_buffers) {
				ThrowIfFailed(device->CreateCommittedResource(
					&heap_properties,
					D3D12_HEAP_FLAG_NONE,
					&rsc_desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&cb.resource)));

				CD3DX12_RANGE readRange(0, 0);
				ThrowIfFailed(cb.resource->Map(0, &readRange, reinterpret_cast<void**>(&cb.mapped_ptr)));
				cb.size = 256;
			}

			return constant_buffers;

		}

		void WaitForPreviousFrame() {
			std::uint64_t const fence = m_fence_value;
			ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), fence));
			++m_fence_value;

			if (m_fence->GetCompletedValue() < fence) {
				ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fence_event));
				WaitForSingleObject(m_fence_event, INFINITE);
			}

			m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE CreateTextureSRV(
			ID3D12Resource* texture,
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM
		) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format = format;
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = 1;

			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
				m_srv_heap->GetCPUDescriptorHandleForHeapStart(),
				m_next_srv_index,
				m_srv_descriptor_size);

			m_device->CreateShaderResourceView(texture, &srv_desc, handle);
			++m_next_srv_index;

			return handle;
		}

		Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
			const std::filesystem::path& path,
			std::string_view entry,
			std::string_view target
		) {
			Microsoft::WRL::ComPtr<ID3DBlob> shader;
			Microsoft::WRL::ComPtr<ID3DBlob> error;

			UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
			flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

			std::wstring wide_path = path.wstring();
			HRESULT hr = D3DCompileFromFile(
				wide_path.c_str(),
				nullptr,
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entry.data(),
				target.data(),
				flags,
				0,
				&shader,
				&error
			);

			if (FAILED(hr)) {
				if (error) {
					OutputDebugString(static_cast<TCHAR*>(error->GetBufferPointer()));
				}
				throw std::runtime_error("Shader compilation failed");
			}

			return shader;
		}

	public:
		D3D12RenderDevice(platform::Win32Window& window, bool use_warp = false, std::uint32_t frame_count = 2u)
			: m_factory(D3D12RenderDevice::CreateFactory()),
			m_device(D3D12RenderDevice::CreateDevice(m_factory, use_warp)),
			m_command_queue(D3D12RenderDevice::CreateCommandQueue(m_device)),
			m_swap_chain(D3D12RenderDevice::CreateSwapChain(window, m_factory, m_command_queue, frame_count)),
			m_rtv_heap(D3D12RenderDevice::CreateRTVHeap(m_device, frame_count)),
			m_rtv_descriptor_size(m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
			m_render_targets(D3D12RenderDevice::CreateFrameResource(m_rtv_heap, m_swap_chain, m_device, m_rtv_descriptor_size, frame_count)),
			m_command_allocator(D3D12RenderDevice::CreateCommandAllocator(m_device)),
			m_command_list(D3D12RenderDevice::CreateCommandList(m_device, m_command_allocator)),
			m_fence(D3D12RenderDevice::CreateFence(m_device)),
			m_fence_event(D3D12RenderDevice::CreateFenceEvent()),
			m_srv_heap(D3D12RenderDevice::CreateSRVHeap(m_device)),
			m_srv_descriptor_size(m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
			m_constant_buffers(D3D12RenderDevice::CreateConstantBuffers(m_device)) {

		}

		~D3D12RenderDevice() noexcept override {
			WaitForPreviousFrame();
			CloseHandle(m_fence_event);
		}

		std::unique_ptr<IShader> CreateShader(const std::filesystem::path& vs_src, const std::filesystem::path& fs_src) override {
			
			auto vertex_shader = CompileShader(vs_src, "main", "vs_5_0");
			auto pixel_shader = CompileShader(fs_src, "main", "ps_5_0");

			// create root signature
			CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
			root_signature_desc.Init(0, nullptr, 0, nullptr,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(
				&root_signature_desc,
				D3D_ROOT_SIGNATURE_VERSION_1,
				&signature,
				&error));

			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
			ThrowIfFailed(m_device->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&root_signature)));

			// 创建输入布局（简化版）
			D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// 创建管线状态对象
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
			pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
			pso_desc.pRootSignature = root_signature.Get();
			pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
			pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };
			pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			pso_desc.DepthStencilState.DepthEnable = FALSE;
			pso_desc.DepthStencilState.StencilEnable = FALSE;
			pso_desc.SampleMask = UINT_MAX;
			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			pso_desc.SampleDesc.Count = 1;

			Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)));

			// 缓存PSO
			size_t hash = std::hash<std::string>{}(vs_src.string() + fs_src.string());
			m_pso_cache[hash] = pso;

			return std::make_unique<D3D12Shader>(pso);

		}

		std::unique_ptr<ITexture> CreateTexture(int width, uint32_t height, const void* data) override {
			return nullptr;
		}

		std::unique_ptr<IVertexBuffer> CreateVertexBuffer() override {
			return nullptr;
		}

		void Clear(float r, float g, float b, float a) override {

		}

		void SetViewport(int x, int y, uint32_t width, uint32_t height) override {

		}

		void DrawTriangles(uint32_t vertex_count) override {

		}

	};
#endif // _WIN32
}