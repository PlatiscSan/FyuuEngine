/* opengl_shader.impl.cpp */
/**
 * @file opengl_shader.impl.cpp
 * @brief Implementation of OpenGLShader and associated compilation utilities.
 *
 * This file contains the logic for compiling HLSL/GLSL to OpenGL shaders,
 * either via direct GLSL compilation, SPIR-V cross-compilation, or native
 * SPIR-V loading (if supported). It also handles file loading with automatic
 * language/stage inference, mirroring the structure of D3D12 and Vulkan
 * shader implementations.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <string_view>
#include <filesystem>
#include <span>
#include <format>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
module fyuu_rhi:opengl_shader_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_shader;
import :enums;
import :types;
import :opengl_common;
import :opengl_logical_device;
import :glslang;
import :shader_utility;

namespace fs = std::filesystem;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::opengl;

	/**
	 * @brief Convert a generic ShaderStage to the corresponding OpenGL shader type enum.
	 * @param stage The shader stage.
	 * @return OpenGL enum value (e.g., GL_VERTEX_SHADER).
	 * @throws std::runtime_error if the stage is not supported by OpenGL.
	 */
	GLenum StageToGLType(ShaderStage stage) {
		switch (stage) {
		case ShaderStage::Vertex:			   return GL_VERTEX_SHADER;
		case ShaderStage::Pixel:				return GL_FRAGMENT_SHADER;
		case ShaderStage::Geometry:			  return GL_GEOMETRY_SHADER;
		case ShaderStage::TessellationControl:   return GL_TESS_CONTROL_SHADER;
		case ShaderStage::TessellationEvaluation:return GL_TESS_EVALUATION_SHADER;
		case ShaderStage::Compute:			   return GL_COMPUTE_SHADER;
		case ShaderStage::Mesh:				  return GL_MESH_SHADER_NV;
		case ShaderStage::Amplification:		 return GL_TASK_SHADER_NV;
		default:
			throw std::runtime_error("Unsupported shader stage for OpenGL");
		}
	}

	/**
	 * @brief Compile GLSL source code directly using OpenGL.
	 * @param src	GLSL source string.
	 * @param stage  Shader stage.
	 * @param option Compilation options (unused for direct GLSL, but kept for consistency).
	 * @return OpenGL shader object handle (GLuint).
	 * @throws std::runtime_error if shader creation or compilation fails.
	 */
	GLuint CompileGLSL(
		std::string_view src,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		GLenum gl_type = StageToGLType(stage);
		GLuint shader = glCreateShader(gl_type);
		if (!shader) {
			throw std::runtime_error("Failed to create OpenGL shader object");
		}

		char const* src_ptr = src.data();
		GLint src_len = static_cast<GLint>(src.size());
		glShaderSource(shader, 1u, &src_ptr, &src_len);
		glCompileShader(shader);

		GLint success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			GLint log_len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
			std::string log(log_len, '\0');
			glGetShaderInfoLog(shader, log_len, nullptr, log.data());
			glDeleteShader(shader);
			throw std::runtime_error(std::format("OpenGL shader compilation failed: {}", log));
		}

		return shader;
	}

	/**
	 * @brief Cross‑compile SPIR-V bytecode to GLSL using SPIRV-Cross, then compile the resulting GLSL.
	 * @param spirv  Span of SPIR-V words.
	 * @param stage  Shader stage.
	 * @param option Compilation options (may specify entry point).
	 * @return OpenGL shader object handle.
	 */
	GLuint CompileSPIRVToGLSL(
		std::span<std::uint32_t const> spirv,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		spirv_cross::CompilerGLSL compiler(spirv.data(), spirv.size());
		
		spirv_cross::CompilerGLSL::Options glsl_options;
		glsl_options.version = 450;				// Target GLSL 4.50
#if defined(_WIN32)
		glsl_options.es = false;				// Not OpenGL ES
#elif defined(__linux__)
		glsl_options.es = false;				// Not OpenGL ES ?
#elif defined(__ANDROID__)
		glsl_options.es = true;					// OpenGL ES
#endif // defined(_WIN32)
		glsl_options.vulkan_semantics = false;	// Use plain GLSL semantics
		compiler.set_common_options(glsl_options);

		if (!option.entry_point.empty()) {
			compiler.set_entry_point(option.entry_point, ShaderStageToSPVExecutionModel(stage));
		}

		std::string glsl = compiler.compile();

		// Compile the generated GLSL source with OpenGL.
		// Note: we pass an empty ShaderCompilationOption because options are already applied.
		return CompileGLSL(glsl, stage, ShaderCompilationOption{});
	}

	/**
	 * @brief Load SPIR-V bytecode directly into an OpenGL shader if ARB_gl_spirv is available,
	 *		otherwise fall back to cross‑compilation to GLSL.
	 * @param spirv  Span of SPIR-V words.
	 * @param stage  Shader stage.
	 * @param option Compilation options (entry point needed for specialization).
	 * @return OpenGL shader object handle.
	 */
	GLuint LoadFromSPIRV(
		std::span<std::uint32_t const> spirv,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		if (GLAD_GL_ARB_gl_spirv) {
			GLenum gl_type = StageToGLType(stage);
			GLuint shader = glCreateShader(gl_type);
			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
						   spirv.data(), static_cast<GLsizei>(spirv.size() * sizeof(std::uint32_t)));
			glSpecializeShader(shader, option.entry_point.c_str(), 0, nullptr, nullptr);
			return shader;
		} else {
			// Fall back to cross‑compilation if direct SPIR-V loading is not supported.
			return CompileSPIRVToGLSL(spirv, stage, option);
		}
	}

	/**
	 * @brief Compile HLSL source to SPIR-V via glslang, then load/cross‑compile for OpenGL.
	 * @param src	HLSL source string.
	 * @param stage  Shader stage.
	 * @param option Compilation options.
	 * @return OpenGL shader object handle.
	 */
	GLuint CompileHLSL(
		std::string_view src,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		std::vector<std::uint32_t> spirv = CompileHLSLToSPIRV(src, stage, option);
		return LoadFromSPIRV(spirv, stage, option);
	}

	/**
	 * @brief Dispatch compilation based on source language.
	 * @param src	  Source code.
	 * @param stage	Shader stage.
	 * @param language Shader language (HLSL or GLSL).
	 * @param option   Compilation options.
	 * @return OpenGL shader object handle.
	 * @throws std::invalid_argument if language is not supported.
	 */
	GLuint Compile(
		std::string_view src,
		ShaderStage stage,
		ShaderLanguage language,
		ShaderCompilationOption const& option
	) {
		switch (language) {
		case ShaderLanguage::HLSL:
			return CompileHLSL(src, stage, option);
		case ShaderLanguage::GLSL:
			return CompileGLSL(src, stage, option);
		default:
			throw std::invalid_argument("Not supported shader language for OpenGL");
		}
	}

	/**
	 * @brief Convert a SPIR-V execution model to the corresponding ShaderStage enum.
	 * @param model SPIR-V execution model.
	 * @return Generic shader stage.
	 * @throws std::runtime_error if the model is not supported by OpenGL.
	 */
	ShaderStage SPVExecutionModelToStage(spv::ExecutionModel model) {
		using enum spv::ExecutionModel;
		switch (model) {
		case ExecutionModelVertex:		return ShaderStage::Vertex;
		case ExecutionModelFragment:	  return ShaderStage::Pixel;
		case ExecutionModelGeometry:	  return ShaderStage::Geometry;
		case ExecutionModelTessellationControl: return ShaderStage::TessellationControl;
		case ExecutionModelTessellationEvaluation: return ShaderStage::TessellationEvaluation;
		case ExecutionModelGLCompute:	 return ShaderStage::Compute;
		case ExecutionModelMeshEXT:	   return ShaderStage::Mesh;
		case ExecutionModelTaskEXT:	   return ShaderStage::Amplification;
		default:
			throw std::runtime_error("Unsupported SPIR-V execution model for OpenGL");
		}
	}

	/**
	 * @brief Reflect a SPIR-V binary to extract the entry point name and execution model.
	 * @param spirv Span of SPIR-V words.
	 * @return A pair containing the deduced ShaderStage and the entry point name.
	 * @throws std::runtime_error if the binary contains no entry points.
	 */
	std::pair<ShaderStage, std::string> ReflectSPIRV(std::span<std::uint32_t const> spirv) {
		spirv_cross::Compiler compiler(spirv.data(), spirv.size());
		auto entries = compiler.get_entry_points_and_stages();
		if (entries.empty()) {
			throw std::runtime_error("SPIR-V binary contains no entry points");
		}
		return { SPVExecutionModelToStage(entries.front().execution_model), entries.front().name };
	}

	/**
	 * @brief Read a shader from a file, auto‑detecting format and performing necessary compilation.
	 * @param path   Filesystem path.
	 * @param option Compilation options (used if recompilation is needed).
	 * @return OpenGL shader object handle.
	 * @throws std::runtime_error on failure (unknown format, stage not inferred, compilation errors).
	 */
	GLuint ReadFrom(
		fs::path const& path,
		ShaderCompilationOption const& option
	) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) {
			throw std::runtime_error(std::format("Failed to open shader file: {}", path.string()));
		}
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<std::byte> buffer(static_cast<std::size_t>(size));
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			throw std::runtime_error(std::format("Failed to read shader file: {}", path.string()));
		}

		// Detect SPIR-V magic number.
		bool is_spirv = false;
		if (buffer.size() >= 4) {
			std::uint32_t magic;
			std::memcpy(&magic, buffer.data(), 4);
			is_spirv = (magic == 0x07230203);
		}

		// Case 1: File contains precompiled SPIR-V bytecode.
		if (is_spirv) {
			std::span<std::uint32_t const> spirv(
				reinterpret_cast<std::uint32_t const*>(buffer.data()),
				buffer.size() / sizeof(std::uint32_t)
			);
			auto&& [stage, _] = ReflectSPIRV(spirv);
			// Compile SPIR-V to GLSL (or load directly if supported).
			return CompileSPIRVToGLSL(spirv, stage, option);
		}

		// Case 2: File contains source code – determine language and stage.
		std::string ext = path.extension().string();
		ShaderLanguage language = ShaderLanguage::GLSL; // default
		ShaderStage stage = ShaderStage::Unknown;

		if (ext == ".hlsl" || ext == ".hlsli" || ext == ".fx") {
			language = ShaderLanguage::HLSL;
			stage = InferStageFromHLSLFilename(path);
		} else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" ||
				   ext == ".geom" || ext == ".comp" || ext == ".tesc" || ext == ".tese") {
			language = ShaderLanguage::GLSL;
			stage = InferStageFromExtension(path);
		} else {
			throw std::runtime_error(std::format("Cannot infer shader language from extension: {}", ext));
		}

		if (stage == ShaderStage::Unknown) {
			throw std::runtime_error("Could not determine shader stage from file name/extension");
		}

		// Remove UTF-8 BOM if present.
		auto src_ptr = reinterpret_cast<char const*>(buffer.data());
		std::size_t src_size = buffer.size();
		if (src_size >= 3 &&
			static_cast<std::uint8_t>(src_ptr[0]) == 0xEF &&
			static_cast<std::uint8_t>(src_ptr[1]) == 0xBB &&
			static_cast<std::uint8_t>(src_ptr[2]) == 0xBF) {
			src_ptr += 3;
			src_size -= 3;
		}

		std::string_view source(src_ptr, src_size);
		return Compile(source, stage, language, option);
	}

} // unnamed namespace

