module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <stdexcept>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"
#endif // !defined(__APPLE__)

module fyuu_rhi:opengl_scheduler;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_traits;

namespace fyuu_rhi::opengl {

	Backend::Scheduler Backend::CreateScheduler(
		LogicalDevice const& ld,
		SchedulerDescriptor const& descriptor
	) {
		using Flag = SchedulerFlagBits;
		if (!ld) {
			throw std::invalid_argument("CreateScheduler(): logical device must not be null");
		}
		if (!descriptor.flags.Test(Flag::Graphics) &&
			!descriptor.flags.Test(Flag::Compute) &&
			!descriptor.flags.Test(Flag::Copy)) {
			throw std::invalid_argument("CreateScheduler(): no execution capability was requested");
		}
		if (descriptor.flags.Test(Flag::Present) &&
			!descriptor.flags.Test(Flag::Graphics)) {
			throw std::invalid_argument("CreateScheduler(): presentation requires graphics capability");
		}

		return std::make_shared<GLScheduler>(
			ld,
			descriptor.flags
		);
	}

}
#endif // !defined(__APPLE__)
