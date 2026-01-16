module;
//#include <vma/vk_mem_alloc.h>
export module fyuu_rhi:vulkan_memory;
import std;
import plastic.any_pointer;
import plastic.unique_resource;
import plastic.registrable;
import vulkan_hpp;
import <vma/vk_mem_alloc.h>;
import :memory;
import :vulkan_declaration;

namespace fyuu_rhi::vulkan {

	namespace details {

		struct VMAAllocationGC {
			VmaAllocator allocator = nullptr;
			void operator()(VmaAllocation allocation);

			VMAAllocationGC() = default;
			VMAAllocationGC(VmaAllocator allocator);
			VMAAllocationGC(VMAAllocationGC const& other);
			VMAAllocationGC(VMAAllocationGC&& other) noexcept;
		};

		using UniqueVMAAllocation = std::unique_ptr<std::remove_pointer_t<VmaAllocation>, VMAAllocationGC>;

	}

	export class VulkanVideoMemory
		: public plastic::utility::Registrable<VulkanVideoMemory>,
		public plastic::utility::EnableSharedFromThis<VulkanVideoMemory>,
		public IVideoMemory {
		friend class IVideoMemory;
	public:
		class HandleGC;

		using UniqueBufferHandle = plastic::utility::UniqueResource<vk::UniqueBuffer, HandleGC>;
		using UniqueTextureHandle = plastic::utility::UniqueResource<vk::UniqueImage, HandleGC>;

	private:
		details::UniqueVMAAllocation m_allocation;
		std::size_t m_device_id;
		std::atomic_flag m_allocated_flag;
		VideoMemoryUsage m_usage;
		VideoMemoryType m_type;

		UniqueBufferHandle CreateBufferHandleImpl(std::size_t size_in_bytes);

		UniqueTextureHandle CreateTextureHandleImpl(std::size_t width, std::size_t height, std::size_t depth);

		VideoMemoryUsage GetUsageImpl() const noexcept;

		VideoMemoryType GetTypeImpl() const noexcept;

	public:
		VulkanVideoMemory(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			std::size_t size_in_bytes,
			VideoMemoryUsage usage,
			VideoMemoryType type
		);

		VulkanVideoMemory(
			VulkanLogicalDevice const& logical_device,
			std::size_t size_in_bytes,
			VideoMemoryUsage usage,
			VideoMemoryType type
		);

		~VulkanVideoMemory() noexcept;

		VmaAllocator GetAllocator() const noexcept;

		VmaAllocation GetNative() const noexcept;

		std::size_t GetSize() const noexcept;
	};

	class VulkanVideoMemory::HandleGC {
	private:
		std::size_t m_video_memory_id;

		void ResetMemory() const;

	public:
		HandleGC(std::size_t video_memory_id) noexcept;
		HandleGC(HandleGC const& other) noexcept;
		HandleGC(HandleGC && other) noexcept;

		void operator()(vk::UniqueBuffer& buffer) const;
		void operator()(vk::UniqueImage& texture) const;

		std::size_t GetVideoMemoryID() const noexcept;
	};

	export using UniqueVulkanBufferHandle = VulkanVideoMemory::UniqueBufferHandle;
	export using UniqueVulkanTextureHandle = VulkanVideoMemory::UniqueTextureHandle;
}