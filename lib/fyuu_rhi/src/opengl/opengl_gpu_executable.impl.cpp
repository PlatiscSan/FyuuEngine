module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <variant>
#include <optional>
#include <span>
#include <format>
#include <algorithm>
#include <ranges>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
module fyuu_rhi:opengl_gpu_executable_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_gpu_executable;
import :enums;
import :types;
import :opengl_common;
import :opengl_logical_device;
import :opengl_shader_data_segment;
import :opengl_shader;
import :opengl_types;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::opengl;

	GLbitfield StageToBit(ShaderStage stage) noexcept {

		if (HasConflictingFlags(stage, ShaderStage::All)) {
			return 0u;
		}

		switch (stage) {
		case ShaderStage::Vertex:					return GL_VERTEX_SHADER_BIT;
		case ShaderStage::TessellationControl:  	return GL_TESS_CONTROL_SHADER_BIT;
		case ShaderStage::TessellationEvaluation:	return GL_TESS_EVALUATION_SHADER_BIT;
		case ShaderStage::Geometry:					return GL_GEOMETRY_SHADER_BIT;
		case ShaderStage::Fragment:					return GL_FRAGMENT_SHADER_BIT;
		case ShaderStage::Compute:					return GL_COMPUTE_SHADER_BIT;
		default: 									return 0u;
		}

	}

	GLenum ToOpenGLTopology(GPUExecutableFlags const& flags) noexcept {

		using namespace GPUExecutableFlagBits;

		if (HasConflictingFlags(flags, PrimitiveTopologyAll)) {
			return 0u;
		}
	
		if (flags & PrimitiveTopologyPointList)
			return GL_POINTS;
		if (flags & PrimitiveTopologyLineList)
			return GL_LINES;
		if (flags & PrimitiveTopologyLineStrip)
			return GL_LINE_STRIP;
		if (flags & PrimitiveTopologyTriangleList)
			return GL_TRIANGLES;
		if (flags & PrimitiveTopologyTriangleStrip)
			return GL_TRIANGLE_STRIP;
		if (flags & PrimitiveTopologyPatchList)
			return GL_PATCHES;
	
		return 0u;

	}

} // anonymous namespace

namespace fyuu_rhi::opengl {
	OpenGLGPUExecutable::Compilation::Compilation(OpenGLGPUExecutable* execu)
		: OpenGLCommon(this, execu) {
	}

	OpenGLGPUExecutable::Compilation::~Compilation() noexcept {
		MakeCurrent();
		if (pipeline) {
			glDeleteProgramPipelines(1, &pipeline);
		}
		for (auto& [_, prog] : stage_programs) {
			if (prog) {
				glDeleteProgram(prog);
			}
		}
	}

	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------
	OpenGLGPUExecutable::OpenGLGPUExecutable(OpenGLLogicalDevice const& logical_device)
		: PolymorphicGPUExecutableBase(this),
		OpenGLCommon(this, logical_device),
		m_handle(Declaration{}) {
	}

