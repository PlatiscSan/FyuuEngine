export module fyuu_rhi:vulkan_resource;
import std;
import plastic.any_pointer;
import plastic.registrable;
import vulkan_hpp;
import :resource;
import :vulkan_memory;

namespace fyuu_rhi::vulkan {

	export class VulkanResource
		: public plastic::utility::Registrable<VulkanResource>,
		public IResource {
		friend class IResource;
	public:
		class MappedHandleGC;
		struct MappedHandle {
			void* ptr;
			std::uintptr_t begin;
			std::uintptr_t end;
		};

		using UniqueMappedHandle = plastic::utility::UniqueResource<MappedHandle, MappedHandleGC>;

	private:
		std::size_t m_width;
		std::size_t m_height;
		std::size_t m_depth;
		ResourceType m_type;
		std::variant<std::monostate, UniqueVulkanBufferHandle, UniqueVulkanTextureHandle> m_handle;

		void ReleaseImpl() noexcept;

		void SetMemoryHandleImpl(UniqueVulkanBufferHandle&& handle);
		void SetMemoryHandleImpl(UniqueVulkanTextureHandle&& handle);

		UniqueMappedHandle MapImpl(std::uintptr_t begin, std::uintptr_t end);

		void SetDeviceLocalBuffer(
			VulkanLogicalDevice& logical_device,
			VulkanCommandQueue& copy_queue,
			std::span<std::byte const> raw,
			std::size_t offset
		);

		void SetHostVisibleBuffer(
			std::span<std::byte const> raw,
			std::size_t offset
		);

		void SetBufferDataImpl(
			VulkanLogicalDevice& logical_device,
			VulkanCommandQueue& copy_queue,
			std::span<std::byte const> raw,
			std::size_t offset = 0
		);

	public:
		static VulkanResource CreateVertexBuffer(std::size_t size_in_bytes);
		static VulkanResource CreateIndexBuffer(std::size_t size_in_bytes);

		VulkanResource(std::size_t width, std::size_t height, std::size_t depth, ResourceType type);
		VulkanResource(plastic::utility::AnyPointer<VulkanVideoMemory> const& memory, std::size_t width, std::size_t height, std::size_t depth, ResourceType type);
		VulkanResource(VulkanVideoMemory& memory, std::size_t width, std::size_t height, std::size_t depth, ResourceType type);
		VulkanResource(VulkanResource&& other) noexcept;

		~VulkanResource() noexcept;

		vk::Buffer GetBufferNative() const;

	};

	class VulkanResource::MappedHandleGC {
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