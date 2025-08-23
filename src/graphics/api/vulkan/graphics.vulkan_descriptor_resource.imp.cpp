module graphics:vulkan_descriptor_resource;
import std;

static vk::UniqueDescriptorSetLayout CreateSetLayout(
	vk::UniqueDevice const& logical_device,
	vk::DescriptorType type,
	vk::ShaderStageFlags stage_flags,
	vk::AllocationCallbacks const* allocator
) {

	vk::DescriptorSetLayoutBinding layout_binding(
		0, // binding
		type,
		1, // descriptorCount
		stage_flags
	);

	vk::DescriptorSetLayoutCreateInfo layout_info({}, layout_binding);

	return logical_device->createDescriptorSetLayoutUnique(layout_info, allocator);

}

static vk::UniqueDescriptorPool CreateDescriptorPool(
	vk::UniqueDevice const& logical_device, 
	vk::DescriptorType type,
	std::uint32_t count,
	vk::AllocationCallbacks const* allocator
) {

	vk::DescriptorPoolSize pool_size(type, count);

	vk::DescriptorPoolCreateInfo pool_info(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		count, // maxSets
		pool_size
	);

	return logical_device->createDescriptorPoolUnique(pool_info, allocator);

}

static std::vector<vk::DescriptorSet> GetDescriptorSets(
	vk::UniqueDevice const& logical_device,
	vk::UniqueDescriptorSetLayout const& set_layout,
	vk::UniqueDescriptorPool const& descriptor_pool,
	std::uint32_t count
) {
	std::vector<vk::DescriptorSetLayout> layouts(count, *set_layout);
	vk::DescriptorSetAllocateInfo alloc_info(*descriptor_pool, layouts);
	return logical_device->allocateDescriptorSets(alloc_info);
}

namespace graphics::api::vulkan {

	std::unique_lock<std::mutex> DescriptorResourcePool::WaitForAllocated(DescriptorResourcePool* pool) {

		std::unique_lock<std::mutex> lock(pool->m_descriptor_sets_mutex);
		if (pool->m_descriptor_sets.size() != pool->m_count) {
			pool->m_condition.wait(
				lock,
				[pool]() {
					return pool->m_descriptor_sets.size() == pool->m_count;
				}
			);
		}

		return lock;

	}

	DescriptorResourcePool::DescriptorResourcePool(
		vk::UniqueDevice const& logical_device, 
		vk::DescriptorType type, 
		std::uint32_t count, 
		vk::ShaderStageFlags stage_flags,
		vk::AllocationCallbacks const* allocator
	) : m_set_layout(CreateSetLayout(logical_device, type, stage_flags, allocator)),
		m_descriptor_pool(CreateDescriptorPool(logical_device, type, count, allocator)),
		m_descriptor_sets(GetDescriptorSets(logical_device, m_set_layout, m_descriptor_pool, count)),
		m_descriptor_sets_mutex(),
		m_condition(),
		m_count(count) {
	}

	DescriptorResourcePool::DescriptorResourcePool(DescriptorResourcePool&& other) noexcept
		: m_set_layout(std::move(other.m_set_layout)),
		m_descriptor_pool(std::move(other.m_descriptor_pool)),
		m_descriptor_sets(std::move(other.m_descriptor_sets)),
		m_count(std::exchange(other.m_count, 0)) {
	}

	DescriptorResourcePool& DescriptorResourcePool::operator=(DescriptorResourcePool&& other) noexcept {
		if (this != &other) {
			auto lock = DescriptorResourcePool::WaitForAllocated(this);
			auto other_lock = DescriptorResourcePool::WaitForAllocated(&other);

			m_set_layout = std::move(other.m_set_layout);
			m_descriptor_pool = std::move(other.m_descriptor_pool);
			m_descriptor_sets = std::move(other.m_descriptor_sets);
			m_count = std::exchange(other.m_count, 0);
		}
		return *this;
	}

	DescriptorResourcePool::~DescriptorResourcePool() noexcept {
		auto lock = DescriptorResourcePool::WaitForAllocated(this);
		m_descriptor_sets.clear();
	}

	DescriptorResourcePool::UniqueDescriptorSet DescriptorResourcePool::AcquireSet() {
		std::unique_lock<std::mutex> lock(m_descriptor_sets_mutex);
		if (m_descriptor_sets.empty()) {
			m_condition.wait(
				lock,
				[this]() {
					return !m_descriptor_sets.empty();
				}
			);
		}
		auto back = std::move(m_descriptor_sets.front());
		m_descriptor_sets.pop_back();
		return UniqueDescriptorSet(back, Collector(this));
	}

	DescriptorResourcePool::Collector::Collector(DescriptorResourcePool* owner) noexcept
		: m_owner(owner) {
	}

	void DescriptorResourcePool::Collector::operator()(vk::DescriptorSet descriptor_set) {
		std::lock_guard<std::mutex> lock(m_owner->m_descriptor_sets_mutex);
		m_owner->m_descriptor_sets.emplace_back(std::move(descriptor_set));
		m_owner->m_condition.notify_one();
	}

	vk::UniqueDescriptorPool const& DescriptorResourcePool::DescriptorPool() const noexcept {
		return m_descriptor_pool;
	}

	vk::UniqueDescriptorSetLayout const& DescriptorResourcePool::GetDescriptorSetLayout() const noexcept {
		return m_set_layout;
	}

}