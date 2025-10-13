module vulkan_backend:buffer_pool;

namespace fyuu_engine::vulkan {

	VulkanBufferAllocation::VulkanBufferAllocation(
		vk::UniqueBuffer&& buffer,
		std::uintptr_t offset,
		std::size_t size,
		void* tag
	) : m_buffer(std::move(buffer)),
		m_offset(offset),
		m_size(size),
		m_tag(tag) {
	}

	void* VulkanBufferAllocation::GetTag() const noexcept {
		return m_tag;
	}

	std::uintptr_t VulkanBufferAllocation::GetOffset() const noexcept {
		return m_offset;;
	}

	std::size_t VulkanBufferAllocation::GetSize() const noexcept {
		return m_size;
	}

	vk::Buffer VulkanBufferAllocation::GetBuffer() const noexcept {
		return *m_buffer;
	}

	void VulkanBufferAllocation::Update(std::span<std::byte> data) {

		auto memory_block = static_cast<VulkanBufferPool::MemoryBlock*>(m_tag);

		bool mapped =
			(memory_block->memory_properties & vk::MemoryPropertyFlagBits::eHostVisible) &&
			(memory_block->memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent);

		if (!mapped) {
			throw std::runtime_error("Not host visible or host coherent buffer");
		}

		std::memcpy(reinterpret_cast<void*>(m_offset), data.data(), data.size());

	}

	void VulkanBufferAllocation::Reset() {
		m_buffer.reset();
	}

	VulkanBufferPool::MemoryBlock::MemoryBlock(
		vk::Device const& logical_device_, 
		vk::UniqueDeviceMemory&& memory_, 
		std::uintptr_t mapped_start_,
		std::vector<std::pair<std::uintptr_t, std::uintptr_t>>&& free_ranges_, 
		vk::MemoryPropertyFlags memory_properties_
	) : logical_device(logical_device_),
		memory(std::move(memory_)),
		mapped_start(mapped_start_),
		free_ranges(std::move(free_ranges_)),
		memory_properties(memory_properties_) {

	}

	VulkanBufferPool::MemoryBlock::MemoryBlock(
		vk::Device const& logical_device_, 
		vk::UniqueDeviceMemory&& memory_, 
		std::vector<std::pair<std::uintptr_t, std::uintptr_t>>&& free_ranges_, 
		vk::MemoryPropertyFlags memory_properties_
	) : logical_device(logical_device_),
		memory(std::move(memory_)),
		mapped_start(0),
		free_ranges(std::move(free_ranges_)),
		memory_properties(memory_properties_) {
	}

	VulkanBufferPool::MemoryBlock::MemoryBlock(MemoryBlock&& other) noexcept
		: logical_device(std::exchange(other.logical_device, vk::Device{})),
		memory(std::move(other.memory)),
		mapped_start(std::exchange(other.mapped_start, 0u)),
		free_ranges(std::move(other.free_ranges)),
		memory_properties(std::exchange(other.memory_properties, vk::MemoryPropertyFlags{})) {
	}

	VulkanBufferPool::MemoryBlock& VulkanBufferPool::MemoryBlock::operator=(MemoryBlock&& other) noexcept {
		if (this != &other) {
			logical_device = std::exchange(other.logical_device, vk::Device{});
			memory = std::move(other.memory);
			mapped_start = std::exchange(other.mapped_start, 0u);
			free_ranges = std::move(other.free_ranges);
			memory_properties = std::exchange(other.memory_properties, vk::MemoryPropertyFlags{});
		}
		return *this;
	}

	VulkanBufferPool::MemoryBlock::~MemoryBlock() noexcept {
		free_ranges.clear();
		bool is_mapped =
			(memory_properties & vk::MemoryPropertyFlagBits::eHostVisible) ||
			(memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent);
		if (logical_device && memory && is_mapped) {
			logical_device.unmapMemory(*memory);
		}
	}

