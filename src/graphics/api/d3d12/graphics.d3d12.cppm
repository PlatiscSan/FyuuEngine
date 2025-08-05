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

	export class D3D12RenderDevice : public IRenderDevice {
	private:
		platform::Win32Window* m_window;
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
		bool m_swap_chain_occluded;

		FrameContext& WaitForNextFrameResource();
		void WaitForLastSubmittedFrame();
		void CleanUpRenderTargets();

	public:
		D3D12RenderDevice(platform::Win32Window& window, bool use_warp = false, std::uint32_t frame_count = 2u);
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