/* d3d12_shader.impl.cpp */
/**
 * @file d3d12_shader.impl.cpp
 * @brief Implementation of D3D12Shader and associated compilation utilities.
 * 
 * This file contains the actual compilation logic using DXC, SPIRV-Cross,
 * and GLSLang. It handles HLSL and GLSL source compilation, SPIR-V cross‑compilation
 * to HLSL, and file loading with automatic stage/language inference.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <string_view>
#include <filesystem>
#include <format>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>
#include "declare_pool.hpp"
module fyuu_rhi:d3d12_shader_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_shader;
import plastic.other;
import :enums;
import :types;
import :d3d12_common;
import :boost_locale;
import :glslang;
import :d3d12_throw;
import :shader_utility;

namespace fs = std::filesystem;

namespace {
	using namespace fyuu_rhi;
	using namespace fyuu_rhi::d3d12;
	
	// Global DXC instances (initialized once)
	Microsoft::WRL::ComPtr<IDxcUtils> s_dxc_utils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> s_dxc_compiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> s_include_handler;

	/**
	 * @brief Initialize the global DXC components if not already done.
	 * 
	 * Uses plastic::utility::InitializeGlobalInstance to ensure thread‑safe
	 * one‑time initialization. Throws on failure.
	 */
	void InitializeDXC() {
		plastic::utility::InitializeGlobalInstance(
			[]() {
				if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_dxc_utils)))) {
					throw std::runtime_error("Failed to create DXC Utils");
				}
				if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_dxc_compiler)))) {
					throw std::runtime_error("Failed to create DXC compiler");
				}
				if (FAILED(s_dxc_utils->CreateDefaultIncludeHandler(&s_include_handler))) {
					throw std::runtime_error("Failed to create default include handler");
				}
			}
		);
	}

	/**
	 * @brief Build the shader model string for DXC (e.g., "vs_6_0").
	 * @param stage        Shader stage.
	 * @param shader_model Numerical shader model (e.g., 60 for 6.0).
	 * @param alloc        Polymorphic allocator for wide string.
	 * @return Wide string suitable for DXC's -T argument.
	 */
	std::pmr::wstring GetShaderModelString(ShaderStage stage, std::uint32_t shader_model, std::pmr::polymorphic_allocator<wchar_t> alloc) {
		std::pmr::wstring model(alloc);
		model.reserve(10);
	
		switch (stage) {
		case ShaderStage::Vertex:		 model.append(L"vs"); break;
		case ShaderStage::Pixel:		  model.append(L"ps"); break;
		case ShaderStage::Compute:		model.append(L"cs"); break;
		case ShaderStage::Geometry:	   model.append(L"gs"); break;
		case ShaderStage::Amplification:  model.append(L"as"); break;
		case ShaderStage::Mesh:		   model.append(L"ms"); break;
		case ShaderStage::RayGeneration:  model.append(L"lib"); break;
		case ShaderStage::RayMiss:		model.append(L"lib"); break;
		case ShaderStage::RayClosestHit:  model.append(L"lib"); break;
		case ShaderStage::RayAnyHit:	  model.append(L"lib"); break;
		case ShaderStage::RayIntersection: model.append(L"lib"); break;
		case ShaderStage::Callable:	   model.append(L"lib"); break;
		default: throw std::runtime_error("Unsupported shader stage");
		}

		wchar_t tens_digit = static_cast<wchar_t>((shader_model / 10) + '0');
		wchar_t ones_digit = static_cast<wchar_t>((shader_model % 10) + '0');

		model += L'_';
		model += tens_digit;
		model += L'_';
		model += ones_digit;

		return model;
	}

	/**
	 * @brief Compile HLSL source code using DXC.
	 * @param src    HLSL source string.
	 * @param stage  Shader stage.
	 * @param option Compilation options.
	 * @return Compiled shader blob (IDxcBlob).
	 * @throws std::runtime_error on compilation errors.
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> CompileHLSL(		
		std::string_view src,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		InitializeDXC();
		InitializeBoostLocale();

		// Use stack‑allocated pools to avoid dynamic allocations for arguments.

		DECLARE_POOL(utf16, wchar_t, 8192u)

		SmallVector<wchar_t const*, 32u> arguments;

		// Create blob from source
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> source_blob;
		s_dxc_utils->CreateBlob(src.data(), static_cast<UINT32>(src.size()), CP_UTF8, &source_blob);
		
		// Entry point
		std::pmr::wstring entry_point = UTF8ToUTF16(option.entry_point, utf16_alloc);
		arguments.push_back(L"-E");
		arguments.push_back(entry_point.data());

		// Determine shader model based on options and stage
		std::uint32_t shader_model = 60u;
		if ((option.hlsl_option & HLSLOption::Enable16BitType) != HLSLOption::None) {
			shader_model = (std::max)(shader_model, 62u);
			arguments.emplace_back(L"-enable-16bit-types");
		}
		shader_model = (stage & ShaderStage::RayTracing) != ShaderStage::Unknown ?
			(std::max)(shader_model, 66u) : shader_model;

		std::pmr::wstring target_profile = GetShaderModelString(stage, shader_model, utf16_alloc);
		arguments.emplace_back(L"-T");
		arguments.emplace_back(target_profile.data());

		// Optimization and debug flags
#if defined(_NDEBUG)
		switch (option.optimization) {
		case OptimizationLevel::O1:  arguments.emplace_back(L"-O1"); break;
		case OptimizationLevel::O2:  arguments.emplace_back(L"-O2"); break;
		case OptimizationLevel::O3:  arguments.emplace_back(L"-O3"); break;
		case OptimizationLevel::Size: arguments.emplace_back(L"-Os"); break;
		default:                     arguments.emplace_back(L"-O1"); break;
		}
		arguments.emplace_back(L"-Zs"); // Enable debug information
#else
		arguments.emplace_back(L"-Od"); // Disable optimizations
		arguments.emplace_back(L"-Zi"); // Generate debug info
#endif // defined(_NDEBUG)

		DxcBuffer source_buffer = {
			.Ptr = source_blob->GetBufferPointer(),
			.Size = source_blob->GetBufferSize(),
			.Encoding = DXC_CP_UTF8
		};

		Microsoft::WRL::ComPtr<IDxcResult> compile_result;
		HRESULT hr = s_dxc_compiler->Compile(
			&source_buffer,
			arguments.data(),
			static_cast<UINT32>(arguments.size()),
			s_include_handler.Get(),
			IID_PPV_ARGS(&compile_result)
		);
		ThrowIfFailed(hr);

		// Check for compilation errors
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
		compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			throw std::runtime_error(std::format("Shader compilation failed: {}", errors->GetStringPointer()));
		}
		
		// Extract the compiled shader blob
		Microsoft::WRL::ComPtr<IDxcBlob> shader_blob;
		compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), nullptr);
		if (!shader_blob) {
			throw std::runtime_error("Failed to get shader blob");
		}
		return shader_blob;
	}

	/**
	 * @brief Cross‑compile SPIR-V bytecode to HLSL using SPIRV-Cross, then compile with DXC.
	 * @param spirv  Span of SPIR-V words.
	 * @param stage  Shader stage.
	 * @param option Compilation options (controls shader model, 16‑bit types, etc.).
	 * @return Compiled HLSL blob.
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> CompileSPIRV(
		std::span<std::uint32_t const> spirv,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		// Create SPIRV-Cross HLSL compiler
		spirv_cross::CompilerHLSL compiler(spirv.data(), spirv.size());
		spirv_cross::CompilerHLSL::Options spirv_cross_options;

		// Set HLSL options
		spirv_cross_options.shader_model = 60u; // minimum
		spirv_cross_options.enable_16bit_types = (option.hlsl_option & HLSLOption::Enable16BitType) != HLSLOption::None;
		spirv_cross_options.shader_model =
			spirv_cross_options.enable_16bit_types ?
			(std::max)(spirv_cross_options.shader_model, 62u) : spirv_cross_options.shader_model;
		spirv_cross_options.shader_model = (stage & ShaderStage::RayTracing) != ShaderStage::Unknown ?
			(std::max)(spirv_cross_options.shader_model, 66u) : spirv_cross_options.shader_model;
		compiler.set_hlsl_options(spirv_cross_options);

		// Ensure resource bindings are preserved
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();
		for (auto& resource : resources.uniform_buffers) {
			std::uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			compiler.set_decoration(resource.id, spv::DecorationBinding, binding);
		}
		for (auto& resource : resources.sampled_images) {
			std::uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			compiler.set_decoration(resource.id, spv::DecorationBinding, binding);
		}

		// Set entry point
		compiler.set_entry_point(option.entry_point, ShaderStageToSPVExecutionModel(stage));

		// Compile SPIR-V to HLSL source
		std::string source = compiler.compile();

		// Finally compile the HLSL with DXC
		return CompileHLSL(source, stage, option);
	}

	/**
	 * @brief Compile GLSL source to HLSL via SPIR-V intermediate.
	 * @param src    GLSL source string.
	 * @param stage  Shader stage.
	 * @param option Compilation options.
	 * @return Compiled HLSL blob.
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> CompileGLSL(
		std::string_view src,
		ShaderStage stage,
		ShaderCompilationOption const& option
	) {
		// First compile GLSL to SPIR-V using glslang (provided by :glslang module)
		std::vector<std::uint32_t> spirv = CompileGLSLToSPIRV(src, stage, option);
		// Then cross‑compile to HLSL
		return CompileSPIRV(spirv, stage, option);
	}

	/**
	 * @brief Dispatch compilation based on source language.
	 * @param src      Source code.
	 * @param stage    Shader stage.
	 * @param language Shader language (HLSL or GLSL).
	 * @param option   Compilation options.
	 * @return Compiled shader blob.
	 * @throws std::invalid_argument if language is not supported.
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> Compile(		
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
			throw std::invalid_argument("Not supported shader language for DirectX12");
		}
	}

	/**
	 * @brief Create a blob from precompiled bytecode without recompiling.
	 * @param bytes Span of bytecode data.
	 * @return IDxcBlob containing the bytecode.
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> Load(
		std::span<std::byte const> bytes
	) {
		InitializeDXC();
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob_encoding;
		ThrowIfFailed(
			s_dxc_utils->CreateBlob(
				bytes.data(),
				static_cast<UINT32>(bytes.size()),
				DXC_CP_ACP,
				&blob_encoding
			)
		);
		return blob_encoding;
	}

	/**
	 * @brief Read a shader from a file, auto‑detecting format and performing necessary compilation.
	 * @param path   Filesystem path.
	 * @param option Compilation options (used if recompilation is needed).
	 * @return Compiled shader blob.
	 * @throws std::runtime_error on failure (unknown format, stage not inferred, compilation errors).
	 */
	Microsoft::WRL::ComPtr<IDxcBlob> ReadFrom(fs::path const& path, ShaderCompilationOption const& option) {
		InitializeDXC();
		InitializeBoostLocale(); 

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> file_blob;
		ThrowIfFailed(s_dxc_utils->LoadFile(path.c_str(), nullptr, &file_blob));

		auto data = static_cast<std::byte const*>(file_blob->GetBufferPointer());
		std::size_t size = file_blob->GetBufferSize();
		bool is_hlsl_bytes = false;
		bool is_spirv_bytes = false;
		if (size >= 4u) {
			std::uint32_t magic;
			std::memcpy(&magic, data, 4u);
			is_hlsl_bytes = magic == 0x43425844;  // DXBC magic number
			is_spirv_bytes = magic == 0x07230203; // SPIR-V magic number
		}
		
		// Case 1: Already compiled HLSL bytecode (DXBC)
		if (is_hlsl_bytes) {
			return file_blob;
		}

		// Case 2: SPIR-V bytecode → cross‑compile to HLSL
		if (is_spirv_bytes) {
			std::span<std::uint32_t const> spirv_bytes(reinterpret_cast<std::uint32_t const*>(data), size / sizeof(std::uint32_t));
			ShaderStage stage = InferStageFromHLSLFilename(path);
			if (stage == ShaderStage::Unknown) {
				throw std::runtime_error(
					"Cannot determine shader stage for SPIR-V file. "
					"Please name the file with a stage-specific extension (e.g., .vert.spv) "
					"or use the explicit compilation API.");
			}
			return CompileSPIRV(spirv_bytes, stage, option);
		}

		// Case 3: Source file – determine language and stage from extension/name
		std::string ext = path.extension().string();
        ShaderLanguage language = ShaderLanguage::HLSL;
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
				"The name must contains a stage keyword (e.g., 'vertex', 'pixel') for HLSL "
				"or use a stage-specific extension (e.g., .vert, .frag) for GLSL.");
		}

		// Remove UTF-8 BOM if present
        auto src_ptr = reinterpret_cast<char const*>(data);
        std::size_t src_size = size;
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

