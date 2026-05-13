module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)

#endif //!defined(__APPLE__)
module fyuu_rhi:vulkan_future;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :vulkan_traits;

namespace fyuu_rhi::vulkan {
	Backend::Future Backend::GetFuture(Promise& promise) noexcept {
		return std::move(promise.last_fut);
	}
}
#endif // !defined(__APPLE__)