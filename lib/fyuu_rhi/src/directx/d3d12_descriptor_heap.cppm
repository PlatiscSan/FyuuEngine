module;
#if defined(_WIN32)
#include <DescriptorHeap.h>
#endif // defined(_WIN32)

export module fyuu_rhi:d3d12_descriptor_heap;
import std;
import plastic.circular_buffer;
import plastic.unique_resource;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
#if defined(_WIN32)
	export struct D3D12DescriptorHandle {
		std::size_t index;
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
	};

	template <std::size_t N = 2048u>
	class D3D12DescriptorAllocator {
	private:
		DirectX::DescriptorHeap m_impl;
		plastic::concurrency::CircularBuffer<std::size_t, N> m_free_queue;

	public:
		class GC {
		private:
			D3D12DescriptorAllocator* m_owner;

		public:
			GC(D3D12DescriptorAllocator* owner = nullptr)
				: m_owner(owner) {

			}

			GC(GC const& other) noexcept
				: m_owner(other.m_owner) {

			}

			GC(GC&& other) noexcept
				: m_owner(std::exchange(other.m_owner, nullptr)) {

			}

			void operator()(D3D12DescriptorHandle handle) {
				if (m_owner) {
					m_owner->m_free_queue.emplace_back(handle.index);
				}
			}

		};

		using Allocation = plastic::utility::UniqueResource<D3D12DescriptorHandle, GC>;

		D3D12DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
			: m_impl(device, type, flags, N) {
			for (std::size_t i = 0; i < N; ++i) {
				m_free_queue.emplace_back(i);
			}
		}

		Allocation Allocate() {

			std::optional<std::size_t> allocated_index = m_free_queue.pop_front();
			if (!allocated_index) {
				return {};
			}

			D3D12DescriptorHandle handle{ 
				*allocated_index, 
				m_impl.GetCpuHandle(*allocated_index), 
				m_impl.Flags() & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ? 
					m_impl.GetGpuHandle(*allocated_index) : 
					D3D12_GPU_DESCRIPTOR_HANDLE{}
			};
			return Allocation(handle, this);

		}

	};

	export using CBV_SRV_UAV_Allocator = D3D12DescriptorAllocator<65536u>;
	export using RTVAllocator = D3D12DescriptorAllocator<2048u>;
	export using DSVAllocator = D3D12DescriptorAllocator<2048u>;
	export using SamplerAllocator = D3D12DescriptorAllocator<2048u>;
#endif // defined(_WIN32)

}