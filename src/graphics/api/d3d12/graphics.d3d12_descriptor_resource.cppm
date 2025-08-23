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

#ifdef interface
#undef interface
#endif // interface

export module graphics:d3d12_descriptor_resource;
export import window;
export import :d3d12_command_object;
export import collective_resource;
export import disable_copy;
import std;

namespace graphics::api::d3d12 {
#ifdef WIN32

	export class DescriptorResourcePool : util::DisableCopy<DescriptorResourcePool> {
	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;

		/*
		*	TODO: use lock-free structure
		*/

		std::deque<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>> m_descriptor_handles;
		std::mutex m_descriptor_handles_mutex;
		std::condition_variable m_condition;
		std::uint32_t m_count;

		static std::unique_lock<std::mutex> WaitForAllocated(DescriptorResourcePool* pool);

	public:
		class Collector {
		private:
			DescriptorResourcePool* m_owner;

		public:
			Collector(DescriptorResourcePool* owner) noexcept;
			void operator()(std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> handle);
		};

		using UniqueDescriptorHandle
			= util::UniqueCollectiveResource<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>, Collector>;

		DescriptorResourcePool(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			std::uint32_t count
		);

		DescriptorResourcePool(DescriptorResourcePool&& other) noexcept;
		DescriptorResourcePool& operator=(DescriptorResourcePool && other) noexcept;

		~DescriptorResourcePool() noexcept;

		D3D12_DESCRIPTOR_HEAP_TYPE PoolType() const noexcept;

		UniqueDescriptorHandle AcquireHandle();

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const noexcept;

	};

#endif // WIN32
}
