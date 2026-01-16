export module fyuu_rhi:vulkan_queue_allocator;
import std;
import plastic.any_pointer;
import plastic.registrable;
import plastic.unique_resource;
import :vulkan_physical_device;
import :command_object;

namespace fyuu_rhi::vulkan {

	export struct VulkanAllocatedCommandQueueInfo {
		std::uint32_t family;
		std::uint32_t index;
	};

	class VulkanQueueAllocationError
		: public std::runtime_error {
	public:
		VulkanQueueAllocationError(std::string const& message)
			: std::runtime_error(message) {

		}
	};

	namespace details {
		class VulkanQueueSet
			: public plastic::utility::Registrable<VulkanQueueSet> {
		private:
			VulkanCommandQueueInfo m_info;
			std::vector<float> m_priority;

			std::set<std::uint32_t> m_allocated_queue;
			std::mutex m_allocated_queue_mutex;

			static std::set<std::uint32_t> Move(std::mutex& mutex, std::set<std::uint32_t> allocated_queue);

		public:
			class VulkanCommandQueueInfoGC;

			using UniqueVulkanCommandQueueInfo
				= plastic::utility::UniqueResource<VulkanAllocatedCommandQueueInfo, VulkanCommandQueueInfoGC>;

			UniqueVulkanCommandQueueInfo Allocate(QueuePriority priority);

			VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float> priority);
			VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float const> priority);

			VulkanQueueSet(VulkanQueueSet&& other) noexcept;

			std::uint32_t GetFamily() const noexcept;

			std::uint32_t GetTotalQueue() const noexcept;

			std::span<float const> GetPriority() const noexcept;

		};

		class VulkanQueueSet::VulkanCommandQueueInfoGC {
		private:
			std::size_t m_queue_set_id;

		public:
			VulkanCommandQueueInfoGC(std::size_t queue_set_id) noexcept;
			VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC const& other) noexcept;
			VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC&& other) noexcept;

			void operator()(VulkanAllocatedCommandQueueInfo queue_info);
		};
	}

	export using UniqueVulkanCommandQueueInfo = details::VulkanQueueSet::UniqueVulkanCommandQueueInfo;

	export struct VulkanQueueOptions {
		std::size_t max_common;
		std::span<float const> common_priority;

		std::size_t max_compute;
		std::span<float const> compute_priority;

		std::size_t max_copy;
		std::span<float const> copy_priority;

		static VulkanQueueOptions PlatformDefault();
	};

	export class VulkanQueueAllocator {
	private:
		std::unordered_map<CommandObjectType, details::VulkanQueueSet> m_queue_sets;

	public:
		VulkanQueueAllocator(
			std::span<VulkanCommandQueueInfo const> queue_infos,
			VulkanQueueOptions const& init_options = VulkanQueueOptions::PlatformDefault()
		);

		UniqueVulkanCommandQueueInfo Allocate(CommandObjectType type, QueuePriority priority);

		std::uint32_t GetFamily(CommandObjectType type) const noexcept;

		std::uint32_t GetTotalQueue(CommandObjectType type) const noexcept;

		std::span<float const> GetPriority(CommandObjectType type) const noexcept;

	};

}