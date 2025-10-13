export module vulkan_backend:buffer_pool;
import vulkan_hpp;
import std;
export import rendering;
import concurrent_vector;
import concurrent_hash_map;

namespace fyuu_engine::vulkan {

	export class VulkanBufferAllocation {
	private:
		vk::UniqueBuffer m_buffer;
		std::uintptr_t m_offset;
		std::size_t m_size;

		/// @brief pointer to VulkanBufferPool::MemoryBlock
		void* m_tag;
		
	public:
		VulkanBufferAllocation(
			vk::UniqueBuffer&& buffer, 
			std::uintptr_t offset,
			std::size_t size, 
			void* tag
		);
		void* GetTag() const noexcept;
		std::uintptr_t GetOffset() const noexcept;
		std::size_t GetSize() const noexcept;
		vk::Buffer GetBuffer() const noexcept;
		void Update(std::span<std::byte> data);
		void Reset();

	};

	export class VulkanBufferPool {
		friend class VulkanBufferAllocation;
	private:
		struct MemoryBlock {
			vk::Device logical_device;
			vk::UniqueDeviceMemory memory;
			std::uintptr_t mapped_start;
			/// @brief ranges for [start, end]
			std::vector<std::pair<std::uintptr_t, std::uintptr_t>> free_ranges;
			vk::MemoryPropertyFlags memory_properties;

			MemoryBlock(
				vk::Device const& logical_device_,
				vk::UniqueDeviceMemory&& memory_,
				std::uintptr_t mapped_start_,
				std::vector<std::pair<std::uintptr_t, std::uintptr_t>>&& free_ranges_,
				vk::MemoryPropertyFlags memory_properties_
			);

			MemoryBlock(
				vk::Device const& logical_device_,
				vk::UniqueDeviceMemory&& memory_,
				std::vector<std::pair<std::uintptr_t, std::uintptr_t>>&& free_ranges_,
				vk::MemoryPropertyFlags memory_properties_
			);
			MemoryBlock(MemoryBlock&& other) noexcept;
			MemoryBlock& operator=(MemoryBlock&& other) noexcept;
			~MemoryBlock() noexcept;
		};

		vk::PhysicalDevice m_physical_device;
		vk::Device m_logical_device;
		vk::BufferUsageFlags m_usage;
		vk::MemoryPropertyFlags m_memory_properties;
		std::size_t m_block_size;
		std::size_t m_min_allocation;
		vk::AllocationCallbacks const* m_allocator;
		
		concurrency::ConcurrentVector<MemoryBlock> m_blocks;

		std::optional<std::uint32_t> FindMemoryType(std::uint32_t type_filter, vk::MemoryPropertyFlags properties) const;
		MemoryBlock CreateBlock(std::size_t block_size) const;
		std::optional<VulkanBufferAllocation> AllocateFromBlock(MemoryBlock& block, std::size_t size, std::size_t alignment) const;
		void MergeFreeRange(MemoryBlock& block) const;

	public:
		VulkanBufferPool() = default;
		VulkanBufferPool(
			vk::PhysicalDevice const& physical_device,
			vk::Device const& logical_device, 
			vk::BufferUsageFlags usage, 
			vk::MemoryPropertyFlags memory_properties,
			std::size_t block_size,
			std::size_t min_allocation,
			vk::AllocationCallbacks const* allocator = nullptr
		);

		std::optional<VulkanBufferAllocation> Allocate(std::size_t size, std::size_t alignment = 1);
		void Free(VulkanBufferAllocation& allocation);
		void Free(std::optional<VulkanBufferAllocation>& allocation);

		std::size_t GetBlockSize() const noexcept;
	};

}
