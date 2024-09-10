#ifndef COMMAND_ALLOCATOR_POOL_H
#define COMMAND_ALLOCATOR_POOL_H

#include <mutex>
#include <queue>
#include <vector>

#include "D3D12Core.h"

namespace Fyuu::graphics::d3d12 {

	class CommandAllocatorPool final {

	public:

		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device10> const& device)
			:m_type(type), m_device(device) {};

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestAllocator(std::uint64_t completed_fence_value);
		void DiscardAllocator(std::uint64_t fence_value, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& allocator);

		~CommandAllocatorPool();

		std::size_t const& GetSize() const noexcept { return m_allocators.size(); }

	private:

		Microsoft::WRL::ComPtr<ID3D12Device10> m_device;

		std::mutex m_allocator_mutex;
		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocators;
		std::queue<std::pair<std::uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>> m_ready_allocators;
		D3D12_COMMAND_LIST_TYPE m_type;

	};

}

#endif // !COMMAND_ALLOCATOR_POOL_H
