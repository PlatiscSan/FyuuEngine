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

#ifdef interface
#undef interface
#endif // interface


export module graphics:d3d12;
export import window;
import std;
import :interface;

namespace graphics::api::d3d12 {
#ifdef _WIN32
		
	struct FrameContext {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
		std::uint64_t fence_value;
		FrameContext(Microsoft::WRL::ComPtr<ID3D12Device> const& device);
	};

	export struct DescriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
	};

	extern Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory();

	extern Microsoft::WRL::ComPtr<ID3D12Device> CreateDevice(
		Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
		bool use_warp = false
	);

	extern Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	);

	extern Microsoft::WRL::ComPtr<IDXGISwapChain3> CreateSwapChain(
		platform::Win32Window* window,
		Microsoft::WRL::ComPtr<IDXGIFactory4> const& factory,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> const& command_queue,
		std::uint32_t frame_count = 2u
	);

	extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateRTVHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::uint32_t frame_count = 2u
	);

	extern std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetRenderTargetDescriptors(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap
	);

	extern std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> CreateFrameResource(
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		UINT m_rtv_descriptor_size,
		std::uint32_t frame_count = 2u
	);

	extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	);

	extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& command_allocator
	);

	extern Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	);

	extern HANDLE CreateFenceEvent();

	extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateSRVHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::uint32_t count = 128u
	);

	extern std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> CreateRenderTargets(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> const& render_target_descriptors,
		std::size_t frame_count = 2u
	);

	extern std::deque<graphics::api::d3d12::DescriptorHandle> GetDescriptorHandles(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& heap
	);

	extern std::vector<graphics::api::d3d12::FrameContext> CreateFrameContexts(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		std::size_t frame_count = 2u
	);

	export class D3D12RenderDevice : public BaseRenderDevice {
	private:
		Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_descriptors;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srv_heap;
		std::deque<DescriptorHandle> m_descriptor_handles;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		std::vector<FrameContext> m_frame_contexts;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fence_event;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain;
		HANDLE m_swap_chain_waitable_object;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_render_targets;
		boost::uuids::uuid m_msg_processor_uuid;
		std::uint64_t m_fence_last_signaled_value;
		std::size_t m_frame_index;
		std::array<float, 4u> m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f };
		core::LoggerPtr m_logger;
		bool m_swap_chain_occluded;

		FrameContext& WaitForNextFrameResource();
		void WaitForLastSubmittedFrame();
		void CleanUpRenderTargets();

		LRESULT HandleResize(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		D3D12RenderDevice(Logger&& logger, platform::Win32Window& window, bool use_warp = false, std::uint32_t frame_count = 2u)
			: BaseRenderDevice(std::forward<Logger>(logger), window),
			m_factory(CreateFactory()),
			m_device(CreateDevice(m_factory, use_warp)),
			m_rtv_heap(CreateRTVHeap(m_device, frame_count)),
			m_rtv_descriptors(GetRenderTargetDescriptors(m_device, m_rtv_heap)),
			m_srv_heap(CreateSRVHeap(m_device)),
			m_descriptor_handles(GetDescriptorHandles(m_device, m_srv_heap)),
			m_command_queue(CreateCommandQueue(m_device)),
			m_frame_contexts(CreateFrameContexts(m_device, frame_count)),
			m_command_list(CreateCommandList(m_device, m_frame_contexts[0].command_allocator)),
			m_fence(CreateFence(m_device)),
			m_fence_event(CreateFenceEvent()),
			m_swap_chain(CreateSwapChain(static_cast<platform::Win32Window*>(m_window), m_factory, m_command_queue, frame_count)),
			m_swap_chain_waitable_object(m_swap_chain->GetFrameLatencyWaitableObject()),
			m_render_targets(CreateRenderTargets(m_device, m_swap_chain, m_rtv_descriptors, frame_count)),
			m_msg_processor_uuid(
				window.AttachMsgProcessor(
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
						return HandleResize(hwnd, msg, wparam, lparam);
					}
				)
			),
			m_fence_last_signaled_value(0u),
			m_frame_index(0u) {

		}

		~D3D12RenderDevice() noexcept override;

		Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() const noexcept;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const noexcept;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() const noexcept;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const noexcept;
		std::size_t FrameCount() const noexcept;

		DescriptorHandle AcquireDescriptorHandle() noexcept;
		void ReturnDescriptorHandle(DescriptorHandle& handle);

		void Clear(float r, float g, float b, float a) override;
		void SetViewport(int x, int y, uint32_t width, uint32_t height) override;

		void SetClearColor(float r, float g, float b, float a);

		bool BeginFrame() override;
		void EndFrame() override;

		API GetAPI() const noexcept override;

	};
#endif // _WIN32
}