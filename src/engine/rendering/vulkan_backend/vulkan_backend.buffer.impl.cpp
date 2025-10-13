module vulkan_backend:buffer;

namespace fyuu_engine::vulkan {
	VulkanBufferCollector::VulkanBufferCollector(VulkanBufferPool* pool) noexcept
		: m_pool(pool) {
	}
	void VulkanBufferCollector::operator()(VulkanBufferAllocation&& buffer) {
		m_pool->Free(buffer);
	}
	void VulkanBufferCollector::operator()(VulkanBufferAllocation& buffer) {
		m_pool->Free(buffer);
	}
}