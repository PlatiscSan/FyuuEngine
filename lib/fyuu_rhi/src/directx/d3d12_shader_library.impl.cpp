module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <directx/d3dx12.h>
#include <DescriptorHeap.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>

#include "declare_pool.h"

module fyuu_rhi:d3d12_shader_library;
import plastic.other;
import :glslang;
import :boost_locale;

namespace fyuu_rhi::d3d12 {


	/*
	*	global dxc instance
	*/

	static Microsoft::WRL::ComPtr<IDxcUtils> s_dxc_utils;
	static Microsoft::WRL::ComPtr<IDxcCompiler3> s_dxc_compiler;
	static Microsoft::WRL::ComPtr<IDxcIncludeHandler> s_include_handler;

	/// @brief glslang compiler resources
	static TBuiltInResource s_glslang_resources;

	static void InitializeDXC() {
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

	static std::pmr::wstring GetShaderModelString(ShaderStage stage, std::uint32_t shader_model, std::pmr::polymorphic_allocator<wchar_t> alloc) {
		
		std::pmr::wstring model(alloc);
		model.reserve(10);
	
		switch (stage) {
		case ShaderStage::Vertex:         model.append(L"vs"); break;
		case ShaderStage::Pixel:          model.append(L"ps"); break;
		case ShaderStage::Compute:        model.append(L"cs"); break;
		case ShaderStage::Geometry:       model.append(L"gs"); break;
		case ShaderStage::Amplification:  model.append(L"as"); break;
		case ShaderStage::Mesh:           model.append(L"ms"); break;
		case ShaderStage::RayGeneration:  model.append(L"lib"); break;
		case ShaderStage::RayMiss:        model.append(L"lib"); break;
		case ShaderStage::RayClosestHit:  model.append(L"lib"); break;
		case ShaderStage::RayAnyHit:      model.append(L"lib"); break;
		case ShaderStage::RayIntersection: model.append(L"lib"); break;
		case ShaderStage::Callable:       model.append(L"lib"); break;
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

	static Microsoft::WRL::ComPtr<ID3DBlob> CreateBlob(std::span<std::byte const> compiled_bytes) {

		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		D3DCreateBlob(static_cast<SIZE_T>(compiled_bytes.size()), &blob);
		std::memcpy(blob->GetBufferPointer(), compiled_bytes.data(), compiled_bytes.size());

		return blob;

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileHLSLShaderFromSource(
		std::string_view src, 
		ShaderStage shader_stage, 
		CompilationOptions const& options
	) {
		
		InitializeDXC();
		common::InitializeBoostLocale();

		/*
		*	stack pool and allocator for conversion from UTF-8 to UTF-16
		*/

		DECLARE_POOL(wchar_t, wchar_t, 8192u)

		/*
		*	container buffer
		*/

		DECLARE_POOL(param_container, std::pmr::wstring, 64u)
		std::pmr::vector<std::pmr::wstring> param_container(param_container_alloc);

		/*
		*	load source
		*/

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> source_blob;
		s_dxc_utils->CreateBlob(src.data(), static_cast<UINT32>(src.size()), CP_UTF8, &source_blob);

		/*
		*  set arguments
		*/

		std::vector<wchar_t const*> arguments;

		// entry point

		std::pmr::wstring entry_point = common::UTF8ToUTF16(options.entry_point, wchar_t_alloc);

		arguments.push_back(L"-E");
		arguments.push_back(entry_point.data());

		// target shader model

		std::uint32_t shader_model = 60u;

		if (options.hlsl_options.test(static_cast<std::size_t>(HLSLOptions::HLSLOptionsEnable16BitType))) {
			shader_model = (std::max)(shader_model, 62u);
			arguments.emplace_back(L"-enable-16bit-types");
		}

		bool is_ray_tracing =
			shader_stage == ShaderStage::RayAnyHit ||
			shader_stage == ShaderStage::RayClosestHit ||
			shader_stage == ShaderStage::RayGeneration ||
			shader_stage == ShaderStage::RayIntersection ||
			shader_stage == ShaderStage::RayMiss;

		shader_model = is_ray_tracing ?
			(std::max)(shader_model, 66u) :
			shader_model;

		std::pmr::wstring target_profile = GetShaderModelString(shader_stage, 60, wchar_t_alloc);
		arguments.emplace_back(L"-T");
		arguments.emplace_back(target_profile.data());

		// optimization

#if defined(_NDEBUG)
		switch (options->dxc_optimization) {
		case OptimizationLevel::O1:
			arguments.emplace_back(L"-O1");
			break;
		case OptimizationLevel::O2:
			arguments.emplace_back(L"-O2");
			break;
		case OptimizationLevel::O3:
			arguments.emplace_back(L"-O3");
			break;
		case OptimizationLevel::Size:
			arguments.emplace_back(L"-Os");
			break;
		}
#endif // defined(_NDEBUG)

		// debug info

#if !defined(_NDEBUG)
		arguments.emplace_back(L"-Od");
		arguments.emplace_back(L"-Zi");
#else
		arguments.emplace_back(L"-Zs");
#endif // !defined(_NDEBUG)
		
		// macros

		for (auto const& macro : options.macros) {	
			std::pmr::wstring& define = param_container.emplace_back(common::UTF8ToUTF16(macro.name.data(), wchar_t_alloc));
			std::pmr::wstring value = common::UTF8ToUTF16(macro.value.data(), wchar_t_alloc);
			define += L'=';
			define.append(std::move(value));
		}

		//// include path

		//std::wstring include_str = L"-I";

		//for (auto const& include : options->include_paths) {
		//	include_str += include.path.wstring();
		//	arguments.push_back(_wcsdup(include_str.c_str()));
		//}

		//// warnings_ as errors
		//if (options->warnings_as_errors) {
		//	arguments.push_back(L"-WX");
		//}

		//// enable 16bit types and row major
		//if (options->hlsl_options.enable_16bit_types) {
		//	arguments.push_back(L"-enable-16bit-types");
		//}
		//if (options->hlsl_options.pack_matrix_row_major) {
		//	arguments.push_back(L"-Zpr");
		//}

		//// determinate if use DXIL format
		//if (options->dxc_options.use_dxil) {
		//	arguments.push_back(L"-Qembed_debug");
		//}

		// compile
		
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

		if (FAILED(hr)) {
			throw std::runtime_error("Failed to compile shader");
		}

		// Check error

		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
		compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			throw std::runtime_error("Shader compilation failed: " + std::string(errors->GetStringPointer()));
		}

		// get compilation result

		Microsoft::WRL::ComPtr<IDxcBlob> shader_blob;
		compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), nullptr);

		if (!shader_blob) {
			throw std::runtime_error("Failed to get shader blob");
		}

		// convert to ID3DBlob

		std::span<std::byte> span{ static_cast<std::byte*>(shader_blob->GetBufferPointer()), shader_blob->GetBufferSize() };
		return CreateBlob(span);

	}

	static ShaderStage DetermineShaderStageFromFilename(std::filesystem::path const& path) {
		std::string filename = path.filename().string();
		std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

		if (filename.find("vs_") != std::string::npos || filename.find("vertex") != std::string::npos) {
			return ShaderStage::Vertex;
		}
		else if (filename.find("ps_") != std::string::npos || filename.find("pixel") != std::string::npos ||
			filename.find("fs_") != std::string::npos || filename.find("fragment") != std::string::npos) {
			return ShaderStage::Pixel;
		}
		else if (filename.find("cs_") != std::string::npos || filename.find("compute") != std::string::npos) {
			return ShaderStage::Compute;
		}
		else if (filename.find("gs_") != std::string::npos || filename.find("geometry") != std::string::npos) {
			return ShaderStage::Geometry;
		}
		else if (filename.find("hs_") != std::string::npos || filename.find("tesscontrol") != std::string::npos) {
			return ShaderStage::TessellationControl;
		}
		else if (filename.find("ds_") != std::string::npos || filename.find("tesseval") != std::string::npos) {
			return ShaderStage::TessellationEvaluation;
		}
		else if (filename.find("as_") != std::string::npos || filename.find("amplification") != std::string::npos) {
			return ShaderStage::Amplification;
		}
		else if (filename.find("ms_") != std::string::npos || filename.find("mesh") != std::string::npos) {
			return ShaderStage::Mesh;
		}
		else if (filename.find("rgen") != std::string::npos) {
			return ShaderStage::RayGeneration;
		}
		else if (filename.find("rmiss") != std::string::npos) {
			return ShaderStage::RayMiss;
		}
		else if (filename.find("rchit") != std::string::npos) {
			return ShaderStage::RayClosestHit;
		}

		return ShaderStage::Unknown;
	}

	static Microsoft::WRL::ComPtr<ID3DBlob> PrepareForHLSLCompilation(std::filesystem::path const& path, CompilationOptions const& options) {
		
//		std::ifstream file(path, std::ios::binary);
//		if (!file) {
//			throw std::runtime_error("Failed to open shader file: " + path.string());
//		}
//
//		std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//
//		ShaderStage stage = DetermineShaderStageFromFilename(path);
//
//		if (stage == ShaderStage::Unknown) {
//			throw std::runtime_error("Unable to determine shader stage for file: " + path.string());
//		}
//
//		// enable debug info when compiling as DEBUG
//#if !defined(_NDEBUG)
//		options->debug_info = true;
//#endif
//
//		return CompileHLSLShaderFromSource(source, stage, options);

		return {};

	}

	static EShLanguage ShaderStageToEShLanguage(ShaderStage shader_stage) {
		switch (shader_stage) {
		case fyuu_rhi::ShaderStage::Callable:
			return EShLanguage::EShLangCallable;
		case fyuu_rhi::ShaderStage::RayMiss:
			return EShLanguage::EShLangMiss;
		case fyuu_rhi::ShaderStage::RayClosestHit:
			return EShLanguage::EShLangClosestHit;
		case fyuu_rhi::ShaderStage::RayAnyHit:
			return EShLanguage::EShLangAnyHit;
		case fyuu_rhi::ShaderStage::RayIntersection:
			return EShLanguage::EShLangIntersect;
		case fyuu_rhi::ShaderStage::RayGeneration:
			return EShLanguage::EShLangRayGen;
		case fyuu_rhi::ShaderStage::Mesh:
			return EShLanguage::EShLangMesh;
		case fyuu_rhi::ShaderStage::Amplification:
			return EShLanguage::EShLangTask;
		case fyuu_rhi::ShaderStage::TessellationEvaluation:
			return EShLanguage::EShLangTessEvaluation;
		case fyuu_rhi::ShaderStage::TessellationControl:
			return EShLanguage::EShLangTessControl;
		case fyuu_rhi::ShaderStage::Geometry:
			return EShLanguage::EShLangGeometry;
		case fyuu_rhi::ShaderStage::Compute:
			return EShLanguage::EShLangCompute;
		case fyuu_rhi::ShaderStage::Pixel:
			return EShLanguage::EShLangFragment;
		case fyuu_rhi::ShaderStage::Vertex:
			return EShLanguage::EShLangVertex;
		default:
			throw std::runtime_error("Unknown shader stage");
		}
	}

	static spv::ExecutionModel ShaderStageToSPVExecutionModel(ShaderStage shader_stage) {
		switch (shader_stage) {
		case fyuu_rhi::ShaderStage::Callable:
			return spv::ExecutionModel::ExecutionModelCallableKHR;
		case fyuu_rhi::ShaderStage::RayMiss:
			return spv::ExecutionModel::ExecutionModelMissKHR;
		case fyuu_rhi::ShaderStage::RayClosestHit:
			return spv::ExecutionModel::ExecutionModelClosestHitKHR;
		case fyuu_rhi::ShaderStage::RayAnyHit:
			return spv::ExecutionModel::ExecutionModelAnyHitKHR;
		case fyuu_rhi::ShaderStage::RayIntersection:
			return spv::ExecutionModel::ExecutionModelIntersectionKHR;
		case fyuu_rhi::ShaderStage::RayGeneration:
			return spv::ExecutionModel::ExecutionModelRayGenerationKHR;
		case fyuu_rhi::ShaderStage::Mesh:
			return spv::ExecutionModel::ExecutionModelMeshEXT;
		case fyuu_rhi::ShaderStage::Amplification:
			return spv::ExecutionModel::ExecutionModelTaskEXT;
		case fyuu_rhi::ShaderStage::TessellationEvaluation:
			return spv::ExecutionModel::ExecutionModelTessellationEvaluation;
		case fyuu_rhi::ShaderStage::TessellationControl:
			return spv::ExecutionModel::ExecutionModelTessellationControl;
		case fyuu_rhi::ShaderStage::Geometry:
			return spv::ExecutionModel::ExecutionModelGeometry;
		case fyuu_rhi::ShaderStage::Compute:
			return spv::ExecutionModel::ExecutionModelGLCompute;
		case fyuu_rhi::ShaderStage::Pixel:
			return spv::ExecutionModel::ExecutionModelFragment;
		case fyuu_rhi::ShaderStage::Vertex:
			return spv::ExecutionModel::ExecutionModelVertex;
		default:
			throw std::runtime_error("Unknown shader stage");
		}
	}

	static std::string CompileSPIRVToHLSL(std::span<std::uint32_t> spirv, ShaderStage shader_stage, CompilationOptions const& options) {

		spirv_cross::CompilerHLSL compiler(spirv.data(), spirv.size());

		spirv_cross::CompilerHLSL::Options spirv_cross_options;

		// minimum version for dxc compiler
		spirv_cross_options.shader_model = 60u;

		spirv_cross_options.enable_16bit_types 
			= options.hlsl_options.test(static_cast<std::size_t>(HLSLOptions::HLSLOptionsEnable16BitType));

		spirv_cross_options.shader_model =
			spirv_cross_options.enable_16bit_types ?
			(std::max)(spirv_cross_options.shader_model, 62u) :
			spirv_cross_options.shader_model;

		bool is_ray_tracing =
			shader_stage == ShaderStage::RayAnyHit ||
			shader_stage == ShaderStage::RayClosestHit ||
			shader_stage == ShaderStage::RayGeneration ||
			shader_stage == ShaderStage::RayIntersection ||
			shader_stage == ShaderStage::RayMiss;

		spirv_cross_options.shader_model = is_ray_tracing ?
			(std::max)(spirv_cross_options.shader_model, 66u) :
			spirv_cross_options.shader_model;

		compiler.set_hlsl_options(spirv_cross_options);

		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		// set resource binding

		for (auto& resource : resources.uniform_buffers) {
			std::uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			compiler.set_decoration(resource.id, spv::DecorationBinding, binding);
		}

		for (auto& resource : resources.sampled_images) {
			std::uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			compiler.set_decoration(resource.id, spv::DecorationBinding, binding);
		}

		compiler.set_entry_point(
			options.entry_point,
			ShaderStageToSPVExecutionModel(shader_stage)
		); 

		std::string source = compiler.compile();

		return source;

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileGLSLToSPIRV(std::filesystem::path const& path, ShaderStage shader_stage, CompilationOptions const& options) {
		
		//InitializeGLSLang();

		//EShLanguage glslang_shader_stage = ShaderStageToEShLanguage(shader_stage);

		//glslang::TShader shader(glslang_shader_stage);

		//std::ifstream file(path, std::ios::binary);
		//if (!file) {
		//	throw std::runtime_error("Failed to open shader file: " + path.string());
		//}
		//std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		//char const* shader_strings[] = { source.c_str() };
		//shader.setStrings(shader_strings, 1);

		//// set environment
		//shader.setEnvInput(glslang::EShSourceGlsl, glslang_shader_stage,
		//	glslang::EShClientVulkan, 100);
		//shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
		//shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

		//shader.setEntryPoint(options->entry_point.c_str());

		//// set macros
		//std::string preamble;
		//for (auto const& macro : options->defines) {
		//	preamble += "#define " + std::string(macro.name) + " " + std::string(macro.value) + "\n";
		//}
		//shader.setPreamble(preamble.c_str());

		//// set version of GLSL
		//shader.setStringsWithLengths(nullptr, nullptr, 1);
		//shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

		//EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		//if (!shader.parse(&s_glslang_resources, 100, false, messages)) {
		//	throw std::runtime_error("Failed to parse shader: " + std::string(shader.getInfoLog()));
		//}

		//glslang::TProgram program;
		//program.addShader(&shader);

		//if (!program.link(messages)) {
		//	throw std::runtime_error("Failed to link shader: " + std::string(program.getInfoLog()));
		//}

		//glslang::SpvOptions spv_options;
		//spv_options.generateDebugInfo = options->debug_info;
		//spv_options.stripDebugInfo = options->strip_debug;
		//spv_options.disableOptimizer = options->optimization == OptimizationLevel::None;
		//spv_options.optimizeSize = options->optimization == OptimizationLevel::Size;
		//spv_options.validate = options->validation.enable_validation;

		//std::vector<std::uint32_t> spirv;
		//glslang::GlslangToSpv(*program.getIntermediate(glslang_shader_stage), spirv, &spv_options);

		//return CompileSPIRVToHLSL(spirv, shader_stage, options);

		return {};

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> ReadFileToBlob(std::filesystem::path const& path, CompilationOptions const& options) {

		std::string ext = path.extension().string();
		Microsoft::WRL::ComPtr<ID3DBlob> shader_blob;

		if (ext == ".hlsl" || ext == ".hlsli") {
			shader_blob = PrepareForHLSLCompilation(path, options);
		}
		else if (ext == ".cso") {
			/*
			*	already compiled
			*/
			ThrowIfFailed(D3DReadFileToBlob(path.wstring().c_str(), &shader_blob));
		}
		else if (ext == ".vert") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Vertex, options);
		}
		else if (ext == ".frag") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Fragment, options);
		}
		else if (ext == ".vert") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Vertex, options);
		}
		else if (ext == ".frag") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Fragment, options);
		}
		else if (ext == ".geom") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Geometry, options);
		}
		else if (ext == ".comp") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::Compute, options);
		}
		else if (ext == ".tesc") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::TessellationControl, options);
		}
		else if (ext == ".tese") {
			shader_blob = CompileGLSLToSPIRV(path, ShaderStage::TessellationEvaluation, options);
		}
		else if (ext == ".rgen" || ext == ".rmiss" || ext == ".rchit" || ext == ".rahit" || ext == ".rint") {
			ShaderStage stage = DetermineShaderStageFromFilename(path);
			shader_blob = CompileGLSLToSPIRV(path, stage, options);
		}
		else {
			throw std::runtime_error("Unsupported shader file extension: " + ext);
		}

		return shader_blob;

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileGLSLShaderFromSource(
		std::string_view src, 
		ShaderStage shader_stage, 
		CompilationOptions const& options
	) {
		
		std::vector<std::uint32_t> spirv = common::CompileGLSLToSPIRV(src, shader_stage, options);
		std::string hlsl_src = CompileSPIRVToHLSL(spirv, shader_stage, options);
		Microsoft::WRL::ComPtr<ID3DBlob> shader = CompileHLSLShaderFromSource(hlsl_src, shader_stage, options);

		return shader;

	}

	static ShaderReflection Reflect(Microsoft::WRL::ComPtr<ID3DBlob> const& shader_blob) {
		
		InitializeDXC();

		DxcBuffer reflection_data = {
			.Ptr = shader_blob->GetBufferPointer(),
			.Size = shader_blob->GetBufferSize(),
			.Encoding = DXC_CP_ACP,
		};

		ShaderReflection reflection;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> d3d12_reflection;

		if (SUCCEEDED(s_dxc_utils->CreateReflection(&reflection_data, IID_PPV_ARGS(&d3d12_reflection)))) {
			D3D12_SHADER_DESC desc;
			d3d12_reflection->GetDesc(&desc);

			reflection.entry_point = desc.Creator;
			//reflection.stage = m_stage;

			
			for (UINT i = 0; i < desc.BoundResources; i++) {
				D3D12_SHADER_INPUT_BIND_DESC bind_desc;
				d3d12_reflection->GetResourceBindingDesc(i, &bind_desc);

				ShaderResourceBinding resource;
				resource.name = bind_desc.Name;
				resource.bind_point = bind_desc.BindPoint;
				resource.bind_count = bind_desc.BindCount;

				bool writable = false;
				switch (bind_desc.Type) {
				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				case D3D_SIT_UAV_APPEND_STRUCTURED:
				case D3D_SIT_UAV_CONSUME_STRUCTURED:
				case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				case D3D_SIT_UAV_FEEDBACKTEXTURE:
					writable = true;
					resource.type = ShaderResourceBinding::Type::UAV;
					break;
				case D3D_SIT_CBUFFER:
					resource.type = ShaderResourceBinding::Type::ConstantBuffer;
					break;
				case D3D_SIT_TBUFFER:
					resource.type = ShaderResourceBinding::Type::Texture;
					break;
				case D3D_SIT_TEXTURE:
					resource.type = ShaderResourceBinding::Type::Texture;
					break;
				case D3D_SIT_SAMPLER:
					resource.type = ShaderResourceBinding::Type::Sampler;
					break;
				case D3D_SIT_STRUCTURED:
					resource.type = ShaderResourceBinding::Type::StructuredBuffer;
					break;
				case D3D_SIT_BYTEADDRESS:
					resource.type = ShaderResourceBinding::Type::ByteAddressBuffer;
					break;
				default:
					resource.type = ShaderResourceBinding::Type::Texture;
					break;
				}

				resource.is_writable = writable;
				//resource.visibility = m_stage;

				reflection.resources.emplace_back(std::move(resource));
				reflection.bindings[resource.name] = resource.bind_point;

			}
		}
		
		return reflection;

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> Compile(
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		CompilationOptions const& options
	) {
		switch (language) {
		case ShaderLanguage::HLSL:
			return CompileHLSLShaderFromSource(src, shader_stage, options);
		case ShaderLanguage::GLSL:
			return CompileGLSLShaderFromSource(src, shader_stage, options);
		default:
			throw std::runtime_error("The shader is not supported current");
		}
	}

	D3D12ShaderLibrary::D3D12ShaderLibrary(std::filesystem::path const& path, plastic::utility::AnyPointer<CompilationOptions> const& options)
		: m_shader_blob(ReadFileToBlob(path, *options)),
		m_reflection(Reflect(m_shader_blob)) {

	}

	D3D12ShaderLibrary::D3D12ShaderLibrary(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		plastic::utility::AnyPointer<CompilationOptions> const& options
	) : m_shader_blob(Compile(src, shader_stage, language, *options)),
		m_reflection(Reflect(m_shader_blob)) {
	}

	D3D12ShaderLibrary::D3D12ShaderLibrary(
		D3D12LogicalDevice const& logical_device,
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		CompilationOptions const& options
	) : m_shader_blob(Compile(src, shader_stage, language, options)),
		m_reflection(Reflect(m_shader_blob)) {
	}

	D3D12ShaderLibrary::D3D12ShaderLibrary(std::span<std::byte const> compiled_bytes) 
		: m_shader_blob(CreateBlob(compiled_bytes)),
		m_reflection(Reflect(m_shader_blob)) {
	}

}