	// ------------------------------------------------------------------------
	// Builder helpers (ensure we are in Declaration state)
	// ------------------------------------------------------------------------
#define ENSURE_DECLARATION \
	auto* declaration = std::get_if<Declaration>(&m_handle); \
	if (!declaration) throw std::logic_error("Cannot modify after compilation");

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetShader(ShaderStage stage, OpenGLShader const& shader) {
		ENSURE_DECLARATION
		if (HasConflictingFlags(stage, ShaderStage::All)) {
			throw std::invalid_argument("Invalid stage: specifies multiple shader stages");
		}
		declaration->shaders[stage] = shader.GetID();
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetShaderDataSegment(OpenGLShaderDataSegment const& shader_data_segment) {
		ENSURE_DECLARATION
		declaration->shader_data_segment_id = shader_data_segment.GetID();
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetFlags(GPUExecutableFlags const& flags) {
		ENSURE_DECLARATION
		declaration->flags = flags;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info) {
		ENSURE_DECLARATION
		declaration->rt_att = info;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetRenderTargetAttachments(
		std::span<BlendInfo const> blend_info,
		std::span<std::uint8_t const> color_write_masks
	) {
		ENSURE_DECLARATION
		auto& rt_att = declaration->rt_att;
		rt_att.blend_state.emplace<std::vector<BlendInfo>>(blend_info.begin(), blend_info.end());
		rt_att.color_write_masks.assign(color_write_masks.begin(), color_write_masks.end());
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetRenderTargetAttachments(
		LogicOp logic_op,
		std::span<std::uint8_t const> color_write_masks
	) {
		ENSURE_DECLARATION
		auto& rt_att = declaration->rt_att;
		rt_att.blend_state.emplace<LogicOp>(logic_op);
		rt_att.color_write_masks.assign(color_write_masks.begin(), color_write_masks.end());
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetDepthBias(DepthBias const& depth_bias) {
		ENSURE_DECLARATION
		declaration->depth_bias = depth_bias;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetDepthStencil(DepthStencilInfo const& depth_stencil) {
		ENSURE_DECLARATION
		declaration->depth_stencil = depth_stencil;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetMultiSample(MultiSample const& multi_sample) {
		ENSURE_DECLARATION
		declaration->multi_sample = multi_sample;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetRenderTargetFormats(std::span<ResourceFlags const> format) {
		ENSURE_DECLARATION
		declaration->rt_formats.clear();
		declaration->rt_formats.assign(format.begin(), format.end());
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetVertexInputLayout(VertexInputLayout const& vertex_input_layout) {
		ENSURE_DECLARATION
		declaration->vertex_input_layout = vertex_input_layout;
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::SetVertexInputLayout(
		std::span<VertexBinding const> bindings,
		std::span<VertexAttribute const> attributes
	) {
		ENSURE_DECLARATION
		auto& layout = declaration->vertex_input_layout;
		layout.bindings.assign(bindings.begin(), bindings.end());
		layout.attributes.assign(attributes.begin(), attributes.end());
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute) {
		ENSURE_DECLARATION
		auto& layout = declaration->vertex_input_layout;
		layout.bindings.emplace_back(binding);
		layout.attributes.emplace_back(attribute);
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::AddVertexBinding(VertexBinding const& binding) {
		ENSURE_DECLARATION
		declaration->vertex_input_layout.bindings.emplace_back(binding);
		return *this;
	}

	OpenGLGPUExecutable& OpenGLGPUExecutable::AddVertexAttribute(VertexAttribute const& attribute) {
		ENSURE_DECLARATION
		declaration->vertex_input_layout.attributes.emplace_back(attribute);
		return *this;
	}

	// ------------------------------------------------------------------------
	// Compilation methods (mirror Vulkan)
	// ------------------------------------------------------------------------
	void OpenGLGPUExecutable::CompileGraphicsExecutable(Declaration* declaration) {
		if (!declaration->shader_data_segment_id) {
			throw std::invalid_argument("Invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<OpenGLShaderDataSegment>(*declaration->shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("Shader data segment lost");
		}

		// For OpenGL, we don't use a single pipeline layout; the shader data segment
		// is responsible for binding resources (via uniform buffers, textures, etc.)
		// We still need to store its ID in the compilation state.

		GLuint pipeline = 0u;
		glCreateProgramPipelines(1, &pipeline);
		if (pipeline == 0u) {
			throw std::runtime_error("Failed to create program pipeline");
		}

		std::unordered_map<ShaderStage, GLuint> stage_programs;
		bool success = true;
		std::string error_log;

		for (auto const& [stage, shader_id] : declaration->shaders) {
			if (!shader_id) {
				error_log = "Invalid shader ID for stage";
				success = false;
				break;
			}

			auto* shader = plastic::utility::QueryObject<OpenGLShader>(*shader_id);
			if (!shader) {
				error_log = "Shader object lost";
				success = false;
				break;
			}

			GLuint prog = glCreateProgram();
			if (prog == 0u) {
				error_log = "Failed to create program for stage";
				success = false;
				break;
			}

			glAttachShader(prog, shader->GetNative());
			glLinkProgram(prog);

			GLint link_status = 0u;
			glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
			if (!link_status) {
				GLint log_len = 0u;
				glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
				std::string log(log_len, 0);
				glGetProgramInfoLog(prog, log_len, nullptr, log.data());
				error_log = std::format("Link failed for stage {}: {}", static_cast<std::size_t>(stage), log);
				glDeleteProgram(prog);
				success = false;
				break;
			}

			glUseProgramStages(pipeline, StageToBit(stage), prog);
			stage_programs[stage] = prog;
		}

		if (!success) {
			for (auto& [_, prog] : stage_programs) {
				glDeleteProgram(prog);
			}
			glDeleteProgramPipelines(1, &pipeline);
			throw std::runtime_error(error_log);
		}

		std::vector<GLenum> rt_formats;
		std::ranges::transform(
			declaration->rt_formats, 
			std::back_inserter(rt_formats),
			[](ResourceFlags const f) { 
				return DetermineInternalFormat(f);
			}
		);

		// Store compilation result
		Compilation& compilation = m_handle.emplace<Compilation>(this);

		compilation.stage_programs = std::move(stage_programs),
		compilation.rt_formats = std::move(rt_formats),
		compilation.shader_data_segment_id = declaration->shader_data_segment_id, 
		compilation.pso_type = ShaderStage::Graphics, 
		compilation.topology = ToOpenGLTopology(declaration->flags),
		compilation.pipeline = pipeline;

	}

	void OpenGLGPUExecutable::CompileComputingExecutable(Declaration* declaration) {
		if (!declaration->shader_data_segment_id) {
			throw std::invalid_argument("Invalid shader data segment");
		}
		auto* shader_data_segment = plastic::utility::QueryObject<OpenGLShaderDataSegment>(*declaration->shader_data_segment_id);
		if (!shader_data_segment) {
			throw std::runtime_error("Shader data segment lost");
		}

		GLuint pipeline = 0u;
		glCreateProgramPipelines(1, &pipeline);
		if (pipeline == 0u) {
			throw std::runtime_error("Failed to create program pipeline");
		}

		// Find the compute shader
		GLuint compute_prog = 0u;
		for (auto const& [stage, shader_id] : declaration->shaders) {
			if ((stage & ShaderStage::Compute) == ShaderStage::Unknown) {
				continue;
			}
			if (!shader_id) {
				throw std::invalid_argument("Invalid compute shader ID");
			}
			auto* shader = plastic::utility::QueryObject<OpenGLShader>(*shader_id);
			if (!shader) {
				throw std::runtime_error("Compute shader lost");
			}
			compute_prog = glCreateProgram();
			if (compute_prog == 0u) {
				throw std::runtime_error("Failed to create compute program");
			}
			glAttachShader(compute_prog, shader->GetNative());
			glLinkProgram(compute_prog);
			GLint link_status = 0u;
			glGetProgramiv(compute_prog, GL_LINK_STATUS, &link_status);
			if (!link_status) {
				GLint log_len = 0u;
				glGetProgramiv(compute_prog, GL_INFO_LOG_LENGTH, &log_len);
				std::string log(log_len, 0);
				glGetProgramInfoLog(compute_prog, log_len, nullptr, log.data());
				glDeleteProgram(compute_prog);
				throw std::runtime_error(std::format("Compute shader link failed: {}", log));
			}
			glUseProgramStages(pipeline, GL_COMPUTE_SHADER_BIT, compute_prog);
			break;
		}

		if (compute_prog == 0u) {
			throw std::invalid_argument("No compute shader attached");
		}

		Compilation& compilation = m_handle.emplace<Compilation>(this);

		compilation.stage_programs[ShaderStage::Compute] = compute_prog,
		compilation.shader_data_segment_id = declaration->shader_data_segment_id, 
		compilation.pso_type = ShaderStage::Compute, 
		compilation.topology = ToOpenGLTopology(declaration->flags),
		compilation.pipeline = pipeline;

	}

	void OpenGLGPUExecutable::CompileMeshExecutable(Declaration* declaration) {
		// Not supported in OpenGL core profile
		throw std::runtime_error("Mesh shaders are not supported in OpenGL");
	}

	void OpenGLGPUExecutable::CompileRayTracingExecutable(Declaration* declaration) {
		// Not supported in OpenGL
		throw std::runtime_error("Ray tracing is not supported in OpenGL");
	}

	void OpenGLGPUExecutable::Compile() {
		auto* declaration = std::get_if<Declaration>(&m_handle);
		if (!declaration) {
			throw std::logic_error("Already compiled");
		}

		if (HasConflictingFlags(declaration->flags, GPUExecutableFlagBits::AllType)) {
			throw std::invalid_argument("Invalid parameters: Multiple types were set.");
		}

		MakeCurrent();

		if (declaration->flags & GPUExecutableFlagBits::Graphics) {
			CompileGraphicsExecutable(declaration);
		} else if (declaration->flags & GPUExecutableFlagBits::Compute) {
			CompileComputingExecutable(declaration);
		} else if (declaration->flags & GPUExecutableFlagBits::Mesh) {
			CompileMeshExecutable(declaration);
		} else if (declaration->flags & GPUExecutableFlagBits::RayTracing) {
			CompileRayTracingExecutable(declaration);
		} else {
			throw std::invalid_argument("Invalid parameter: No type set.");
		}
	}

	// ------------------------------------------------------------------------
	// Queries (only after compilation)
	// ------------------------------------------------------------------------
	std::optional<std::size_t> OpenGLGPUExecutable::GetAssociatedShaderDataSegmentID() const noexcept {
		if (auto* compilation = std::get_if<Compilation>(&m_handle)) {
			return compilation->shader_data_segment_id;
		}
		return {};
	}

	std::pair<ShaderStage, GLuint> OpenGLGPUExecutable::GetPSO() const noexcept {
		if (auto* compilation = std::get_if<Compilation>(&m_handle)) {
			return { compilation->pso_type, compilation->pipeline };
		}
		return { ShaderStage::Unknown, 0u };
	}

	// ------------------------------------------------------------------------
	// Reset
	// ------------------------------------------------------------------------
	void OpenGLGPUExecutable::Reset() {
		m_handle.emplace<Declaration>();
	}

	// ------------------------------------------------------------------------
	// Pipeline binding (OpenGL specific)
	// ------------------------------------------------------------------------
	void OpenGLGPUExecutable::Bind() {
		auto* compilation = std::get_if<Compilation>(&m_handle);
		if (!compilation) {
			throw std::logic_error("Pipeline not compiled yet");
		}
		if (compilation->pipeline == 0u) {
			throw std::logic_error("Invalid pipeline handle");
		}
		MakeCurrent();
		glBindProgramPipeline(compilation->pipeline);
	}

	void OpenGLGPUExecutable::Unbind() noexcept {
		if (auto* compilation = std::get_if<Compilation>(&m_handle)) {
			if (compilation->pipeline != 0u) {
				MakeCurrent();
				glBindProgramPipeline(0u);
			}
		}
	}

	GLenum OpenGLGPUExecutable::GetTopology() const noexcept {
		if (auto* compilation = std::get_if<Compilation>(&m_handle)) {
			return compilation->topology;
		}
		return 0u;
	}

	GLuint OpenGLGPUExecutable::GetProgram(ShaderStage stage) const noexcept {
		auto* compilation = std::get_if<Compilation>(&m_handle);
		if (!compilation) {
			throw std::logic_error("Pipeline not compiled yet");
		}
		auto iter = compilation->stage_programs.find(stage);
		if (iter == compilation->stage_programs.end()) {
			return 0u;
		}
		return iter->second;
	}

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)