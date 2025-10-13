export module vulkan_backend:buffer;
import vulkan_hpp;
import std;
export import rendering;
export import :buffer_pool;
export import collective_resource;

namespace fyuu_engine::vulkan {

	export class VulkanBufferCollector {
	private:
		VulkanBufferPool* m_pool;

	public:
		VulkanBufferCollector(VulkanBufferPool* pool) noexcept;
		void operator()(VulkanBufferAllocation&& buffer);
		void operator()(VulkanBufferAllocation& buffer);
	};

	using UniqueVulkanBuffer = util::UniqueCollectiveResource<VulkanBufferAllocation, VulkanBufferCollector>;

}