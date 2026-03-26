/* vulkan_shader.impl.cpp */
/**
 * @file vulkan_shader.impl.cpp
 * @brief Implementation of VulkanShader and associated compilation utilities.
 *
 * This file contains the logic for compiling HLSL/GLSL to SPIR-V using glslang,
 * and for loading files with automatic language/stage inference. It mirrors the
 * structure of the D3D12 shader implementation but produces Vulkan shader modules.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <iostream>
#include <fstream>
#include <string_view>
#include <filesystem>
#include <span>
#include <cstring>
#include <algorithm> 
#include <format>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:vulkan_shader_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_shader;
import vulkan;
import :enums;
import :types;
import :vulkan_logical_device;
import :glslang;
import :shader_utility;

namespace fs = std::filesystem;

namespace {

    using namespace fyuu_rhi;
    using namespace fyuu_rhi::vulkan;

    // ------------------------------------------------------------------------
    // Compilation functions (return Vulkan shader modules)
    // ------------------------------------------------------------------------

    /**
     * @brief Compile HLSL source to SPIR-V and create a Vulkan shader module.
     * @param logical_device Vulkan device used to create the module.
     * @param src            HLSL source string.
     * @param stage          Shader stage.
     * @param option         Compilation options.
     * @return Unique handle to the created shader module.
     */
    vk::UniqueShaderModule CompileHLSL(
        VulkanLogicalDevice const& logical_device,
        std::string_view src,
        ShaderStage stage,
        ShaderCompilationOption const& option
    ) {
        std::vector<std::uint32_t> spirv = CompileHLSLToSPIRV(src, stage, option);
        vk::ShaderModuleCreateInfo info(
            {},                              // flags
            spirv.size(),                    // codeSize (in uint32_t units)
            spirv.data()                     // pCode
        );
        return logical_device.CreateShader(info);
    }

    /**
     * @brief Compile GLSL source to SPIR-V and create a Vulkan shader module.
     * @param logical_device Vulkan device.
     * @param src            GLSL source string.
     * @param stage          Shader stage.
     * @param option         Compilation options.
     * @return Unique handle to the created shader module.
     */
    vk::UniqueShaderModule CompileGLSL(
        VulkanLogicalDevice const& logical_device,
        std::string_view src,
        ShaderStage stage,
        ShaderCompilationOption const& option
    ) {
        std::vector<std::uint32_t> spirv = CompileGLSLToSPIRV(src, stage, option);
        vk::ShaderModuleCreateInfo info(
            {},
            spirv.size() * sizeof(std::uint32_t),
            spirv.data()
        );
        return logical_device.CreateShader(info);
    }

    /**
     * @brief Dispatch compilation based on source language.
     * @param logical_device Vulkan device.
     * @param src            Source code.
     * @param stage          Shader stage.
     * @param language       Shader language (HLSL or GLSL).
     * @param option         Compilation options.
     * @return Unique handle to the created shader module.
     * @throws std::invalid_argument if language is not supported.
     */
    vk::UniqueShaderModule Compile(
        VulkanLogicalDevice const& logical_device,
        std::string_view src,
        ShaderStage stage,
        ShaderLanguage language,
        ShaderCompilationOption const& option
    ) {
        switch (language) {
        case ShaderLanguage::HLSL:
            return CompileHLSL(logical_device, src, stage, option);
        case ShaderLanguage::GLSL:
            return CompileGLSL(logical_device, src, stage, option);
        default:
            throw std::invalid_argument("Not supported shader language for Vulkan");
        }
    }

    /**
     * @brief Read a shader from a file, auto‑detecting format and performing necessary compilation.
     * @param logical_device Vulkan device used to create the module.
     * @param path           Filesystem path.
     * @param option         Compilation options (used if recompilation is needed).
     * @return Unique handle to the created shader module.
     * @throws std::runtime_error on failure (unknown format, stage not inferred, compilation errors).
     */
    vk::UniqueShaderModule ReadFrom(
        VulkanLogicalDevice const& logical_device,
        fs::path const& path,
        ShaderCompilationOption const& option
    ) {
        // Open the file and read its contents into a vector.
        // We cannot rely on a LoadFile helper like in D3D12, so we use standard file I/O.
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

        // Examine the first few bytes to detect file type.
        bool is_spirv = false;
        if (buffer.size() >= 4) {
            std::uint32_t magic;
            std::memcpy(&magic, buffer.data(), 4);
            // SPIR-V magic number (little-endian)
            is_spirv = (magic == 0x07230203);
        }

        // Case 1: File contains precompiled SPIR-V bytecode.
        if (is_spirv) {
            vk::ShaderModuleCreateInfo info(
                {},
                buffer.size() / sizeof(std::uint32_t),
                reinterpret_cast<std::uint32_t const*>(buffer.data())
            );
            return logical_device.CreateShader(info);
        }

        // Case 2: File contains source code – we need to compile it.
        // Determine language and stage from the file name/extension.
        std::string ext = path.extension().string();
        ShaderLanguage language = ShaderLanguage::HLSL; // default
        ShaderStage stage = ShaderStage::Unknown;

        if (ext == ".hlsl" || ext == ".hlsli" || ext == ".fx") {
            language = ShaderLanguage::HLSL;
            stage = InferStageFromHLSLFilename(path);
        } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" ||
                   ext == ".geom" || ext == ".comp" || ext == ".tesc" || ext == ".tese") {
            language = ShaderLanguage::GLSL;
            stage = InferStageFromExtension(path);
        } else {
            throw std::runtime_error(std::format(
                "Cannot infer shader language from file extension: {}", ext));
        }

        if (stage == ShaderStage::Unknown) {
            throw std::runtime_error(
                "Could not determine shader stage from file name or extension. "
                "The name must contain a stage keyword (e.g., 'vertex', 'pixel') for HLSL "
                "or use a stage-specific extension (e.g., .vert, .frag) for GLSL.");
        }

        // Remove UTF-8 BOM if present (common in text files)
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

        // Compile to SPIR-V and create module
        return Compile(logical_device, source, stage, language, option);

    }

} // unnamed namespace

namespace fyuu_rhi::vulkan {

    // ------------------------------------------------------------------------
    // VulkanShader constructors
    // ------------------------------------------------------------------------

    VulkanShader::VulkanShader(
        VulkanLogicalDevice const& logical_device,
        std::string_view src,
        ShaderStage stage,
        ShaderLanguage language,
        ShaderCompilationOption const& option
    ) : PolymorphicShaderBase(this),
        VulkanCommon(this),
        m_impl(Compile(logical_device, src, stage, language, option)) {
        // Compilation and module creation done in Compile.
    }

    VulkanShader::VulkanShader(
        VulkanLogicalDevice const& logical_device,
        std::span<std::byte const> bytes
    ) : PolymorphicShaderBase(this),
        VulkanCommon(this),
        m_impl([&logical_device, bytes]() {
            vk::ShaderModuleCreateInfo info(
                {},
                bytes.size() / sizeof(std::uint32_t),
                reinterpret_cast<std::uint32_t const*>(bytes.data())
            );
            return logical_device.CreateShader(info);
        }()) {
        // Direct module creation from byte span.
    }

    VulkanShader::VulkanShader(
        VulkanLogicalDevice const& logical_device,
        fs::path const& path,
        ShaderCompilationOption const& option
    ) : PolymorphicShaderBase(this),
        VulkanCommon(this),
        m_impl(ReadFrom(logical_device, path, option)) {
        // File read, inference, compilation, and module creation performed in ReadFrom.
    }

	vk::ShaderModule VulkanShader::GetNative() const noexcept {
		return *m_impl;
	}

} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)