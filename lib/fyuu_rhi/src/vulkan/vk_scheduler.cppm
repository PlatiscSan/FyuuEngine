module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <stdexcept>
#include <utility>
#endif // !defined(__cpp_lib_modules)

module fyuu_rhi:vulkan_scheduler;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :vulkan_traits;

namespace {

	fyuu_rhi::vulkan::CommandQueueType GetQueueType(
		fyuu_rhi::execution::SchedulerFlags const& flags
	) {
		using Flag = fyuu_rhi::execution::SchedulerFlagBits;
		using Type = fyuu_rhi::vulkan::CommandQueueType;
		if (flags.Test(Flag::Graphics)) {
			return Type::Graphics;
		}
		if (flags.Test(Flag::Compute)) {
			return Type::Compute;
		}
		if (flags.Test(Flag::Copy)) {
			return Type::Copy;
		}
		throw std::invalid_argument("CreateScheduler(): no execution capability was requested");
	}

	fyuu_rhi::vulkan::QueuePriority GetQueuePriority(float priority) noexcept {
		using Priority = fyuu_rhi::vulkan::QueuePriority;
		if (priority >= 0.75f) {
			return Priority::High;
		}
		if (priority >= 0.25f) {
			return Priority::Medium;
		}
		return Priority::Low;
	}

	void ValidateSchedulerFlags(fyuu_rhi::execution::SchedulerFlags const& flags) {
		using Flag = fyuu_rhi::execution::SchedulerFlagBits;
		if (flags.Test(Flag::Present) && !flags.Test(Flag::Graphics)) {
			throw std::invalid_argument("CreateScheduler(): presentation requires graphics capability");
		}
	}

}

namespace fyuu_rhi::vulkan {

	Backend::Scheduler Backend::CreateScheduler(
		LogicalDevice& ld,
		SchedulerDescriptor const& descriptor
	) {
		ValidateSchedulerFlags(descriptor.flags);
		auto allocation = ld.queue_alloc.Allocate(
			GetQueueType(descriptor.flags),
			GetQueuePriority(descriptor.priority)
		);
		auto info = allocation->GetInfo();
		auto queue = ld.impl->getQueue(
			info.family,
			info.index,
			*ld.dispatcher
		);
		return std::make_shared<VulkanScheduler>(
			std::move(allocation),
			queue,
			descriptor.flags
		);
	}

}
#endif // !defined(__APPLE__)
