/* vulkan_queue_allocator.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <span>
#endif
export module fyuu_rhi:vulkan_queue_allocator;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import vulkan;
import plastic.registrable;
import plastic.resource;
import :vulkan_types;
import :enums;

namespace fyuu_rhi::vulkan {
	// Info structure that uniquely identifies an allocated queue:
	// queue family index and queue index within that family.
	export struct VulkanAllocatedCommandQueueInfo {
		std::uint32_t family;
		std::uint32_t index;
	};

	// Configuration options for the queue allocator.
	export struct VulkanQueueOptions {
		std::size_t max_common;                     // Max number of universal (graphics) queues to use
		std::span<float const> common_priority;     // Priorities for each universal queue

		std::size_t max_compute;                     // Max number of compute queues
		std::span<float const> compute_priority;     // Priorities for compute queues

		std::size_t max_copy;                         // Max number of copy queues
		std::span<float const> copy_priority;         // Priorities for copy queues

		static VulkanQueueOptions PlatformDefault();  // Returns a default configuration for the current platform
	};


	/**
	 * Deleter for UniqueVulkanCommandQueueInfo.
	 * When a UniqueVulkanCommandQueueInfo is destroyed, this functor is called with
	 * the VulkanAllocatedCommandQueueInfo. It looks up the originating VulkanQueueSet
	 * via its registered ID and erases the queue index from the allocated set,
	 * effectively freeing it for future allocations.
	 */
	export class VulkanCommandQueueInfoGC {
	private:
		std::optional<std::size_t> m_queue_set_id; // ID of the VulkanQueueSet that owns this queue

	public:
		VulkanCommandQueueInfoGC(std::optional<std::size_t> const& queue_set_id) noexcept;
		VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC const& other) noexcept;
		VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC&& other) noexcept;

		void operator()(VulkanAllocatedCommandQueueInfo queue_info);
	};

	export using UniqueVulkanCommandQueueInfo = plastic::utility::UniqueResource<VulkanAllocatedCommandQueueInfo, VulkanCommandQueueInfoGC>;


	/**
	 * VulkanQueueSet represents a set of queues belonging to the same queue family.
	 * It tracks which individual queues are currently allocated and allows allocation
	 * based on priority. The class is registrable so that its instances can be looked up
	 * by an ID (used by the deleter to return queues).
	 */
	class VulkanQueueSet
		: public plastic::utility::Registrable<VulkanQueueSet> {
		// Deleter functor for UniqueVulkanCommandQueueInfo that returns the queue to the set.
		friend class VulkanCommandQueueInfoGC;
	private:
		VulkanCommandQueueInfo m_info;          // Basic info about the queue family (family index, total count, type)
		std::vector<float> m_priority;           // Priority values for each queue in this family (size = total queues)

		std::unordered_set<std::uint32_t> m_allocated_queue; // Indices that are currently in use
		std::mutex m_allocated_queue_mutex;      // Protects m_allocated_queue

		// Helper to move the allocated set under lock.
		static std::unordered_set<std::uint32_t> Move(std::mutex& mutex, std::unordered_set<std::uint32_t> allocated_queue);

	public:
		// Allocate a queue from this set that matches the requested priority.
		UniqueVulkanCommandQueueInfo Allocate(QueuePriority priority);

		// Constructors: store queue family info and per‑queue priorities.
		VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float const> priority);

		// Move constructor – transfers ownership of allocated indices.
		VulkanQueueSet(VulkanQueueSet&& other) noexcept;

		// Accessors.
		std::uint32_t GetFamily() const noexcept;
		std::uint32_t GetTotalQueue() const noexcept;
		std::span<float const> GetPriority() const noexcept;

	};

	/**
	 * VulkanQueueAllocator is the main interface for allocating Vulkan queues.
	 * It holds three VulkanQueueSet objects – one for each queue type (AllCommands, Compute, Copy).
	 * Allocation requests are forwarded to the appropriate set.
	 */
	export class VulkanQueueAllocator {
	private:
		std::unordered_map<CommandObjectType, VulkanQueueSet> m_queue_sets;

	public:
		// Constructor: builds the three queue sets by selecting appropriate families from the given list.
		VulkanQueueAllocator(
			std::span<VulkanCommandQueueInfo const> queue_infos,
			VulkanQueueOptions const& init_options = VulkanQueueOptions::PlatformDefault()
		);

		// Allocate a queue of the given type with the requested priority.
		UniqueVulkanCommandQueueInfo Allocate(CommandObjectType type, QueuePriority priority);

		// Query properties of the underlying queue set for a given type.
		std::uint32_t GetFamily(CommandObjectType type) const noexcept;
		std::uint32_t GetTotalQueue(CommandObjectType type) const noexcept;
		std::span<float const> GetPriority(CommandObjectType type) const noexcept;

	};

}
#endif // !defined(__APPLE__)