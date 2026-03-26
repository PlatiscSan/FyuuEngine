module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <variant>
#endif
#include "glad.hpp"
module fyuu_rhi:webgpu_view_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :webgpu_view;
import plastic.resource;
import :types;
import :webgpu_common;
import :webgpu_resource;

namespace fyuu_rhi::webgpu {



}

#endif // !defined(__APPLE__)