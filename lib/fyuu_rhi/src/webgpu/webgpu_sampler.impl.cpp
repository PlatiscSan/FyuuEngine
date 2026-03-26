module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <variant>
#include <span>
#endif
#include "webgpu.hpp"

module fyuu_rhi:webgpu_sampler_impl;

#if defined(__cpp_lib_modules)
import std;
#endif
import :webgpu_sampler;
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	
} // namespace fyuu_rhi::webgpu