	std::optional<std::uint32_t> VulkanBufferPool::FindMemoryType(
		std::uint32_t type_filter, 
		vk::MemoryPropertyFlags properties
	) const {

		vk::PhysicalDeviceMemoryProperties mem_properties;
		m_physical_device.getMemoryProperties(&mem_properties);

		/*vk::PhysicalDeviceMemoryProperties mem_properties = m_physical_device.getMemoryProperties();*/

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
			if ((type_filter & (1 << i)) &&
				(mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return std::nullopt;
	}

	VulkanBufferPool::MemoryBlock VulkanBufferPool::CreateBlock(std::size_t block_size) const {

		vk::BufferCreateInfo buffer_info({}, block_size, m_usage, vk::SharingMode::eExclusive);
		vk::UniqueBuffer dummy = m_logical_device.createBufferUnique(buffer_info);

		vk::MemoryRequirements mem_requirements = m_logical_device.getBufferMemoryRequirements(*dummy);
		std::optional<std::uint32_t> memory_type = FindMemoryType(mem_requirements.memoryTypeBits, m_memory_properties);

		vk::MemoryAllocateInfo alloc_info(
			mem_requirements.size, 
			memory_type.value()
		);

		vk::UniqueDeviceMemory memory = m_logical_device.allocateMemoryUnique(alloc_info, m_allocator);

		bool can_map =
			(m_memory_properties & vk::MemoryPropertyFlagBits::eHostVisible) ||
			(m_memory_properties & vk::MemoryPropertyFlagBits::eHostCoherent);

		//void* mapped = can_map ?
		//	m_logical_device.mapMemory(*memory, 0, mem_requirements.size, vk::MemoryMapFlagBits::ePlacedEXT) :
		//	nullptr;

		void* mapped = can_map ?
			m_logical_device.mapMemory(*memory, 0, mem_requirements.size) :
			nullptr;

		std::uintptr_t start = reinterpret_cast<std::uintptr_t>(mapped);
		std::uintptr_t end = start + mem_requirements.size;

		std::vector<std::pair<std::uintptr_t, std::uintptr_t>> free_ranges;
		free_ranges.emplace_back(start, end);

		return can_map ? 
			MemoryBlock(m_logical_device, std::move(memory), start, std::move(free_ranges), m_memory_properties) :
			MemoryBlock(m_logical_device, std::move(memory), std::move(free_ranges), m_memory_properties);
	}

	std::optional<VulkanBufferAllocation> VulkanBufferPool::AllocateFromBlock(
		MemoryBlock& block, 
		std::size_t size, 
		std::size_t alignment
	) const {

		auto& free_ranges = block.free_ranges;

		if (free_ranges.empty()) {
			return std::nullopt;
		}

		/*
		*	best fit algorithm
		*/

		auto best_fit_iter = free_ranges.end();
		std::size_t best_fit_waste = std::numeric_limits<std::size_t>::max();

		for (auto iter = free_ranges.begin(); iter != free_ranges.end(); ++iter) {

			auto [start, end] = *iter;
			std::size_t range_size = end - start;

			std::size_t aligned_offset = (start + alignment - 1) & ~(alignment - 1);
			std::size_t padding = aligned_offset == 0 ?
				0 :
				aligned_offset - start;
			std::size_t required_size = padding + size;

			if (range_size >= required_size) {

				std::size_t waste = range_size - required_size;

				if (waste < best_fit_waste) {
					best_fit_waste = waste;
					best_fit_iter = iter;
				}
			}
		}

		if (best_fit_iter == free_ranges.end()) {
			return std::nullopt;
		}

		std::pair<std::size_t, std::size_t> best_range = std::move(*best_fit_iter);
		free_ranges.erase(best_fit_iter);

		auto [start, end] = best_range;
		std::size_t range_size = end - start;


		std::size_t aligned_offset = (start + alignment - 1) & ~(alignment - 1);
		std::size_t padding = aligned_offset == 0 ?
			0 :
			aligned_offset - start;
		std::size_t remaining_size = range_size - padding - size;


		if (padding > 0) {
			/*
			*	push back as free block if front padding exists
			*/

			std::pair<std::size_t, std::size_t> front_chunk;
			front_chunk.first = start;
			front_chunk.second = start + padding;
			free_ranges.emplace_back(std::move(front_chunk));
		}

		if (remaining_size > 0) {
			/*
			*	push as free block if there is remaining
			*/


			std::pair<std::size_t, std::size_t> back_chunk;
			back_chunk.first = block.mapped_start + aligned_offset + size;
			back_chunk.second = back_chunk.first + remaining_size;
			free_ranges.emplace_back(std::move(back_chunk));
		}

		vk::BufferCreateInfo buffer_creation_info(
			{}, 
			size,
			m_usage,
			vk::SharingMode::eExclusive
		);

		std::size_t logical_start = start - block.mapped_start;

		vk::UniqueBuffer buffer = m_logical_device.createBufferUnique(buffer_creation_info, m_allocator);
		m_logical_device.bindBufferMemory(*buffer, *block.memory, logical_start);

		void* tag = &block;

		return std::make_optional<VulkanBufferAllocation>(std::move(buffer), start, range_size, tag);

	}

	void VulkanBufferPool::MergeFreeRange(MemoryBlock& block) const {

		/*
		*	sort block by offset
		*/

		auto& free_ranges = block.free_ranges;

		std::sort(
			free_ranges.begin(),
			free_ranges.end(),
			[](std::pair<std::size_t, std::size_t> const& a, std::pair<std::size_t, std::size_t> const& b) {
				return a.first < b.first;
			}
		);


		/*
		*	merge adjacent free ranges
		*/

		auto iter = free_ranges.begin();
		while (iter != free_ranges.end()) {

			auto next_iter = std::next(iter);
			if (next_iter != free_ranges.end() &&
				iter->second + 1 == next_iter->first) {

				/*
				*	merge chunk
				*/

				iter->second += next_iter->second;
				free_ranges.erase(next_iter);

				/*
				*	do not advance, continue determining if more chunks can be merged
				*/

			}
			else {
				++iter;
			}

		}


	}

	VulkanBufferPool::VulkanBufferPool(
		vk::PhysicalDevice const& physical_device,
		vk::Device const& logical_device,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags memory_properties,
		std::size_t block_size,
		std::size_t min_allocation,
		vk::AllocationCallbacks const* allocator
	) : m_physical_device(physical_device),
		m_logical_device(logical_device),
		m_usage(usage),
		m_memory_properties(memory_properties),
		m_block_size(block_size),
		m_min_allocation(min_allocation),
		m_allocator(allocator) {
	}

	std::optional<VulkanBufferAllocation> VulkanBufferPool::Allocate(std::size_t size, std::size_t alignment) {

		alignment = alignment == 0u ?
			alignment :
			1u;


		std::size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
		aligned_size = aligned_size < m_min_allocation ?
			m_min_allocation :
			aligned_size;

		{
			auto locked_modifier = m_blocks.LockedModifier();
			for (auto& block : locked_modifier) {
				if (std::optional<VulkanBufferAllocation> buffer = AllocateFromBlock(block, size, alignment)) {
					return buffer;
				}
			}
		}

		/*
		*	No suitable block, create new block
		*/

		std::size_t block_size = (std::max)(m_block_size, aligned_size * 2);
		MemoryBlock& block = m_blocks.emplace_back(CreateBlock(block_size));

		return AllocateFromBlock(block, size, alignment);

	}

	void VulkanBufferPool::Free(VulkanBufferAllocation& allocation) {

		if (!allocation.GetBuffer()) {
			return;
		}

		auto locked_modifier = m_blocks.LockedModifier();
		for (auto& block : locked_modifier) {
			if (allocation.GetTag() == &block) {
				std::size_t start = allocation.GetOffset();
				std::size_t end = allocation.GetOffset() + allocation.GetSize() - 1;
				block.free_ranges.emplace_back(start, end);

				MergeFreeRange(block);
				allocation.Reset();
				return;
			}
		}

	}

	void VulkanBufferPool::Free(std::optional<VulkanBufferAllocation>& allocation) {
		
		if (!allocation) {
			return;
		}

		Free(*allocation);

	}

	std::size_t VulkanBufferPool::GetBlockSize() const noexcept {
		return m_block_size;
	}

}