#ifndef D3D12_COMMAND_OBJECT_H
#define D3D12_COMMAND_OBJECT_H

#include "D3D12Core.h"
#include "CommandAllocatorPool.h"

namespace Fyuu::graphics::d3d12 {

	struct CommandObject {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> list;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
	};

	class CommandQueue final {

		friend CommandObject CreateNewCommandObject(D3D12_COMMAND_LIST_TYPE type);
		friend class CommandContext;

	public:

		CommandQueue(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device10> const& device);
		~CommandQueue() noexcept;

		std::uint64_t IncreaseFence();
		bool IsFenceCompleted(std::uint64_t fence_value);
		void StallForFence(std::uint64_t fence_value);
		void StallForProducer(CommandQueue const& producer);
		void WaitForFence(std::uint64_t fence_value);

		void WaitForIdle() { WaitForFence(IncreaseFence()); }

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> const& GetCommandQueue() const noexcept {
			return m_command_queue;
		}

	private:

		std::uint64_t ExecuteCommandList(ID3D12CommandList* list);
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestAllocator();
		void DiscardAllocator(std::uint64_t fence_value_for_reset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& allocator);

		std::unique_ptr<CommandAllocatorPool> m_allocator_pool;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;

		std::mutex m_fence_mutex;
		Microsoft::WRL::ComPtr<ID3D12Fence1> m_fence;

		std::uint64_t m_next_fence_value;
		std::uint64_t m_last_completed_fence_value;

		std::mutex m_fence_event_mutex;
		HANDLE m_fence_event;

		D3D12_COMMAND_LIST_TYPE m_type;


	};

	std::shared_ptr<CommandQueue> const& Queue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	std::shared_ptr<CommandQueue> const& GraphicsQueue() noexcept;
	std::shared_ptr<CommandQueue> const& ComputeQueue() noexcept;
	std::shared_ptr<CommandQueue> const& CopyQueue() noexcept;

	CommandObject CreateNewCommandObject(D3D12_COMMAND_LIST_TYPE type);

	void InitializeCommandListManager();
	bool IsFenceCompleted(std::uint64_t fence_value);
	void WaitForFence(std::uint64_t fence_value);
	void IdleGPU();
	

}

#endif