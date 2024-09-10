#include "pch.h"
#include "CommandAllocatorPool.h"

using namespace Fyuu::graphics::d3d12;
using namespace Microsoft::WRL;

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Fyuu::graphics::d3d12::CommandAllocatorPool::RequestAllocator(std::uint64_t completed_fence_value) {

	std::lock_guard<std::mutex> lock(m_allocator_mutex);

	ComPtr<ID3D12CommandAllocator> allocator;

	if (!m_ready_allocators.empty()) {

		auto& allocator_pair = m_ready_allocators.front();

		if (allocator_pair.first <= completed_fence_value) {

			allocator = allocator_pair.second;
			ThrowIfFailed(allocator->Reset());
			m_ready_allocators.pop();

		}

	}

	if (!allocator) {

		ThrowIfFailed(m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&allocator)));
		m_allocators.push_back(allocator);

	}

	return allocator;

}

void Fyuu::graphics::d3d12::CommandAllocatorPool::DiscardAllocator(std::uint64_t fence_value, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& allocator) {

	std::lock_guard<std::mutex> lock(m_allocator_mutex);
	m_ready_allocators.emplace(fence_value, allocator);

}

Fyuu::graphics::d3d12::CommandAllocatorPool::~CommandAllocatorPool() {

	for (auto& allocator : m_allocators) {
		allocator->Release();
	}

	m_allocators.clear();

}