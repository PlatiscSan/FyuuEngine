module;
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include "declare_pool.h"

module fyuu_rhi:glslang;
import std;
import plastic.other;

/// @brief glslang compiler resources
static TBuiltInResource s_glslang_resources;

static TBuiltInResource InitGLSLangResources() {

	TBuiltInResource resource;

	resource.maxLights = 32;
	resource.maxClipPlanes = 6;
	resource.maxTextureUnits = 32;
	resource.maxTextureCoords = 32;
	resource.maxVertexAttribs = 64;
	resource.maxVertexUniformComponents = 4096;
	resource.maxVaryingFloats = 64;
	resource.maxVertexTextureImageUnits = 32;
	resource.maxCombinedTextureImageUnits = 80;
	resource.maxTextureImageUnits = 32;
	resource.maxFragmentUniformComponents = 4096;
	resource.maxDrawBuffers = 32;
	resource.maxVertexUniformVectors = 128;
	resource.maxVaryingVectors = 8;
	resource.maxFragmentUniformVectors = 16;
	resource.maxVertexOutputVectors = 16;
	resource.maxFragmentInputVectors = 15;
	resource.minProgramTexelOffset = -8;
	resource.maxProgramTexelOffset = 7;
	resource.maxClipDistances = 8;
	resource.maxComputeWorkGroupCountX = 65535;
	resource.maxComputeWorkGroupCountY = 65535;
	resource.maxComputeWorkGroupCountZ = 65535;
	resource.maxComputeWorkGroupSizeX = 1024;
	resource.maxComputeWorkGroupSizeY = 1024;
	resource.maxComputeWorkGroupSizeZ = 64;
	resource.maxComputeUniformComponents = 1024;
	resource.maxComputeTextureImageUnits = 16;
	resource.maxComputeImageUniforms = 8;
	resource.maxComputeAtomicCounters = 8;
	resource.maxComputeAtomicCounterBuffers = 1;
	resource.maxVaryingComponents = 60;
	resource.maxVertexOutputComponents = 64;
	resource.maxGeometryInputComponents = 64;
	resource.maxGeometryOutputComponents = 128;
	resource.maxFragmentInputComponents = 128;
	resource.maxImageUnits = 8;
	resource.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resource.maxCombinedShaderOutputResources = 8;
	resource.maxImageSamples = 0;
	resource.maxVertexImageUniforms = 0;
	resource.maxTessControlImageUniforms = 0;
	resource.maxTessEvaluationImageUniforms = 0;
	resource.maxGeometryImageUniforms = 0;
	resource.maxFragmentImageUniforms = 8;
	resource.maxCombinedImageUniforms = 8;
	resource.maxGeometryTextureImageUnits = 16;
	resource.maxGeometryOutputVertices = 256;
	resource.maxGeometryTotalOutputComponents = 1024;
	resource.maxGeometryUniformComponents = 1024;
	resource.maxGeometryVaryingComponents = 64;
	resource.maxTessControlInputComponents = 128;
	resource.maxTessControlOutputComponents = 128;
	resource.maxTessControlTextureImageUnits = 16;
	resource.maxTessControlUniformComponents = 1024;
	resource.maxTessControlTotalOutputComponents = 4096;
	resource.maxTessEvaluationInputComponents = 128;
	resource.maxTessEvaluationOutputComponents = 128;
	resource.maxTessEvaluationTextureImageUnits = 16;
	resource.maxTessEvaluationUniformComponents = 1024;
	resource.maxTessPatchComponents = 120;
	resource.maxPatchVertices = 32;
	resource.maxTessGenLevel = 64;
	resource.maxViewports = 16;
	resource.maxVertexAtomicCounters = 0;
	resource.maxTessControlAtomicCounters = 0;
	resource.maxTessEvaluationAtomicCounters = 0;
	resource.maxGeometryAtomicCounters = 0;
	resource.maxFragmentAtomicCounters = 8;
	resource.maxCombinedAtomicCounters = 8;
	resource.maxAtomicCounterBindings = 1;
	resource.maxVertexAtomicCounterBuffers = 0;
	resource.maxTessControlAtomicCounterBuffers = 0;
	resource.maxTessEvaluationAtomicCounterBuffers = 0;
	resource.maxGeometryAtomicCounterBuffers = 0;
	resource.maxFragmentAtomicCounterBuffers = 1;
	resource.maxCombinedAtomicCounterBuffers = 1;
	resource.maxAtomicCounterBufferSize = 16384;
	resource.maxTransformFeedbackBuffers = 4;
	resource.maxTransformFeedbackInterleavedComponents = 64;
	resource.maxCullDistances = 8;
	resource.maxCombinedClipAndCullDistances = 8;
	resource.maxSamples = 4;
	resource.maxMeshOutputVerticesNV = 256;
	resource.maxMeshOutputPrimitivesNV = 512;
	resource.maxMeshWorkGroupSizeX_NV = 32;
	resource.maxMeshWorkGroupSizeY_NV = 1;
	resource.maxMeshWorkGroupSizeZ_NV = 1;
	resource.maxTaskWorkGroupSizeX_NV = 32;
	resource.maxTaskWorkGroupSizeY_NV = 1;
	resource.maxTaskWorkGroupSizeZ_NV = 1;
	resource.maxMeshViewCountNV = 4;

	resource.limits.nonInductiveForLoops = 1;
	resource.limits.whileLoops = 1;
	resource.limits.doWhileLoops = 1;
	resource.limits.generalUniformIndexing = 1;
	resource.limits.generalAttributeMatrixVectorIndexing = 1;
	resource.limits.generalVaryingIndexing = 1;
	resource.limits.generalSamplerIndexing = 1;
	resource.limits.generalVariableIndexing = 1;
	resource.limits.generalConstantMatrixVectorIndexing = 1;

	return resource;
}

