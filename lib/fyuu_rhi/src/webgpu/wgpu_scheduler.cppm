module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <stdexcept>
#endif // !defined(__cpp_lib_modules)
#include <webgpu/webgpu_cpp.h>

module fyuu_rhi:webgpu_scheduler;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_traits;

namespace fyuu_rhi::webgpu {

	Backend::Scheduler Backend::CreateScheduler(
		LogicalDevice const& ld,
		SchedulerDescriptor const& descriptor
	) {
		using Flag = SchedulerFlagBits;
		if (!descriptor.flags.Test(Flag::Graphics) &&
			!descriptor.flags.Test(Flag::Compute) &&
			!descriptor.flags.Test(Flag::Copy)) {
			throw std::invalid_argument("CreateScheduler(): no execution capability was requested");
		}
		if (descriptor.flags.Test(Flag::Present) &&
			!descriptor.flags.Test(Flag::Graphics)) {
			throw std::invalid_argument("CreateScheduler(): presentation requires graphics capability");
		}

		return std::make_shared<WebGPUScheduler>(
			ld.GetQueue(),
			descriptor.flags
		);
	}

}
