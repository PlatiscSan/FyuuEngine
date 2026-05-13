module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <atomic>
#include <optional>
#endif
#if defined(_WIN32)
#include <DescriptorHeap.h>
#endif
#include <concurrentqueue/moodycamel/concurrentqueue.h>
export module fyuu_rhi:d3d12_descriptor_allocator;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif

namespace {
	struct Heap {
		DirectX::DescriptorHeap impl;
		moodycamel::ConcurrentQueue<std::size_t> free_indices;

		Heap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t total_descriptors, bool shader_visible)
			: impl(device, type, shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, total_descriptors),
			free_indices() {
			for (std::size_t i = 0; i < total_descriptors; ++i) {
				free_indices.enqueue(i);
			}
		}
	};
}

namespace fyuu_rhi::d3d12 {

	export class D3D12DescriptorHandle final {
	private:
		std::shared_ptr<Heap> m_heap;
		std::size_t m_idx;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu;

	public:
		D3D12DescriptorHandle(std::shared_ptr<Heap> const& heap, std::size_t idx, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) noexcept
			: m_heap(heap), m_idx(idx), m_cpu(cpu), m_gpu(gpu) {
		}

		D3D12DescriptorHandle(std::shared_ptr<Heap> const& heap, std::size_t idx, D3D12_CPU_DESCRIPTOR_HANDLE cpu) noexcept
			: m_heap(heap), m_idx(idx), m_cpu(cpu), m_gpu() {
		}

		~D3D12DescriptorHandle() {
			if (m_heap) {
				m_heap->free_indices.enqueue(m_idx);
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE CPU() const noexcept {
			return m_cpu;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GPU() const noexcept {
			return m_gpu;
		}
	};

	export class D3D12DescriptorAllocator final {
	private:
		std::shared_ptr<Heap> m_heap;
		std::size_t m_total_descriptors;

	public:
		D3D12DescriptorAllocator(
			ID3D12Device* device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			std::size_t total_descriptors,
			bool shader_visible
		) : m_heap(std::make_shared<Heap>(device, type, total_descriptors, shader_visible)),
			m_total_descriptors(total_descriptors) {
		}

		D3D12DescriptorAllocator(D3D12DescriptorAllocator const& other) noexcept = default;
		D3D12DescriptorAllocator(D3D12DescriptorAllocator&& other) noexcept = default;

		std::shared_ptr<D3D12DescriptorHandle> TryAllocate() {
			std::size_t index;
			if (m_heap->free_indices.try_dequeue(index)) {
				D3D12_CPU_DESCRIPTOR_HANDLE cpu = m_heap->impl.GetCpuHandle(index);
				D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
				if (m_heap->impl.Flags() & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
					return std::make_shared<D3D12DescriptorHandle>(m_heap, index, cpu, m_heap->impl.GetGpuHandle(index));
				}
				return std::make_shared<D3D12DescriptorHandle>(m_heap, index, cpu);
			}
			return nullptr;
		}

		std::shared_ptr<D3D12DescriptorHandle> Allocate() {
			constexpr std::size_t max_spins = 10000u;
			for (std::size_t spin = 0u; spin < max_spins; ++spin) {
				auto handle = TryAllocate();
				if (handle) {
					return handle;
				}
				std::this_thread::yield();
			}
			throw std::runtime_error("D3D12DescriptorAllocator::Allocate() failed, no free descriptors");
		}

		template <std::size_t Count>
		std::array<std::shared_ptr<D3D12DescriptorHandle>, Count> AllocateBatch() {
			std::array<std::shared_ptr<D3D12DescriptorHandle>, Count> allocations;
			for (std::size_t i = 0; i < Count; ++i) {
				allocations[i] = Allocate();
			}
			return allocations;
		}

		struct Stats {
			std::size_t free_count_approx;
			std::size_t allocated_count;
			std::size_t total_descriptors;
		};

		Stats GetStats() const {
			std::size_t free_approx = m_heap->free_indices.size_approx();
			return Stats{
				.free_count_approx = free_approx,
				.allocated_count = m_total_descriptors - free_approx,
				.total_descriptors = m_total_descriptors
			};
		}

		ID3D12DescriptorHeap* GetNative() const {
			return m_heap->impl.Heap();
		}

		std::size_t GetTotalDescriptorCount() const {
			return m_total_descriptors;
		}
	};

	export inline D3D12DescriptorAllocator CreateUniversalViewAllocator(
		ID3D12Device* device,
		std::size_t total_descriptors = 65536u
	) {
		return D3D12DescriptorAllocator(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			total_descriptors,
			true
		);
	}

	export inline D3D12DescriptorAllocator CreateRTVAllocator(
		ID3D12Device* device,
		std::size_t total_descriptors = 2048u
	)  {
		return D3D12DescriptorAllocator(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			total_descriptors,
			false
		);
	}

	export inline D3D12DescriptorAllocator CreateDSVAllocator(
		ID3D12Device* device,
		std::size_t total_descriptors = 2048u
	) {
		return D3D12DescriptorAllocator(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			total_descriptors,
			false
		);
	}
	
	export inline D3D12DescriptorAllocator CreateSamplerAllocator(
		ID3D12Device* device,
		std::size_t total_descriptors = 2048u
	) {
		return D3D12DescriptorAllocator(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			total_descriptors,
			true
		);
	}

} // namespace fyuu_rhi::d3d12

#endif // _WIN32