static void InitializeGLSLang() {
	plastic::utility::InitializeGlobalInstance(
		[]() {
			if (!glslang::InitializeProcess()) {
				throw std::runtime_error("Failed to initialize GLSLang");
			}
			static plastic::utility::Defer gc(
				[]() {
					glslang::FinalizeProcess();
				}
			);
			s_glslang_resources = InitGLSLangResources();
		}
	);
}

namespace fyuu_rhi::common {

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

	static std::vector<std::uint32_t> CompileShader(
		std::string_view src,
		ShaderStage shader_stage,
		CompilationOptions const& options,
		glslang::EShSource language
	) {


		InitializeGLSLang();

		EShLanguage glslang_shader_stage = ShaderStageToEShLanguage(shader_stage);

		glslang::TShader shader(glslang_shader_stage);

		char const* shader_strings[] = { src.data() };
		shader.setStrings(shader_strings, 1);

		/*
		*	set shader language, shader stage and target client version
		*/

		shader.setEnvInput(
			language, glslang_shader_stage,
			glslang::EShClientVulkan,
			130 // 100 for Vulkan 1.0, 110 for Vulkan 1.1...
		);

#if !defined(_NDEBUG)
		shader.setDebugInfo(true);
#endif // !defined(_NDEBUG)

		shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
		shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

		shader.setEntryPoint(options.entry_point.c_str());


		// set macros

		std::ostringstream preamble;
		for (auto const& macro : options.macros) {
			preamble << "#define " << macro.name << " " << macro.value << "\n";
		}

		std::string preamble_str = preamble.str();

		shader.setPreamble(preamble_str.data());

		EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDefault);
		if (!shader.parse(&s_glslang_resources, 100, false, messages)) {
			throw std::runtime_error(std::format("Failed to parse shader: {}", shader.getInfoLog()));
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages)) {
			throw std::runtime_error("Failed to link shader: " + std::string(program.getInfoLog()));
		}

		glslang::SpvOptions spv_options;
		std::vector<std::uint32_t> spirv;
#if !defined(_NDEBUG)
		spv_options.generateDebugInfo = true;
		spv_options.stripDebugInfo = false;
		spv_options.disableOptimizer = true;
		spv_options.optimizeSize = false;
		spv_options.disassemble = true;
		spv_options.validate = true;
		spv_options.emitNonSemanticShaderDebugInfo = true;
		spv_options.emitNonSemanticShaderDebugSource = true;
#else
		spv_options.generateDebugInfo = false;
		spv_options.stripDebugInfo = true;
		spv_options.disableOptimizer = false;
		spv_options.optimizeSize = true;
		spv_options.disassemble = false;
		spv_options.validate = false;
		spv_options.emitNonSemanticShaderDebugInfo = false;
		spv_options.emitNonSemanticShaderDebugSource = false;
#endif // !defined(_NDEBUG)

		glslang::TIntermediate* intermediate = program.getIntermediate(glslang_shader_stage);
		glslang::GlslangToSpv(*intermediate, spirv, &spv_options);

		return spirv;


	}

	std::vector<std::uint32_t> CompileGLSLToSPIRV(
		std::string_view src, 
		ShaderStage shader_stage, 
		CompilationOptions const& options
	) {
		return CompileShader(src, shader_stage, options, glslang::EShSource::EShSourceGlsl);
	}

	std::vector<std::uint32_t> CompileHLSLToSPIRV(
		std::string_view src, 
		ShaderStage shader_stage, 
		CompilationOptions const& options
	) {
		return CompileShader(src, shader_stage, options, glslang::EShSource::EShSourceHlsl);
	}

}