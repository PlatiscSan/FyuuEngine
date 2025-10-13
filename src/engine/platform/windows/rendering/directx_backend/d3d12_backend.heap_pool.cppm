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

export module d3d12_backend:heap_pool;
import std;
import concurrent_vector;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	export struct D3D12HeapChunk {
		Microsoft::WRL::ComPtr<ID3D12Heap> heap;
		std::size_t offset;
		std::size_t size;

		D3D12HeapChunk() = default;
		D3D12HeapChunk(D3D12HeapChunk const&) = delete;
		D3D12HeapChunk(D3D12HeapChunk&& other) noexcept;
		D3D12HeapChunk& operator=(D3D12HeapChunk&& other) noexcept;
	};

	export class D3D12HeapPool {
	private:
		struct HeapEntry {
			Microsoft::WRL::ComPtr<ID3D12Heap> heap;
			std::vector<D3D12HeapChunk> free_chunks;
		};

		std::variant<std::monostate, ID3D12Device*, Microsoft::WRL::ComPtr<ID3D12Device>> m_device;
		D3D12_HEAP_TYPE m_heap_type;
		D3D12_HEAP_FLAGS m_heap_flags;
		std::size_t m_heap_size;
		std::size_t m_min_allocation;

		concurrency::ConcurrentVector<HeapEntry> m_heaps;

		Microsoft::WRL::ComPtr<ID3D12Heap> CreateHeap(std::size_t size) const;
		std::optional<D3D12HeapChunk> AllocateFromHeap(HeapEntry& heap_entry, std::size_t size, std::size_t alignment) const;
		void MergeFreeChunks(HeapEntry& heap_entry) const;

	public:
		D3D12HeapPool() = default;
		D3D12HeapPool(
			ID3D12Device* device,
			D3D12_HEAP_TYPE heap_type,
			D3D12_HEAP_FLAGS heap_flags,
			std::size_t heap_size,
			std::size_t min_allocation
		);

		D3D12HeapPool(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			D3D12_HEAP_TYPE heap_type,
			D3D12_HEAP_FLAGS heap_flags,
			std::size_t heap_size,
			std::size_t min_allocation
		);

		std::optional<D3D12HeapChunk> Allocate(std::size_t size, std::size_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		void Free(D3D12HeapChunk& chunk);
		void Free(std::optional<D3D12HeapChunk>& chunk);

		D3D12_HEAP_TYPE GetPoolType() const noexcept;
		std::size_t GetHeapSize() const noexcept;
		std::size_t GetMinimumAllocation() const noexcept;

	};

#endif // WIN32

}