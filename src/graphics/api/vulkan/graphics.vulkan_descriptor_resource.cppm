export module graphics:vulkan_descriptor_resource;
import std;
export import :vulkan_command_object;
export import disable_copy;
export import collective_resource;

namespace graphics::api::vulkan {

	export class DescriptorResourcePool : public util::DisableCopy<DescriptorResourcePool> {
	private:
		vk::UniqueDescriptorSetLayout m_set_layout;
		vk::UniqueDescriptorPool m_descriptor_pool;

		std::vector<vk::DescriptorSet> m_descriptor_sets;
		std::mutex m_descriptor_sets_mutex;
		std::condition_variable m_condition;

		std::uint32_t m_count;

		static std::unique_lock<std::mutex> WaitForAllocated(DescriptorResourcePool* pool);

	public:
		class Collector {
		private:
			DescriptorResourcePool* m_owner;

		public:
			Collector(DescriptorResourcePool* owner) noexcept;
			void operator()(vk::DescriptorSet descriptor_set);
		};

		using UniqueDescriptorSet = util::UniqueCollectiveResource<vk::DescriptorSet, Collector>;

		DescriptorResourcePool(
			vk::UniqueDevice const& logical_device,
			vk::DescriptorType type,
			std::uint32_t count,
			vk::ShaderStageFlags stage_flags = vk::ShaderStageFlagBits::eAll,
			vk::AllocationCallbacks const* allocator = nullptr
		);

		DescriptorResourcePool(DescriptorResourcePool&& other) noexcept;
		DescriptorResourcePool& operator=(DescriptorResourcePool&& other) noexcept;

		~DescriptorResourcePool() noexcept;

		vk::DescriptorType PoolType() const noexcept;

		UniqueDescriptorSet AcquireSet();

		vk::UniqueDescriptorPool const& DescriptorPool() const noexcept;
		vk::UniqueDescriptorSetLayout const& GetDescriptorSetLayout() const noexcept;

	};

}
