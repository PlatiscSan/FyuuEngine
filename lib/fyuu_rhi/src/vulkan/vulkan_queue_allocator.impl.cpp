/* vulkan_queue_allocator.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <chrono>
#include <random>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "declare_pool.hpp"
module fyuu_rhi:vulkan_queue_allocator_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_queue_allocator;
import vulkan;
import plastic.registrable;
import plastic.resource;
import :vulkan_types;
import :enums;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::vulkan;

	/**
	 * Creates a VulkanQueueSet by randomly selecting an appropriate queue family from the list.
	 * The function filters queues by type, randomly picks one, then applies limits and priorities
	 * from the init_options. Random selection distributes load across multiple families of the same type.
	 *
	 * @param type The required queue type.
	 * @param queue_infos List of all available queue families.
	 * @param init_options Configuration limits and priority lists.
	 * @return A configured VulkanQueueSet for that type.
	 */
	VulkanQueueSet CreateQueueSet(
		CommandObjectType type,
		std::span<VulkanCommandQueueInfo const> queue_infos,
		VulkanQueueOptions const& init_options
	) {

		DECLARE_POOL(indices, std::uint32_t, 256u);
		std::pmr::vector<std::uint32_t> indices(indices_alloc);
		indices.reserve(indices_pool_size);

		auto length = static_cast<std::uint32_t>(queue_infos.size());

		// Collect indices of queue families that support the requested type.
		for (std::uint32_t i = 0; i < length; ++i) {
			if (queue_infos[i].type == type) {
				indices.emplace_back(i);
			}
		}

		// Randomly select one family from the matching ones.

		static thread_local std::mt19937 gen(std::random_device{}());
		static thread_local std::uniform_int_distribution<std::size_t> dist;
		using param_t = std::uniform_int_distribution<std::size_t>::param_type;
		dist.param(param_t{ 0, indices.size() - 1 });

		std::size_t selected_index = dist(gen);

		VulkanCommandQueueInfo queue_info = queue_infos[indices[selected_index]];

		// Cap the number of queues to use according to init_options and assign priorities.
		switch (type) {
		case fyuu_rhi::CommandObjectType::Copy:
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_copy)
			);
			return VulkanQueueSet(queue_info, init_options.copy_priority.subspan(0, queue_info.num_available));

		case fyuu_rhi::CommandObjectType::Compute:
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_compute)
			);
			return VulkanQueueSet(queue_info, init_options.compute_priority.subspan(0, queue_info.num_available));

		case fyuu_rhi::CommandObjectType::AllCommands:
		default:
			queue_info.num_available = std::min(
				queue_info.num_available,
				static_cast<std::uint32_t>(init_options.max_common)
			);
			return VulkanQueueSet(queue_info, init_options.common_priority.subspan(0, queue_info.num_available));
		}


	}

}

namespace fyuu_rhi::vulkan {

	// GC constructors – trivial copy/move of the optional ID.
	VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(std::optional<std::size_t> const& queue_set_id) noexcept
		: m_queue_set_id(queue_set_id) {
	}

	VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC const& other) noexcept
		: m_queue_set_id(other.m_queue_set_id) {
	}

	VulkanCommandQueueInfoGC::VulkanCommandQueueInfoGC(VulkanCommandQueueInfoGC&& other) noexcept
		: m_queue_set_id(std::move(other.m_queue_set_id)) {
	}

	// Deleter: return the queue index to its originating set.
	void VulkanCommandQueueInfoGC::operator()(VulkanAllocatedCommandQueueInfo queue_info) {

		if (!m_queue_set_id) {
			return;
		}

		VulkanQueueSet* queue_set = plastic::utility::QueryObject<VulkanQueueSet>(*m_queue_set_id);
		if (!queue_set) {
			return;
		}

		std::lock_guard<std::mutex> lock(queue_set->m_allocated_queue_mutex);
		queue_set->m_allocated_queue.erase(queue_info.index);

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

	// Move the allocated set out from under the lock.
	std::unordered_set<std::uint32_t> VulkanQueueSet::Move(std::mutex& mutex, std::unordered_set<std::uint32_t> allocated_queue) {
		std::lock_guard<std::mutex> lock(mutex);
		std::unordered_set<std::uint32_t> moved = std::move(allocated_queue);
		return moved;
	}

	// Attempt to allocate a queue that satisfies the given priority.
	UniqueVulkanCommandQueueInfo VulkanQueueSet::Allocate(QueuePriority priority) {

		// Use a pool allocator for temporary index storage (reduces heap fragmentation).
		DECLARE_POOL(indices, std::uint32_t, 256u);
		std::pmr::vector<std::uint32_t> indices(indices_alloc);
		indices.reserve(indices_pool_size);

		auto length = static_cast<std::uint32_t>(m_priority.size());

		// Gather all queue indices whose priority falls into the requested band.
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
			throw std::runtime_error("No queue satisfies the priority");
		}

		// Lock the allocated set and pick the first free index from the candidate list.
		std::lock_guard<std::mutex> lock(m_allocated_queue_mutex);
		for (std::uint32_t index : indices) {
			auto end_iter = m_allocated_queue.end();
			if (m_allocated_queue.find(index) == end_iter) {
				// Found a free queue; return a handle with a deleter that knows this set's ID.
				return UniqueVulkanCommandQueueInfo(
					VulkanAllocatedCommandQueueInfo{ *m_info.family, index },
					VulkanCommandQueueInfoGC(GetID())
				);
			}
		}

		throw std::runtime_error("No queue available");
	}

	// Constructors: copy priorities into internal vector.
	VulkanQueueSet::VulkanQueueSet(VulkanCommandQueueInfo const& info, std::span<float const> priority)
		: Registrable(this),
		m_info(info),
		m_priority(priority.begin(), priority.end()),
		m_allocated_queue(),
		m_allocated_queue_mutex() {

	}

	// Move constructor: move the allocated set using the thread‑safe helper.
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

	// Constructor builds the three queue sets by calling CreateQueueSet for each type.
	VulkanQueueAllocator::VulkanQueueAllocator(
		std::span<VulkanCommandQueueInfo const> queue_infos,
		VulkanQueueOptions const& init_options
	) : m_queue_sets(
		[queue_infos, &init_options]() {
			decltype(m_queue_sets) queue_sets;
			queue_sets.emplace(CommandObjectType::AllCommands, CreateQueueSet(CommandObjectType::AllCommands, queue_infos, init_options));
			queue_sets.emplace(CommandObjectType::Compute, CreateQueueSet(CommandObjectType::Compute, queue_infos, init_options));
			queue_sets.emplace(CommandObjectType::Copy, CreateQueueSet(CommandObjectType::Copy, queue_infos, init_options));

			return queue_sets;

		}()) {

	}

	// Forward allocation to the appropriate queue set.
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


#endif // !defined(__APPLE__)