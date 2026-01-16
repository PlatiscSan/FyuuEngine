module;
#ifdef _WIN32
#include <d3d12.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // _WIN32

export module fyuu_rhi:d3d12_command_object;
import std;
import plastic.any_pointer;
import plastic.unique_resource;
import plastic.registrable;
import plastic.circular_buffer;
import :d3d12_common;
export import :command_object;


namespace fyuu_rhi::d3d12 {
#ifdef _WIN32

	export class D3D12CommandBuffer
		: public ICommandBuffer {
	private:
		plastic::utility::AnyPointer<ID3D12GraphicsCommandList2> m_impl;

	public:
		D3D12CommandBuffer(plastic::utility::AnyPointer<ID3D12GraphicsCommandList2> const& command_list);

		/// @brief for single time use
		/// @param logical_device 
		/// @param buffer_type 
		D3D12CommandBuffer(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
			CommandObjectType buffer_type
		);

		D3D12CommandBuffer(
			plastic::utility::AnyPointer<ID3D12Device2> const& logical_device,
			D3D12_COMMAND_LIST_TYPE type
		);

		D3D12CommandBuffer(D3D12CommandBuffer&& other) noexcept;

		plastic::utility::AnyPointer<ID3D12GraphicsCommandList2> GetNative() const noexcept;

		ID3D12GraphicsCommandList2* GetRawNative() const noexcept;

	};

	export class D3D12CommandAllocator
		: public plastic::utility::Registrable<D3D12CommandAllocator>,
		public ICommandAllocator {
	private:
		D3D12_COMMAND_LIST_TYPE m_type;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_impl;
		plastic::concurrency::CircularBuffer<D3D12CommandBuffer, 16ull> m_free_command_buffers;
		
	public:
		class CommandBufferGC;
		using UniqueCommandBuffer = plastic::utility::UniqueResource<D3D12CommandBuffer, CommandBufferGC>;

		D3D12CommandAllocator(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
			CommandObjectType allocator_type
		);

		UniqueCommandBuffer Allocate();

	};

	class D3D12CommandAllocator::CommandBufferGC {
	private:
		std::size_t m_allocator_id;
	
	public:
		CommandBufferGC(std::size_t allocator_id) noexcept;
		CommandBufferGC(CommandBufferGC const& other) noexcept;
		CommandBufferGC(CommandBufferGC&& other) noexcept;

		void operator()(D3D12CommandBuffer& command_buffer) noexcept;
	};

	export class D3D12CommandQueue 
		: public plastic::utility::EnableSharedFromThis<D3D12CommandQueue>,
		public ICommandQueue {
		friend class ICommandQueue;

	private:
		std::size_t m_logical_device_id;
		D3D12_COMMAND_LIST_TYPE m_command_list_type;
		std::uint64_t m_fence_value;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_impl;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		EventHandle m_fence_event;

		void WaitUntilCompletedImpl();

		std::uint64_t ExecuteCommandImpl(D3D12CommandBuffer& command_buffer);

	public:
		D3D12CommandQueue(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
			CommandObjectType queue_type,
			QueuePriority priority = QueuePriority::High
		);

		D3D12CommandQueue(
			D3D12LogicalDevice const& logical_device,
			CommandObjectType queue_type,
			QueuePriority priority = QueuePriority::High
		);

		D3D12_COMMAND_LIST_TYPE GetType() const noexcept;

		std::uint64_t Signal();
		bool IsComplete(std::uint64_t fence_value);
		void Wait(std::uint64_t fence_value);
		void Flush();



		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetNative() const noexcept;

		ID3D12CommandQueue* GetRawNative() const noexcept;

	};

#endif // _WIN32

}