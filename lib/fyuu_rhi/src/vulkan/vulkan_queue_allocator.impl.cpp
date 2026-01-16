module;
#include "declare_pool.h"

module fyuu_rhi:vulkan_queue_allocator;

static constexpr std::size_t IndicesPoolSize = 256u;

namespace fyuu_rhi::vulkan {

	namespace details {

		std::set<std::uint32_t> VulkanQueueSet::Move(std::mutex& mutex, std::set<std::uint32_t> allocated_queue) {
			std::lock_guard<std::mutex> lock(mutex);
			std::set<std::uint32_t> moved = std::move(allocated_queue);
			return moved;
		}

		VulkanQueueSet::UniqueVulkanCommandQueueInfo VulkanQueueSet::Allocate(QueuePriority priority) {
			
			DECLARE_POOL(indices_pool, std::uint32_t, IndicesPoolSize);

			std::pmr::vector<std::uint32_t> indices(indices_pool_alloc);
			auto length = static_cast<std::uint32_t>(m_priority.size());

			switch (priority) {
			case fyuu_rhi::QueuePriority::Low:
				for (std::uint32_t i = 0; i < length; ++i) {
					if (m_priority[i] <= 0.33f) {
						indices.emplace_back(i);
					}
				}
				break;

			case fyuu_rhi::QueuePriority::Medium:
				for (std::uint32_t i = 0; i < length; ++i) {
					float current_priority = m_priority[i];
					if (current_priority > 0.33f && current_priority <= 0.67f) {
						indices.emplace_back(i);
					}
				}
				break;

			case fyuu_rhi::QueuePriority::High:
			default:
				for (std::uint32_t i = 0; i < length; ++i) {
					if (m_priority[i] > 0.67f) {
						indices.emplace_back(i);
					}
				}
				break;
			}

			if (indices.empty()) {
				throw VulkanQueueAllocationError("No queue satisfies the priority");
			}

			std::lock_guard<std::mutex> lock(m_allocated_queue_mutex);
			for (std::uint32_t index : indices) {
				auto end_iter = m_allocated_queue.end();
				if (m_allocated_queue.find(index) == end_iter) {
					return UniqueVulkanCommandQueueInfo(
						VulkanAllocatedCommandQueueInfo{ *m_info.family, index }, 
						VulkanCommandQueueInfoGC(GetID())
					);
				}
			}

			throw VulkanQueueAllocationError("No queue available");

		}

		VulkanQueueSet::VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float> priority)
			: m_info(info), m_priority(priority.begin(), priority.end()), m_allocated_queue(), m_allocated_queue_mutex() {
		}

