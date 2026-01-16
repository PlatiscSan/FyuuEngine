module;
#include <d3d12.h>
#include <wrl.h>
export module fyuu_rhi:d3d12_resource;
import std;
import plastic.any_pointer;
import plastic.registrable;
import :resource;
import :d3d12_memory;

namespace fyuu_rhi::d3d12 {

	export class D3D12Resource
		: public plastic::utility::Registrable<D3D12Resource>,
		public IResource {
		friend class IResource;

	public:
		class MappedHandleGC;
		struct MappedHandle {
			D3D12_RANGE range;
			void* ptr;
		};

		using UniqueMappedHandle = plastic::utility::UniqueResource<MappedHandle, MappedHandleGC>;

	private:
		std::size_t m_width;
		std::size_t m_height;
		std::size_t m_depth;
		ResourceType m_type;
		UniqueD3D12ResourceHandle m_handle;

		void ReleaseImpl() noexcept;

		void SetMemoryHandleImpl(UniqueD3D12ResourceHandle&& handle);

		UniqueMappedHandle MapImpl(std::uintptr_t begin, std::uintptr_t end);

		void SetDeviceLocalBuffer(
			D3D12LogicalDevice& logical_device,
			D3D12CommandQueue& copy_queue,
			std::span<std::byte const> raw,
			std::size_t offset
		);

		void SetHostVisibleBuffer(
			std::span<std::byte const> raw,
			std::size_t offset
		);

		void SetBufferDataImpl(
			D3D12LogicalDevice& logical_device,
			D3D12CommandQueue& copy_queue,
			std::span<std::byte const> raw,
			std::size_t offset = 0
		);

	public:
		static D3D12Resource CreateVertexBuffer(std::size_t size_in_bytes);
		static D3D12Resource CreateIndexBuffer(std::size_t size_in_bytes);

		D3D12Resource(std::size_t width, std::size_t height, std::size_t depth, ResourceType type);
		D3D12Resource(plastic::utility::AnyPointer<D3D12VideoMemory> const& memory, std::size_t width, std::size_t height, std::size_t depth, ResourceType type);
		D3D12Resource(D3D12VideoMemory& memory, std::size_t width, std::size_t height, std::size_t depth, ResourceType type);

		~D3D12Resource() noexcept;

	};

	class D3D12Resource::MappedHandleGC {
	private:
		std::size_t m_resource_id;

	public:
		MappedHandleGC(std::size_t resource_id) noexcept;
		MappedHandleGC(MappedHandleGC const& other) noexcept;
		MappedHandleGC(MappedHandleGC&& other) noexcept;

		MappedHandleGC& operator=(MappedHandleGC const& other) noexcept;
		MappedHandleGC& operator=(MappedHandleGC&& other) noexcept;

		void operator()(MappedHandle& handle);

		std::size_t GetResourceID() const noexcept;
	};

}