namespace fyuu_rhi::opengl {

	OpenGLShader::OpenGLShader(
		OpenGLLogicalDevice const& logical_device,
		std::string_view src,
		ShaderStage stage,
		ShaderLanguage language,
		ShaderCompilationOption const& option
	) : PolymorphicShaderBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, src, stage, language, &option]() {
				MakeCurrent(); // Ensure the correct OpenGL context is current.
				return Compile(src, stage, language, option);
			}()) {
		// Compilation done in initializer list.
	}

	OpenGLShader::OpenGLShader(
		OpenGLLogicalDevice const& logical_device,
		std::span<std::byte const> bytes
	) : PolymorphicShaderBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, bytes]() {
				MakeCurrent();
				if (bytes.size() < 4) {
					throw std::runtime_error("Bytecode too small to be a valid SPIR-V binary");
				}

				std::uint32_t magic;
				std::memcpy(&magic, bytes.data(), 4);
				if (magic != 0x07230203) {
					throw std::runtime_error("Bytecode is not a valid SPIR-V binary (magic number mismatch)");
				}

				std::span<std::uint32_t const> spirv(
					reinterpret_cast<std::uint32_t const*>(bytes.data()),
					bytes.size() / sizeof(std::uint32_t)
				);

				auto&& [stage, entry_point] = ReflectSPIRV(spirv);

				// Create a temporary ShaderCompilationOption with the reflected entry point.
				ShaderCompilationOption opt;
				opt.entry_point = entry_point;
				return LoadFromSPIRV(spirv, stage, opt);
			}()) {
		// SPIR-V reflection and loading performed in lambda.
	}

	OpenGLShader::OpenGLShader(
		OpenGLLogicalDevice const& logical_device,
		fs::path const& path,
		ShaderCompilationOption const& option
	) : PolymorphicShaderBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, &path, &option]() {
				MakeCurrent();
				return ReadFrom(path, option);
			}()) {
		// File read, inference, compilation performed in ReadFrom.
	}

	OpenGLShader::OpenGLShader(OpenGLShader&& other) noexcept
		: PolymorphicShaderBase(std::move(other)),
		OpenGLCommon(std::move(other)),
		m_impl(std::exchange(other.m_impl, 0u)) {
	}

	OpenGLShader::~OpenGLShader() noexcept {
		if (m_impl) {
			MakeCurrent();
			glDeleteShader(m_impl);
			m_impl = 0u;
		}
	}

	GLuint OpenGLShader::GetNative() const noexcept {
		return m_impl;
	}

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)