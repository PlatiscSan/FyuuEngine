/* vulkan_shader.cppm */
/**
 * @file vulkan_shader.cppm
 * @brief Module interface for Vulkan shader implementation.
 *
 * This module provides the VulkanShader class, which encapsulates a Vulkan
 * shader module (vk::UniqueShaderModule). It supports creating a shader module
 * from source code (HLSL/GLSL), from precompiled SPIR-V bytecode, or by loading
 * and compiling a shader file with automatic language and stage inference.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string_view>
#include <filesystem>
#include <span>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_shader;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :enums;
import :types;
import :vulkan_common;

namespace fs = std::filesystem;

namespace fyuu_rhi::vulkan {

	/**
	 * @class VulkanShader
	 * @brief Represents a Vulkan shader module.
	 *
	 * This class stores a vk::UniqueShaderModule and provides access to it.
	 * It can be constructed from source code (HLSL/GLSL), from precompiled SPIR-V
	 * bytecode, or by loading and compiling a shader file. The class inherits
	 * from polymorphic base classes for integration with the RHI.
	 */
	export class VulkanShader
		: public PolymorphicShaderBase,
		public VulkanCommon<VulkanShader> {
	private:
		vk::UniqueShaderModule m_impl; ///< The Vulkan shader module handle.

	public:
		/**
		 * @brief Construct a shader module from source code.
		 * @param logical_device Reference to the logical device used to create the module.
		 * @param src			Source code as a string view.
		 * @param stage		  Shader stage (vertex, pixel, etc.).
		 * @param language	   Shader language (HLSL or GLSL).
		 * @param option		 Compilation options (entry point, optimizations, etc.).
		 * @throws std::runtime_error if compilation or module creation fails.
		 */
		VulkanShader(
			VulkanLogicalDevice const& logical_device,
			std::string_view src,
			ShaderStage stage,
			ShaderLanguage language,
			ShaderCompilationOption const& option
		);

		/**
		 * @brief Construct a shader module from precompiled SPIR-V bytecode.
		 * @param logical_device Reference to the logical device.
		 * @param bytes	Span of bytecode data (must be valid SPIR-V).
		 * @throws std::runtime_error if module creation fails.
		 */
		VulkanShader(
			VulkanLogicalDevice const& logical_device,
			std::span<std::byte const> bytes
		);

		/**
		 * @brief Construct a shader module by loading and compiling a file.
		 * @param logical_device Reference to the logical device.
		 * @param path	Filesystem path to the shader file.
		 * @param option Compilation options (used if recompilation is needed).
		 * @throws std::runtime_error if file cannot be read, stage cannot be inferred, or compilation/module creation fails.
		 */
		VulkanShader(
			VulkanLogicalDevice const& logical_device,
			fs::path const& path,
			ShaderCompilationOption const& option
		);

		vk::ShaderModule GetNative() const noexcept;

	};
	
} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)