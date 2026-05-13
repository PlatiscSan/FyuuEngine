module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include <webgpu/webgpu_cpp.h>
module fyuu_rhi:webgpu_future;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_traits;
namespace fyuu_rhi::webgpu {
	wgpu::Future Backend::GetFuture(wgpu::Future& promise) noexcept {
		return std::exchange(promise, {});
	}

}
