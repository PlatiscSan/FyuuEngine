/* d3d12_descriptor_allocator.cppm */
// Module declaration: this file implements the private module partition for D3D12 descriptor allocators.
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <array>
#include <atomic>
#include <optional>
#endif
#if defined(_WIN32)
#include <DescriptorHeap.h>   // DirectX Tool Kit for DescriptorHeap wrapper
#endif // defined(_WIN32)
export module fyuu_rhi:d3d12_descriptor_allocator;   // Define the module partition
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;					// Import entire standard library if modules are supported
#endif
import plastic.circular_buffer;   // Lock‑free circular buffer (single producer / single consumer)
import plastic.resource;		  // SharedResource wrapper for RAII
import plastic.registrable;	   // Base class for objects that can be looked up by ID

namespace fyuu_rhi::d3d12 {

	// Descriptor handle returned by the allocator.
	// Contains the index inside the heap, CPU/GPU handles, and the queue ID it came from.
	export struct D3D12DescriptorHandle {
		std::size_t index;					 // Linear index inside the heap (0 .. totalDescriptors-1)
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle; // CPU handle for this descriptor
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle; // GPU handle (only valid if heap is shader‑visible)
		std::size_t queue_id;				   // Which sub‑queue this index belongs to (for release)
	};

	// Template allocator for a specific heap type.
	//   type	  – D3D12_DESCRIPTOR_HEAP_TYPE (e.g. CBV_SRV_UAV, RTV, DSV, SAMPLER)
	//   N		 – total number of descriptors managed by this allocator (default 2048)
	//   QueueSize – size of each per‑thread friendly free‑list (max 63, default 63)
	template <D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t N = 2048u, std::size_t QueueSize = 63ull>
	class D3D12DescriptorAllocator
		: public plastic::utility::Registrable<D3D12DescriptorAllocator<type, N, QueueSize>> {
	private:
		// Enforce QueueSize limits because the circular buffer can hold at most 63 elements.
		static_assert(QueueSize < 64ull, "Each CircularBuffer can hold at most 63 elements");
		static_assert(QueueSize > 0, "QueueSize must be greater than 0");
		static_assert(N > 0, "N must be greater than 0");

		// Number of independent queues needed to cover all descriptors.
		// We round up so that total descriptors = NumQueues * QueueSize (possibly slightly larger than N).
		static constexpr std::size_t NumQueues = (N + QueueSize - 1) / QueueSize;
		static constexpr std::size_t TotalDescriptors = NumQueues * QueueSize;

		// Wrapper around a Direct3D12 descriptor heap (from DirectX Tool Kit).
		DirectX::DescriptorHeap m_impl;

		// Array of lock‑free circular buffers, each holding free indices for one queue.
		// Each buffer is SPSC (single producer, single consumer) safe.
		std::array<plastic::concurrency::CircularBuffer<std::size_t, QueueSize>, NumQueues> m_free_queues;

		// Hint for the next queue to try when allocating (atomic, relaxed consistency).
		alignas(std::hardware_constructive_interference_size)  // Avoid false sharing
			std::atomic_size_t m_next_queue;

		// Thread‑local cache of the last queue that successfully served an allocation for this thread.
		// Helps maintain locality: a thread tends to reuse the same queue.
		static inline thread_local std::size_t s_tls_last_queue;

	public:
		using Base = plastic::utility::Registrable<D3D12DescriptorAllocator<type, N, QueueSize>>;

		// Garbage collector functor used by SharedResource to return a descriptor to the allocator.
		class GC {
		private:
			std::optional<std::size_t> m_allocator_id;   // ID of the allocator that owns the descriptor

		public:
			GC()
				: m_allocator_id() {
			}

			GC(std::optional<std::size_t> const& allocator_id)
				: m_allocator_id(allocator_id) {
			}

			GC(GC const& other) noexcept
				: m_allocator_id(other.m_allocator_id) {
			}

			GC(GC&& other) noexcept
				: m_allocator_id(std::exchange(other.m_allocator_id, 0)) {
			}

			// Called when the SharedResource is destroyed; returns the index to its original queue.
			void operator()(D3D12DescriptorHandle& handle) {
				if (m_allocator_id) {
					auto allocator = plastic::utility::QueryObject<D3D12DescriptorAllocator<type, N, QueueSize>>(*m_allocator_id);
					allocator->Release(handle.index, handle.queue_id);
				}
			}
		};

		// Public type for an allocated descriptor – a SharedResource with automatic return.
		using Allocation = plastic::utility::SharedResource<D3D12DescriptorHandle, GC>;
		
		// Traits to determine whether a heap type should be created as shader‑visible.
		// CBV_SRV_UAV and SAMPLER heaps are typically shader‑visible; RTV/DSV are not.
		template <D3D12_DESCRIPTOR_HEAP_TYPE T>
		struct IsShaderVisible : public std::false_type {};

		template <>
		struct IsShaderVisible<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>
			: public std::true_type {};

		template <>
		struct IsShaderVisible<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>
			: public std::true_type {};

		template <D3D12_DESCRIPTOR_HEAP_TYPE T>
		static constexpr bool IsShaderVisibleValue = IsShaderVisible<T>::value;

		// Constructor for shader‑visible heap types (e.g. CBV_SRV_UAV, SAMPLER).
		// Creates a descriptor heap with D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE.
		D3D12DescriptorAllocator(ID3D12Device* device) requires IsShaderVisibleValue<type> 
			: Base(this),
			m_impl(device, type, D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, TotalDescriptors),
			m_next_queue(0) {
			InitializeQueues();
		}

		// Constructor for non‑shader‑visible heap types (RTV, DSV).
		// Creates a heap without the shader‑visible flag.
		D3D12DescriptorAllocator(ID3D12Device* device)
			: Base(this),
			m_impl(device, type, D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE, TotalDescriptors),
			m_next_queue(0) {
			InitializeQueues();
		}

		// Fill all free queues with the indices they are responsible for.
		void InitializeQueues() {
			for (std::size_t queue_id = 0; queue_id < NumQueues; ++queue_id) {
				std::size_t start_index = queue_id * QueueSize;
				std::size_t end_index = (std::min)(start_index + QueueSize, TotalDescriptors);

				for (std::size_t i = start_index; i < end_index; ++i) {
					m_free_queues[queue_id].emplace_back(i);
				}
			}
		}

		// Return a descriptor index to its original queue.
		// Called by the GC when a SharedResource is destroyed.
		void Release(std::size_t index, std::size_t queue_id) {
			m_free_queues[queue_id].emplace_back(index);
		}

		// Attempt to allocate from a specific queue (non‑blocking).
		// Returns an optional Allocation; if the queue is empty, returns std::nullopt.
		std::optional<Allocation> TryAllocate(std::size_t queue_id) {
			if (auto allocated_index = m_free_queues[queue_id].pop_front(false)) {
				D3D12DescriptorHandle handle{
					*allocated_index,
					m_impl.GetCpuHandle(*allocated_index),
					(m_impl.Flags() & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ?
						m_impl.GetGpuHandle(*allocated_index) :
						D3D12_GPU_DESCRIPTOR_HANDLE{},
					queue_id 
				};

				return Allocation(handle, GC(static_cast<Base*>(this)->GetID()));
			}
			return std::nullopt;
		}

		// Allocate one descriptor, blocking if necessary (spins until success).
		// Uses a two‑phase strategy:
		//   1. Try queues starting from the thread‑local hint (s_tls_last_queue) without waiting.
		//   2. If that fails, try every queue in a loop, this time waiting (blocking pop).
		// Throws std::runtime_error if allocation fails unexpectedly (should never happen if total descriptors ≥ requests).
		Allocation Allocate() {

			std::size_t start_queue = s_tls_last_queue;

			// Phase 1: try each queue once, non‑blocking, starting from the thread‑local hint.
			for (std::size_t attempt = 0; attempt < NumQueues; ++attempt) {
				std::size_t queue_id = (start_queue + attempt) % NumQueues;

				if (auto allocation = TryAllocate(queue_id)) {
					s_tls_last_queue = queue_id;
					m_next_queue.store((queue_id + 1) % NumQueues, std::memory_order::relaxed);
					return std::move(*allocation);
				}
			}

			// Phase 2: all queues were empty on first pass – now try again but block on each.
			for (std::size_t queue_id = 0; queue_id < NumQueues; ++queue_id) {
				if (auto allocated_index = m_free_queues[queue_id].pop_front(true)) {
					s_tls_last_queue = queue_id;
					m_next_queue.store((queue_id + 1) % NumQueues, std::memory_order::relaxed);

					D3D12DescriptorHandle handle{
						*allocated_index,
						m_impl.GetCpuHandle(*allocated_index),
						(m_impl.Flags() & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ?
							m_impl.GetGpuHandle(*allocated_index) :
							D3D12_GPU_DESCRIPTOR_HANDLE{},
						queue_id
					};

					return Allocation(handle, GC(static_cast<Base*>(this)->GetID()));
				}
			}

			// If we reach here, no descriptor was ever free – should not happen unless N is exhausted.
			throw std::runtime_error("D3D12DescriptorAllocator: Allocation failed unexpectedly");
		}

		// Allocate an array of Count descriptors in a batch.
		// Each descriptor is individually allocated (batch is not contiguous in the heap).
		template <std::size_t Count>
		std::array<Allocation, Count> AllocateBatch() {
			static_assert(Count <= QueueSize, "Batch size cannot exceed queue size");

			std::array<Allocation, Count> allocations;

			for (std::size_t i = 0; i < Count; ++i) {
				allocations[i] = Allocate();
			}

			return allocations;
		}

		// Statistics for each internal free queue.
		struct QueueStats {
			std::size_t free_count;   // Number of free descriptors currently in the queue
			std::size_t capacity;	  // Maximum capacity of the queue (QueueSize)
		};

		// Retrieve current free counts for all queues (useful for debugging/telemetry).
		std::array<QueueStats, NumQueues> GetQueueStats() const {
			std::array<QueueStats, NumQueues> stats;

			for (std::size_t i = 0; i < NumQueues; ++i) {
				stats[i] = QueueStats{
					.free_count = m_free_queues[i].size(),
					.capacity = m_free_queues[i].capacity()
				};
			}

			return stats;
		}

		// Return a ComPtr to the underlying ID3D12DescriptorHeap.
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetNative() const noexcept {
			return m_impl.Heap();
		}

		// Return a raw pointer to the underlying heap.
		ID3D12DescriptorHeap* GetRawNative() const noexcept {
			return m_impl.Heap();
		}

		// Total number of descriptors this allocator manages.
		static constexpr std::size_t GetTotalDescriptorCount() noexcept {
			return TotalDescriptors;
		}

		// Number of internal free queues.
		static constexpr std::size_t GetQueueCount() noexcept {
			return NumQueues;
		}

		// Capacity of each queue (the maximum number of descriptors that can be cached per queue).
		static constexpr std::size_t GetQueueSize() noexcept {
			return QueueSize;
		}

	};

	// Predefined allocator types for common descriptor heap kinds.
	export using D3D12UniversalViewAllocator = D3D12DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536u>;
	export using D3D12RTVAllocator = D3D12DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2048u>;
	export using D3D12DSVAllocator = D3D12DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2048u>;
	export using D3D12SamplerAllocator = D3D12DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048u>;

}

#endif // defined(_WIN32)