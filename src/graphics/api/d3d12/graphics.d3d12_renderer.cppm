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

#ifdef interface
#undef interface
#endif // interface

export module graphics:d3d12_renderer;
export import :d3d12_descriptor_resource;

namespace graphics::api::d3d12 {
#ifdef _WIN32

	class FrameContext {
	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
		std::uint32_t m_frame_index;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_render_target;
		DescriptorResourcePool m_descriptor_pool;
		std::vector<ID3D12CommandList*> m_ready_command_lists;
		std::mutex m_ready_command_lists_mutex;
		std::uint64_t m_fence_value;

	public:
		static std::vector<FrameContext> CreateFrames(
			std::uint32_t frame_count,
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> const& rtv_heap,
			Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
			std::uint32_t resource_descriptor_count = 1024u
		);

		FrameContext(
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle,
			std::uint32_t frame_index,
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain,
			std::uint32_t resource_descriptor_count = 1024u
		);

		FrameContext(FrameContext const&) = delete;
		FrameContext& operator=(FrameContext const&) = delete;

		FrameContext(FrameContext&& other) noexcept;
		FrameContext& operator=(FrameContext&& other) noexcept;

		~FrameContext() noexcept;

		DescriptorResourcePool::UniqueDescriptorHandle AcquireResourceDescriptorHandle() noexcept;

		std::uint64_t GetFenceValue() const noexcept;

		D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const noexcept;

		Microsoft::WRL::ComPtr<ID3D12Resource> GetRenderTarget() const noexcept {
			return m_render_target;
		}

		void CommitCommands(ID3D12CommandList* list);
		std::vector<ID3D12CommandList*> FetchAllCommands() noexcept;

		void ResetRenderTarget(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			Microsoft::WRL::ComPtr<IDXGISwapChain3> const& swap_chain
		);

		void SignalFence(std::uint64_t fence_value) noexcept;

		void ReleaseRenderTarget() noexcept;

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

	extern Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	);

	extern HANDLE CreateFenceEvent();

	export class D3D12RenderDevice final : public BaseRenderDevice {
	private:
		/*
		*	DirectX12 Core
		*/

		Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fence_event;
		HANDLE m_swap_chain_waitable_object;
		std::uint64_t m_fence_value;

		/// @brief contexts for each frame
		std::vector<FrameContext> m_frames;

		boost::uuids::uuid m_msg_processor_uuid;

		util::Subscriber* m_command_ready_subscription;

		std::size_t m_next_frame_index;
		std::size_t m_previous_frame_index = 0;


		/// @brief An atomic flag indicating if the rendering thread can submit commands
		std::atomic_flag m_allow_submission = {};

		bool m_swap_chain_occluded = false;

		void WaitForNextFrame();
		void WaitForLastSubmittedFrame();

		LRESULT HandleResize(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		void OnCommandReady(CommandReadyEvent const& e);

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		D3D12RenderDevice(Logger&& logger, platform::Win32Window& window, bool use_warp = false, std::uint32_t frame_count = 2u)
			: BaseRenderDevice(std::forward<Logger>(logger), window),
			m_factory(CreateFactory()),
			m_device(CreateDevice(m_factory, use_warp)),
			m_rtv_heap(CreateRTVHeap(m_device, frame_count)),
			m_command_queue(CreateCommandQueue(m_device)),
			m_swap_chain(CreateSwapChain(static_cast<platform::Win32Window*>(m_window), m_factory, m_command_queue, frame_count)),
			m_fence(CreateFence(m_device)),
			m_fence_event(CreateFenceEvent()),
			m_swap_chain_waitable_object(m_swap_chain->GetFrameLatencyWaitableObject()),
			m_fence_value(0u),
			m_frames(FrameContext::CreateFrames(frame_count, m_device, m_rtv_heap, m_swap_chain)),	
			m_msg_processor_uuid(
				window.AttachMsgProcessor(
					[this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
						return HandleResize(hwnd, msg, wparam, lparam);
					}
				)
			),
			m_command_ready_subscription(
				m_message_bus.Subscribe<CommandReadyEvent>(
					[this](CommandReadyEvent const& e) {
						OnCommandReady(e);
					}
				)
			),
			m_next_frame_index(m_swap_chain->GetCurrentBackBufferIndex()) {

		}

		~D3D12RenderDevice() noexcept override;

		Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() const noexcept;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const noexcept;

		std::size_t FrameCount() const noexcept;

		DescriptorResourcePool::UniqueDescriptorHandle AcquireResourceDescriptorHandle() noexcept;

		DescriptorResourcePool CreateDescriptorResourcePool(D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count) const;

		void Clear(float r, float g, float b, float a) override;
		void SetViewport(int x, int y, uint32_t width, uint32_t height) override;

		bool BeginFrame() override;
		void EndFrame() override;

		API GetAPI() const noexcept override;

		ICommandObject& AcquireCommandObject() override;

	};
#endif // _WIN32
}