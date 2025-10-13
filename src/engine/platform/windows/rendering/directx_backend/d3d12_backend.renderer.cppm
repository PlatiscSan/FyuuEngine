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

export module d3d12_backend:renderer;
export import :command_object;
export import simple_logger;
export import windows_window;
export import collective_resource;
import :heap_pool;
import rendering;
import std;
import circular_buffer;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	struct FrameContext {
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		D3D12_CPU_DESCRIPTOR_HANDLE render_target_descriptor;
		std::uint64_t fence_value;
		concurrency::CircularBuffer<ID3D12CommandList*, 128u> ready_commands;

		FrameContext(Microsoft::WRL::ComPtr<ID3D12Resource> const& buffer_, D3D12_CPU_DESCRIPTOR_HANDLE render_target_descriptor_);
	};

	export struct D3D12RendererTraits {
		using CommandObjectType = D3D12CommandObject;
		using CollectiveBufferType = UniqueD3D12Buffer;
		using OutputTargetType = D3D12RenderTarget;
	};

	export class D3D12Renderer final : public core::BaseRenderer<D3D12Renderer, D3D12RendererTraits> {
		friend class core::BaseRenderer<D3D12Renderer, D3D12RendererTraits>;

	private:
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		util::PointerWrapper<WindowsWindow> m_window;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_render_target_heap;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fence_event;
		HANDLE m_swap_chain_waitable_object;
		std::uint64_t m_fence_value;
		std::shared_ptr<concurrency::ISubscription> m_command_ready_subscription;

		std::unordered_map<HeapPoolType, D3D12HeapPool> m_heap_pools;

		std::uint32_t m_current_frame_index;
		std::uint32_t m_previous_frame_index;
		std::vector<FrameContext> m_frames;
		boost::uuids::uuid m_msg_processor_uuid;

		std::atomic_flag m_allow_submission = {};
		bool m_swap_chain_occluded;

		bool BeginFrameImpl();

		void EndFrameImpl();

		D3D12CommandObject& GetCommandObjectImpl() noexcept;

		D3D12HeapPool* PickHeapPool(core::BufferDescriptor const& desc);

		UniqueD3D12Buffer CreateBufferImpl(core::BufferDescriptor const& desc);

		D3D12RenderTarget OutputTargetOfCurrentFrameImpl();

		void WaitForLastSubmittedFrame();

		void WaitForFrame();

		void CreateFrames(std::uint32_t frame_count);

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		void OnCommandReady(D3D12CommandListReady e);

	public:
		using Base = core::BaseRenderer<D3D12Renderer, D3D12RendererTraits>;

		D3D12Renderer(
			core::LoggerPtr const& logger,
			Microsoft::WRL::ComPtr<IDXGIFactory6> const& factory,
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			util::PointerWrapper<WindowsWindow> const& window,
			std::uint32_t frame_count,
			HeapPoolSizeMap& heap_size_map
		);

		D3D12Renderer(D3D12Renderer&& other) noexcept;
		~D3D12Renderer();
		D3D12Renderer& operator=(D3D12Renderer&& other) noexcept;

		Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const noexcept;

	};


	export class D3D12RendererBuilder :
		public util::EnableSharedFromThis<D3D12RendererBuilder>,
		public core::BaseRendererBuilder<D3D12RendererBuilder, D3D12Renderer> {

		friend class core::BaseRendererBuilder<D3D12RendererBuilder, D3D12Renderer>;

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		HeapPoolSizeMap m_heap_pool_size_map;
		util::PointerWrapper<WindowsWindow> m_window;
		std::uint32_t m_frame_count = 2u;
		bool m_use_software = false;
		bool m_use_high_performance_adapter = true;

		static Microsoft::WRL::ComPtr<IDXGIFactory6> CreateFactory();
		Microsoft::WRL::ComPtr<IDXGIAdapter1> QueryHardwareAdapter();
		Microsoft::WRL::ComPtr<ID3D12Device> CreateDevice();

		std::shared_ptr<D3D12Renderer> BuildImpl();
		void BuildImpl(std::optional<D3D12Renderer>& buffer);
		void BuildImpl(std::span<std::byte> buffer);

	public:
		D3D12RendererBuilder();

		D3D12RendererBuilder& UseSoftware(bool use_software = true) noexcept;
		D3D12RendererBuilder& UseHighPerformanceAdapter(bool use_high_performance_adapter = true) noexcept;
		D3D12RendererBuilder& SetTargetWindow(util::PointerWrapper<WindowsWindow> const& window);
		D3D12RendererBuilder& SetFrameCount(std::uint32_t frame_count) noexcept;
		D3D12RendererBuilder& SetHeapPoolSizeMap(HeapPoolSizeMap const& map) noexcept;
	};

#endif // WIN32

}