namespace fyuu_rhi::d3d12 {

	// ------------------------------------------------------------------------
	// D3D12Shader constructors
	// ------------------------------------------------------------------------

	D3D12Shader::D3D12Shader(
		D3D12LogicalDevice const& logical_device,
		std::string_view src,
		ShaderStage stage,
		ShaderLanguage language,
		ShaderCompilationOption const& option
	) : PolymorphicShaderBase(this),
		D3D12Common(this),
		m_impl(Compile(src, stage, language, option)) {
		// Compilation done in initializer list.
	}

	D3D12Shader::D3D12Shader(
		D3D12LogicalDevice const& logical_device,
		std::span<std::byte const> bytes
	) : PolymorphicShaderBase(this),
		D3D12Common(this),
		m_impl(Load(bytes)) {
		// Loaded bytecode as blob.
	}

	D3D12Shader::D3D12Shader(
		D3D12LogicalDevice const& logical_device,
		fs::path const& path,
		ShaderCompilationOption const& option
	) : PolymorphicShaderBase(this),
		D3D12Common(this),
		m_impl(ReadFrom(path, option)) {
		// File read and compilation done in ReadFrom.
	}

	// ------------------------------------------------------------------------
	// Accessors
	// ------------------------------------------------------------------------

	Microsoft::WRL::ComPtr<IDxcBlob> D3D12Shader::GetNative() const noexcept {
		return m_impl;
	}

	IDxcBlob* D3D12Shader::GetRawNative() const noexcept {
		return m_impl.Get();
	}

} // namespace fyuu_rhi::d3d12