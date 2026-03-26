module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_gpu_executable_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_gpu_executable;
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetShader(ShaderStage stage, WebGPUShader const& shader) {
		return *this;
	}
	
	WebGPUGPUExecutable& WebGPUGPUExecutable::SetShaderDataSegment(WebGPUShaderDataSegment const& shader_data_segment) {
		return *this;
	}
	
	WebGPUGPUExecutable& WebGPUGPUExecutable::SetFlags(GPUExecutableFlags const& flags) {
		return *this;
	}
	
	WebGPUGPUExecutable& WebGPUGPUExecutable::SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info) {
		return *this;
	}
	
	WebGPUGPUExecutable& WebGPUGPUExecutable::SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks) {
		return *this;
	}
	
	WebGPUGPUExecutable& WebGPUGPUExecutable::SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetDepthBias(DepthBias const& depth_bias) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetDepthStencil(DepthStencilInfo const& depth_stencil) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetMultiSample(MultiSample const& multi_sample) {
		return *this;
	}

	WebGPUGPUExecutable &WebGPUGPUExecutable::SetRenderTargetFormats(std::span<ResourceFlags const> format) {
		// TODO: insert return statement here
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetVertexInputLayout(VertexInputLayout const& vertex_input_layout) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::AddVertexBinding(VertexBinding const& binding) {
		return *this;
	}

	WebGPUGPUExecutable& WebGPUGPUExecutable::AddVertexAttribute(VertexAttribute const& attribute) {
		return *this;
	}

	void WebGPUGPUExecutable::Compile() {

	}

	void WebGPUGPUExecutable::Reset() {

	}

} // namespace fyuu_rhi::webgpu