		VulkanQueueSet::VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float const> priority)
			: m_info(info), m_priority(priority.begin(), priority.end()), m_allocated_queue(), m_allocated_queue_mutex() {

		}

		VulkanQueueSet::VulkanQueueSet(VulkanQueueSet&& other) noexcept
			: Registrable(std::move(other)),
			m_info(std::move(other.m_info)),
			m_priority(std::move(other.m_priority)),
			m_allocated_queue(VulkanQueueSet::Move(other.m_allocated_queue_mutex, other.m_allocated_queue)),
			m_allocated_queue_mutex() {
		}

		std::uint32_t VulkanQueueSet::GetFamily() const noexcept {
			return *m_info.family;
		}

		std::uint32_t VulkanQueueSet::GetTotalQueue() const noexcept {
			return m_info.num_available;
		}

		std::span<float const> VulkanQueueSet::GetPriority() const noexcept {
			return m_priority;
		}

		VulkanQueueSet::VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(std::size_t queue_set_id) noexcept
			: m_queue_set_id(queue_set_id) {
		}

		VulkanQueueSet::VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC const& other) noexcept
			: m_queue_set_id(other.m_queue_set_id) {
		}

		VulkanQueueSet::VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC&& other) noexcept
			: m_queue_set_id(std::exchange(other.m_queue_set_id, 0u)) {
		}

		void VulkanQueueSet::VulkanCommandQueueInfoGC::operator()(VulkanAllocatedCommandQueueInfo queue_info) {
			
			plastic::utility::AnyPointer<VulkanQueueSet> queue_set
				= plastic::utility::QueryObject<VulkanQueueSet>(m_queue_set_id);

			std::lock_guard<std::mutex> lock(queue_set->m_allocated_queue_mutex);
			queue_set->m_allocated_queue.erase(queue_info.index);

		}

	}

	VulkanQueueOptions VulkanQueueOptions::PlatformDefault() {

		VulkanQueueOptions options;

#if defined(_WIN32) || defined(__linux__)

		/*
		*	high performance desktop GPUs
		*/

		options.max_common = 3u;
		options.max_compute = 3u;
		options.max_copy = 3u;

		/// high, medium, low
		static constexpr std::array priority = { 1.00f, 0.33f, 0.67f };

		options.common_priority = priority;
		options.compute_priority = priority;
		options.copy_priority = priority;

#elif defined(__ANDROID__)

		/*
		*	mobile and embedded devices
		*/

		options.max_common = 1u;
		options.max_compute = 1u;
		options.max_copy = 1u;

		static constexpr std::array priority = { 1.00f };

		options.common_priority = priority;
		options.compute_priority = priority;
		options.copy_priority = priority;

#endif // defined(_WIN32) || defined(__linux__)

		return options;

	}

	/**
	* Creates a VulkanQueueSet by randomly selecting an appropriate queue from available queue information.
	* The function filters queues by type, randomly selects one, then configures it based on initialization options.
	*
	* @param type The type of queue to create (Copy, Compute, or AllCommands)
	* @param queue_infos Array of available Vulkan queue information to select from
	* @param init_options Configuration options including queue limits and priorities
	* @return A configured VulkanQueueSet ready for use
	*/
	static details::VulkanQueueSet CreateQueueSet(
		CommandObjectType type,
		std::span<VulkanCommandQueueInfo const> queue_infos,
		VulkanQueueOptions const& init_options
	) {
		// Step 1: Create a memory pool for temporary index storage
		// Using monotonic_buffer_resource for efficient, one-time allocation of index list
		std::array<std::byte, IndicesPoolSize> buffer;
		std::pmr::monotonic_buffer_resource indices_pool(&buffer, buffer.size());

		// Step 2: Create a polymorphic allocator using the memory pool
		// This allows the vector to allocate from our temporary buffer
		std::pmr::polymorphic_allocator<std::uint32_t> alloc(&indices_pool);

		// Step 3: Create vector to store indices of queues matching the requested type
		// Using PMR allocator to avoid heap allocations for small lists
		std::pmr::vector<std::uint32_t> indices(alloc);
		auto length = static_cast<std::uint32_t>(queue_infos.size());

		// Step 4: Filter queues - collect indices of all queues matching the requested type
		for (std::uint32_t i = 0; i < length; ++i) {
			if (queue_infos[i].type == type) {
				indices.emplace_back(i);
			}
		}

		///*
		//*	test output
		//*/

		//for (std::uint32_t index : indices) {
		//	VulkanCommandQueueInfo const& info = queue_infos[index];
		//	std::cout << "Filtered Queue - Type: "
		//		<< static_cast<std::uint32_t>(info.type)
		//		<< ", Family: " << info.family.value()
		//		<< ", Available: " << info.num_available
		//		<< std::endl;
		//}

		// Step 5: Randomly select one queue from the filtered indices
		// This provides load distribution when multiple queues of the same type exist
		std::random_device rd;                           // Non-deterministic random seed
		std::mt19937 gen(rd());                          // Mersenne Twister PRNG engine
		std::uniform_int_distribution<std::uint32_t> dist(0, static_cast<std::uint32_t>(indices.size()) - 1);

		std::uint32_t selected_index = dist(gen);        // Random index within filtered queue list

		// Step 6: Retrieve the selected queue's information
		VulkanCommandQueueInfo queue_info = queue_infos[indices[selected_index]];

		// Step 7: Configure the queue based on its type and initialization options
		switch (type) {
		case fyuu_rhi::CommandObjectType::Copy:
			// For copy queues: limit available count and set appropriate priority
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_copy)
			);
			return details::VulkanQueueSet(queue_info, init_options.copy_priority);

		case fyuu_rhi::CommandObjectType::Compute:
			// For compute queues: limit available count and set appropriate priority
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_compute)
			);
			return details::VulkanQueueSet(queue_info, init_options.compute_priority);

		case fyuu_rhi::CommandObjectType::AllCommands:
		default:
			// For universal/graphics queues: limit available count and set appropriate priority
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_common)
			);
			return details::VulkanQueueSet(queue_info, init_options.common_priority);
		}
	}

	VulkanQueueAllocator::VulkanQueueAllocator(
		std::span<VulkanCommandQueueInfo const> queue_infos,
		VulkanQueueOptions const& init_options
	) : m_queue_sets(
		[queue_infos, &init_options]() {
			decltype(m_queue_sets) queue_sets;
			queue_sets.emplace(CommandObjectType::AllCommands, CreateQueueSet(CommandObjectType::AllCommands, queue_infos, init_options));
			queue_sets.emplace(CommandObjectType::Compute, CreateQueueSet(CommandObjectType::Compute, queue_infos, init_options));
			queue_sets.emplace(CommandObjectType::Copy, CreateQueueSet(CommandObjectType::Copy, queue_infos, init_options));

			///*
			//*	test output
			//*/

			//for (auto& [type, set] : queue_sets) {
			//	std::cout << "Queue Set - Type: "
			//		<< static_cast<std::uint32_t>(type)
			//		<< ", Family: " << set.GetFamily()
			//		<< ", Total Queues: " << set.GetTotalQueue()
			//		<< ", Priorities: ";
			//	for (float priority : set.GetPriority()) {
			//		std::cout << priority << " ";
			//	}
			//	std::cout << std::endl;
			//}

			return queue_sets;

		}()) {

	}

	UniqueVulkanCommandQueueInfo VulkanQueueAllocator::Allocate(CommandObjectType type, QueuePriority priority) {
		return m_queue_sets.at(type).Allocate(priority);
	}

	std::uint32_t VulkanQueueAllocator::GetFamily(CommandObjectType type) const noexcept {
		return m_queue_sets.at(type).GetFamily();
	}

	std::uint32_t VulkanQueueAllocator::GetTotalQueue(CommandObjectType type) const noexcept {
		return m_queue_sets.at(type).GetTotalQueue();
	}

	std::span<float const> VulkanQueueAllocator::GetPriority(CommandObjectType type) const noexcept {
		return m_queue_sets.at(type).GetPriority();
	}

}