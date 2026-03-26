/* opengl_shader.cppm */
/**
 * @file opengl_shader.cppm
 * @brief Module interface for OpenGL shader implementation.
 *
 * This module provides the OpenGLShader class, which encapsulates an OpenGL
 * shader object (GLuint). It supports creating a shader from source code
 * (HLSL/GLSL), from precompiled SPIR-V bytecode, or by loading and compiling
 * a shader file with automatic language and stage inference.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <string_view>
#include <filesystem>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
export module fyuu_rhi:opengl_shader;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :opengl_common;

namespace fs = std::filesystem;

namespace fyuu_rhi::opengl {

    /**
     * @class OpenGLShader
     * @brief Represents an OpenGL shader object.
     *
     * This class stores a GLuint handle to an OpenGL shader and provides
     * RAII-style management. It can be constructed from source code (HLSL/GLSL),
     * from precompiled SPIR-V bytecode, or by loading and compiling a shader file.
     * The class inherits from polymorphic base classes for integration with the RHI.
     */
    export class OpenGLShader
        : public PolymorphicShaderBase,
          public OpenGLCommon<OpenGLShader> {
    private:
        GLuint m_impl; ///< OpenGL shader object handle.

    public:
        /**
         * @brief Construct a shader from source code.
         * @param logical_device Reference to the logical device (used for context management).
         * @param src            Source code as a string view.
         * @param stage          Shader stage (vertex, pixel, etc.).
         * @param language       Shader language (HLSL or GLSL).
         * @param option         Compilation options (entry point, optimizations, etc.).
         * @throws std::runtime_error if compilation fails.
         */
        OpenGLShader(
            OpenGLLogicalDevice const& logical_device,
            std::string_view src,
            ShaderStage stage,
            ShaderLanguage language,
            ShaderCompilationOption const& option
        );

        /**
         * @brief Construct a shader from precompiled SPIR-V bytecode.
         * @param logical_device Reference to the logical device.
         * @param bytes          Span of bytecode data (must be valid SPIR-V).
         * @throws std::runtime_error if bytecode is invalid or shader creation fails.
         */
        OpenGLShader(
            OpenGLLogicalDevice const& logical_device,
            std::span<std::byte const> bytes
        );

        /**
         * @brief Construct a shader by loading and compiling a file.
         * @param logical_device Reference to the logical device.
         * @param path           Filesystem path to the shader file.
         * @param option         Compilation options (used if recompilation is needed).
         * @throws std::runtime_error if file cannot be read, stage cannot be inferred,
         *                            or compilation fails.
         */
        OpenGLShader(
            OpenGLLogicalDevice const& logical_device,
            fs::path const& path,
            ShaderCompilationOption const& option
        );

		OpenGLShader(OpenGLShader&& other) noexcept;

        GLuint GetNative() const noexcept;

		~OpenGLShader() noexcept;

    };

}

#endif // !defined(__APPLE__)