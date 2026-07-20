module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#endif
#include <boost/lockfree/policies.hpp>
#include <boost/lockfree/queue.hpp>
#if defined(_WIN32)
#include <DescriptorHeap.h>
#endif // defined(_WIN32)
export module fyuu_rhi:d3d12_descriptor_allocator;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif

namespace {

	class DescriptorHeap : public DirectX::DescriptorHeap {
	private:
		boost::lockfree::queue<std::size_t> m_free_indices;

	public:
		DescriptorHeap(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t total_desc, bool shader_visible)
			: DirectX::DescriptorHeap(dev, type, shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, total_desc),
			m_free_indices(total_desc) {
			for (std::size_t i = 0; i < total_desc; ++i) {
				m_free_indices.push(i);
			}
		}

		std::size_t Acquire() {
			std::size_t index;
			std::size_t spin_count = 0;
			while (!m_free_indices.pop(index)) {
				if (++spin_count > 100u) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					spin_count = 0;
				}
				else {
					std::this_thread::yield();
				}
			}
			return index;
		}

		void Release(std::size_t index) {
			m_free_indices.push(index);
		}
	};

}

namespace fyuu_rhi::d3d12 {

	export class ManagedDescriptorHandle final {
	private:
		std::shared_ptr<DescriptorHeap> m_heap;
		std::optional<std::size_t> m_idx;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu;

	public:
		ManagedDescriptorHandle(std::shared_ptr<DescriptorHeap> const& heap, std::size_t idx, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) noexcept
			: m_heap(heap), m_idx(idx), m_cpu(cpu), m_gpu(gpu) {
		}

		ManagedDescriptorHandle(std::shared_ptr<DescriptorHeap> const& heap, std::size_t idx, D3D12_CPU_DESCRIPTOR_HANDLE cpu) noexcept
			: m_heap(heap), m_idx(idx), m_cpu(cpu), m_gpu() {
		}

		ManagedDescriptorHandle(ManagedDescriptorHandle const&) = delete;
		ManagedDescriptorHandle& operator=(ManagedDescriptorHandle const&) = delete;

		ManagedDescriptorHandle(ManagedDescriptorHandle&& other) noexcept
			: m_heap(std::move(other.m_heap)),
			m_idx(std::move(other.m_idx)),
			m_cpu(std::exchange(other.m_cpu, {})),
			m_gpu(std::exchange(other.m_gpu, {})) {
			m_idx.reset();
		}

		ManagedDescriptorHandle& operator=(ManagedDescriptorHandle&& other) noexcept {
			if (this != &other) {
				if (m_heap && m_idx) {
					m_heap->Release(*m_idx);
				}
				m_heap = std::move(other.m_heap);
				m_idx = std::move(other.m_idx);
				m_idx.reset();
				m_cpu = std::exchange(other.m_cpu, {});
				m_gpu = std::exchange(other.m_gpu, {});
			}
			return *this;
		}

		~ManagedDescriptorHandle() {
			if (m_heap && m_idx) {
				m_heap->Release(*m_idx);
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE CPU() const noexcept {
			return m_cpu;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GPU() const noexcept {
			return m_gpu;
		}
	};

	export class DescriptorAllocator final {
	private:
		std::shared_ptr<DescriptorHeap> m_heap;

	public:
		DescriptorAllocator(
			ID3D12Device* dev,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			std::size_t total_desc,
			bool shader_visible
		) : m_heap(std::make_shared<DescriptorHeap>(dev, type, total_desc, shader_visible)) {
		}

		DescriptorAllocator(DescriptorAllocator const& other) noexcept = default;
		DescriptorAllocator(DescriptorAllocator&& other) noexcept = default;

		std::shared_ptr<ManagedDescriptorHandle> Allocate() {
			std::size_t idx = m_heap->Acquire();
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = m_heap->GetCpuHandle(idx);
			if (m_heap->Flags() & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
				D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = m_heap->GetGpuHandle(idx);
				return std::make_shared<ManagedDescriptorHandle>(m_heap, idx, cpu_handle, gpu_handle);
			}
			else {
				return std::make_shared<ManagedDescriptorHandle>(m_heap, idx, cpu_handle);
			}
		}

		template <std::size_t Count>
		std::array<std::shared_ptr<ManagedDescriptorHandle>, Count> AllocateBatch() {
			std::array<std::shared_ptr<ManagedDescriptorHandle>, Count> allocations;
			for (std::size_t i = 0; i < Count; ++i) {
				allocations[i] = Allocate();
			}
			return allocations;
		}


		ID3D12DescriptorHeap* GetNative() const {
			return m_heap->Heap();
		}

	};

	export inline DescriptorAllocator CreateUniversalViewAllocator(
		ID3D12Device* dev,
		std::size_t total_desc = 65536u
	) {
		return DescriptorAllocator(
			dev,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			total_desc,
			true
		);
	}

	export inline DescriptorAllocator CreateRTVAllocator(
		ID3D12Device* dev,
		std::size_t total_desc = 2048u
	)  {
		return DescriptorAllocator(
			dev,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			total_desc,
			false
		);
	}

	export inline DescriptorAllocator CreateDSVAllocator(
		ID3D12Device* dev,
		std::size_t total_desc = 2048u
	) {
		return DescriptorAllocator(
			dev,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			total_desc,
			false
		);
	}
	
	export inline DescriptorAllocator CreateSamplerAllocator(
		ID3D12Device* dev,
		std::size_t total_desc = 2048u
	) {
		return DescriptorAllocator(
			dev,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			total_desc,
			true
		);
	}

} // namespace fyuu_rhi::d3d12

#endif // _WIN32