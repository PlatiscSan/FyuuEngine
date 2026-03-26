module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_shader_data_segment_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_shader_data_segment;
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	WebGPUShaderDataSegment& WebGPUShaderDataSegment::Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		return *this;
	}

	WebGPUShaderDataSegment& WebGPUShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns) {
		return *this;
	}

	WebGPUShaderDataSegment& WebGPUShaderDataSegment::Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		return *this;
	}

	WebGPUShaderDataSegment& WebGPUShaderDataSegment::Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns) {
		return *this;
	}

	WebGPUShaderDataSegment& WebGPUShaderDataSegment::Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		return *this;
	}

	void WebGPUShaderDataSegment::Instantiate()
	{
	}

	void WebGPUShaderDataSegment::Reset()
	{
	}

} // namespace fyuu_rhi::webgpu