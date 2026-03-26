/* d3d12_shader.cppm */
/**
 * @file d3d12_shader.cppm
 * @brief Module interface for D3D12 shader implementation.
 * 
 * This module provides the D3D12Shader class, which encapsulates a compiled shader
 * for Direct3D 12 using the DXC compiler. It supports compiling from source (HLSL/GLSL),
 * loading from bytecode, and inferring shader stage from file names.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string_view>
#include <filesystem>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <d3dcompiler.h>
#include <dxcapi.h>
export module fyuu_rhi:d3d12_shader;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;
import :types;
import :d3d12_common;

namespace fs = std::filesystem;

namespace fyuu_rhi::d3d12 {
	/**
	 * @class D3D12Shader
	 * @brief Represents a compiled shader for Direct3D 12.
	 * 
	 * This class stores a compiled shader blob (IDxcBlob) and provides access to it.
	 * It can be constructed from source code (HLSL/GLSL), from precompiled bytecode,
	 * or by loading and compiling a shader file. The class inherits from polymorphic
	 * base classes for integration with the RHI.
	 */
	export class D3D12Shader
		: public PolymorphicShaderBase,
		public D3D12Common<D3D12Shader> {
	private:
		Microsoft::WRL::ComPtr<IDxcBlob> m_impl; ///< The compiled shader blob.

	public:
		/**
		 * @brief Construct a shader from source code.
		 * @param logical_device Reference to the logical device (unused but required for consistency).
		 * @param src            Source code as a string view.
		 * @param stage          Shader stage (vertex, pixel, etc.).
		 * @param language       Shader language (HLSL or GLSL).
		 * @param option         Compilation options (entry point, optimizations, etc.).
		 * @throws std::runtime_error if compilation fails.
		 */
		D3D12Shader(
			D3D12LogicalDevice const& logical_device,
			std::string_view src,
			ShaderStage stage,
			ShaderLanguage language,
			ShaderCompilationOption const& option
		);

		/**
		 * @brief Construct a shader from precompiled bytecode.
		 * @param logical_device Reference to the logical device (unused).
		 * @param bytes          Span of bytecode data.
		 */
		D3D12Shader(
			D3D12LogicalDevice const& logical_device,
			std::span<std::byte const> bytes
		);

		/**
		 * @brief Construct a shader by loading and compiling a file.
		 * @param logical_device Reference to the logical device (unused).
		 * @param path           Filesystem path to the shader file.
		 * @param option         Compilation options.
		 * @throws std::runtime_error if file cannot be read or compilation fails.
		 */
		D3D12Shader(
			D3D12LogicalDevice const& logical_device,
			fs::path const& path,
			ShaderCompilationOption const& option
		);

		/**
		 * @brief Get the native shader blob as a COM smart pointer.
		 * @return ComPtr<IDxcBlob> containing the compiled shader.
		 */
		Microsoft::WRL::ComPtr<IDxcBlob> GetNative() const noexcept;

		/**
		 * @brief Get the raw pointer to the native shader blob.
		 * @return Raw IDxcBlob* pointer (do not release manually).
		 */
		IDxcBlob* GetRawNative() const noexcept;
	};
}