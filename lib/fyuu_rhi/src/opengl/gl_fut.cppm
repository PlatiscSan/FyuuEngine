module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <memory>
#include <utility>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"
#if defined(_WIN32)
#include "glad/glad_wgl.h"
#else
#if defined(__linux__)
#include "glad/glad_glx.h"
#endif // defined(__linux__)
#include "glad/glad_egl.h"
#endif // defined(_WIN32)
#endif //!defined(__APPLE__)
module fyuu_rhi:opengl_future;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_traits;

namespace fyuu_rhi::opengl {
	std::shared_ptr<std::remove_pointer_t<GLsync>> Backend::GetFuture(std::shared_ptr<std::remove_pointer_t<GLsync>>& promise) noexcept {
		return std::move(promise);
	}
}
#endif // !defined(__APPLE__)