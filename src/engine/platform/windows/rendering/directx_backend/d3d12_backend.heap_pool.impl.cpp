module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module d3d12_backend:heap_pool;
import :util;

#ifdef max
#undef max
#endif

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	D3D12HeapChunk::D3D12HeapChunk(D3D12HeapChunk&& other) noexcept
		: heap(std::move(other.heap)),
		offset(std::exchange(other.offset, 0)),
		size(std::exchange(other.size, 0)) {
	}

	D3D12HeapChunk& D3D12HeapChunk::operator=(D3D12HeapChunk&& other) noexcept {
		if(this != &other) {
			heap = std::move(other.heap);
			offset = std::exchange(other.offset, 0);
			size = std::exchange(other.size, 0);
		}
		return *this;
	}

	Microsoft::WRL::ComPtr<ID3D12Heap> D3D12HeapPool::CreateHeap(std::size_t size) const {

		D3D12_HEAP_DESC desc{};
		desc.SizeInBytes = size;
		desc.Properties.Type = m_heap_type;
		desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		desc.Flags = m_heap_flags;

		Microsoft::WRL::ComPtr<ID3D12Heap> heap;
		ID3D12Device* device = std::visit(
			[](auto&& device) -> ID3D12Device* {
				using Device = std::decay_t<decltype(device)>;
				if constexpr (std::is_same_v<Device, ID3D12Device*>) {
					return device;
				}
				else if constexpr (std::is_same_v<Device, Microsoft::WRL::ComPtr<ID3D12Device>>) {
					return device.Get();
				}
				else {
					return nullptr;
				}
			},
			m_device
		);

		ThrowIfFailed(device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
		
		return heap;

	}

	std::optional<D3D12HeapChunk> D3D12HeapPool::AllocateFromHeap(
		HeapEntry& heap_entry,
		std::size_t size, 
		std::size_t alignment
	) const {

		auto& free_chunks = heap_entry.free_chunks;
		if (free_chunks.empty()) {
			return std::nullopt;
		}

		/*
		*	best fit algorithm
		*/

		auto best_fit_iter = free_chunks.end();
		std::size_t best_fit_waste = std::numeric_limits<std::size_t>::max();

		for (auto it = free_chunks.begin(); it != free_chunks.end(); ++it) {

			std::size_t aligned_offset = (it->offset + alignment - 1) & ~(alignment - 1);
			std::size_t padding = aligned_offset - it->offset;
			std::size_t required_size = padding + size;

			if (it->size >= required_size) {

				std::size_t waste = it->size - required_size;

				if (waste < best_fit_waste) {
					best_fit_waste = waste;
					best_fit_iter = it;
				}
			}
		}


		if (best_fit_iter == free_chunks.end()) {
			return std::nullopt;
		}

		D3D12HeapChunk best_chunk = std::move(*best_fit_iter);
		free_chunks.erase(best_fit_iter);


		std::size_t aligned_offset = (best_chunk.offset + alignment - 1) & ~(alignment - 1);
		std::size_t padding = aligned_offset - best_chunk.offset;
		std::size_t remaining_size = best_chunk.size - padding - size;


		if (padding > 0) {
			/*
			*	push back as free chunk if front padding exists
			*/

			D3D12HeapChunk front_chunk;
			front_chunk.heap = best_chunk.heap;
			front_chunk.offset = best_chunk.offset;
			front_chunk.size = padding;
			free_chunks.emplace_back(std::move(front_chunk));
		}

		if (remaining_size > 0) {
			/*
			*	push as free chunk if there is remaining
			*/


			D3D12HeapChunk back_chunk;
			back_chunk.heap = best_chunk.heap;
			back_chunk.offset = aligned_offset + size;
			back_chunk.size = remaining_size;
			free_chunks.emplace_back(std::move(back_chunk));
		}

		D3D12HeapChunk allocated_chunk;
		allocated_chunk.heap = best_chunk.heap;
		allocated_chunk.offset = aligned_offset;
		allocated_chunk.size = size;

		return allocated_chunk;

	}

	void D3D12HeapPool::MergeFreeChunks(HeapEntry& heap_entry) const {
		
		/*
		*	sort chunks by offset
		*/

		auto& free_chunks = heap_entry.free_chunks;

		std::sort(
			free_chunks.begin(),
			free_chunks.end(),
			[](D3D12HeapChunk const& a, D3D12HeapChunk const& b) {
				return a.offset < b.offset;
			}
		);

		/*
		*	merge adjacent free chunks
		*/

		auto iter = free_chunks.begin();
		while (iter != free_chunks.end()) {
			
			auto next_iter = std::next(iter);
			if (next_iter != free_chunks.end() &&
				iter->offset + iter->size == next_iter->offset) {

				/*
				*	merge chunk
				*/

				iter->size += next_iter->size;
				free_chunks.erase(next_iter);

				/*
				*	do not advance, continue determining if more chunks can be merged
				*/

			}
			else {
				++iter;
			}

		}

	}

	D3D12HeapPool::D3D12HeapPool(
		ID3D12Device* device,
		D3D12_HEAP_TYPE heap_type, 
		D3D12_HEAP_FLAGS heap_flags, 
		size_t heap_size, 
		size_t min_allocation
	) : m_device(std::in_place_type<ID3D12Device*>, device),
		m_heap_type(heap_type),
		m_heap_flags(heap_flags),
		m_heap_size(heap_size),
		m_min_allocation(min_allocation == 0 ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : min_allocation) {

	}
	D3D12HeapPool::D3D12HeapPool(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		D3D12_HEAP_TYPE heap_type, 
		D3D12_HEAP_FLAGS heap_flags, 
		size_t heap_size, 
		size_t min_allocation
	) : m_device(std::in_place_type<Microsoft::WRL::ComPtr<ID3D12Device>>, device),
		m_heap_type(heap_type),
		m_heap_flags(heap_flags),
		m_heap_size(heap_size),
		m_min_allocation(min_allocation == 0 ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : min_allocation) {
	}

	std::optional<D3D12HeapChunk> D3D12HeapPool::Allocate(std::size_t size, std::size_t alignment) {

		alignment = alignment == 0 ? 
			alignment : 
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		std::size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
		aligned_size = aligned_size < m_min_allocation ?
			m_min_allocation :
			aligned_size;

		{
			auto locked_modifier = m_heaps.LockedModifier();
			for (auto& heap_entry : locked_modifier) {
				if (std::optional<D3D12HeapChunk> chunk = AllocateFromHeap(heap_entry, size, alignment)) {
					return chunk;
				}
			}
		}

		/*
		*	No suitable chunk, create new heap
		*/

		std::size_t heap_size = (std::max)(m_heap_size, aligned_size * 2);
		Microsoft::WRL::ComPtr<ID3D12Heap> heap = CreateHeap(heap_size);

		HeapEntry entry{};
		entry.heap = heap;

		D3D12HeapChunk initial_chunk{};
		initial_chunk.heap = heap;
		initial_chunk.offset = 0;
		initial_chunk.size = heap_size;

		entry.free_chunks.emplace_back(std::move(initial_chunk));
		auto emplaced_entry = m_heaps.emplace_back(std::move(entry));

		std::optional<D3D12HeapChunk> chunk = AllocateFromHeap(emplaced_entry, size, alignment);

		return chunk;

	}

	void D3D12HeapPool::Free(D3D12HeapChunk& chunk) {

		if (!chunk.heap) {
			return;
		}

		auto locked_modifier = m_heaps.LockedModifier();
		for (auto& heap_entry : locked_modifier) {
			if (heap_entry.heap == chunk.heap) {
				heap_entry.free_chunks.emplace_back(std::move(chunk));
				MergeFreeChunks(heap_entry);
				break;
			}
		}

	}

	void D3D12HeapPool::Free(std::optional<D3D12HeapChunk>& chunk) {

		if (!chunk) {
			return;
		}
		Free(*chunk);
		chunk.reset();

	}

	D3D12_HEAP_TYPE D3D12HeapPool::GetPoolType() const noexcept {
		return m_heap_type;
	}

	std::size_t D3D12HeapPool::GetHeapSize() const noexcept {
		return m_heap_size;
	}

	std::size_t D3D12HeapPool::GetMinimumAllocation() const noexcept {
		return m_min_allocation;
	}

#endif // WIN32

}