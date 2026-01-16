module;
#include <D3D12MemAlloc.h>
#include <wrl.h>
export module fyuu_rhi:d3d12_memory;
import std;
import plastic.any_pointer;
import plastic.other;
import plastic.registrable;
import plastic.unique_resource;
import :memory;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	export struct D3D12ResourceHandle {
		Microsoft::WRL::ComPtr<ID3D12Resource2> impl;
		D3D12_RESOURCE_STATES state;
	};

	export class D3D12VideoMemory
		: public plastic::utility::Registrable<D3D12VideoMemory>,
		public plastic::utility::EnableSharedFromThis<D3D12VideoMemory>,
		public IVideoMemory {
		friend class IVideoMemory;

	public:
		class HandleGC;

		using UniqueResourceHandle = plastic::utility::UniqueResource<D3D12ResourceHandle, HandleGC>;

	private:
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> m_allocation;
		std::size_t m_device_id;
		std::atomic_flag m_allocated_flag;
		VideoMemoryUsage m_usage;
		VideoMemoryType m_type;

		UniqueResourceHandle CreateBufferHandleImpl(std::size_t size_in_bytes);

		UniqueResourceHandle CreateTextureHandleImpl(std::size_t width, std::size_t height, std::size_t depth);

		VideoMemoryUsage GetUsageImpl() const noexcept;

		VideoMemoryType GetTypeImpl() const noexcept;

	public:	
		D3D12VideoMemory(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device, 
			std::size_t size_in_bytes,
			VideoMemoryUsage usage,
			VideoMemoryType type
		);

		D3D12VideoMemory(
			D3D12LogicalDevice const& logical_device,
			std::size_t size_in_bytes,
			VideoMemoryUsage usage,
			VideoMemoryType type
		);

		~D3D12VideoMemory() noexcept;
	};

	class D3D12VideoMemory::HandleGC {
	private:
		std::size_t m_video_memory_id;

	public:
		HandleGC(std::size_t video_memory_id) noexcept;
		HandleGC(HandleGC const& other) noexcept;
		HandleGC(HandleGC&& other) noexcept;

		HandleGC& operator=(HandleGC const& other) noexcept;
		HandleGC& operator=(HandleGC&& other) noexcept;

		void operator()(D3D12ResourceHandle& resource);

		std::size_t GetVideoMemoryID() const noexcept;

	};

	export using UniqueD3D12ResourceHandle = D3D12VideoMemory::UniqueResourceHandle;

}