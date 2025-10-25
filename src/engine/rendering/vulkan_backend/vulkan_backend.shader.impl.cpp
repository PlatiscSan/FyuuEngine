module;
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

module vulkan_backend:shader;
import defer;

namespace fyuu_engine::vulkan {

	static std::once_flag s_glslang_init_flag;
	static void InitializeGlslang() {
		std::call_once(
			s_glslang_init_flag,
			[]() {
				if (!glslang::InitializeProcess()) {
					throw std::runtime_error("Failed to initialize glslang process.");
				}
				static util::Defer gc(
					[]() {
						glslang::FinalizeProcess();
					}
				);
			}
		);
	}

	static EShLanguage ToGlslangShaderType(core::ShaderStage type) noexcept {
		switch (type) {
		case core::ShaderStage::Vertex:
			return EShLanguage::EShLangVertex;
		case core::ShaderStage::Fragment:
			return EShLanguage::EShLangFragment;
		case core::ShaderStage::Compute:
			return EShLanguage::EShLangCompute;
		default:
			return EShLangCount;
		}
	}

	std::vector<std::byte> VulkanShaderCompiler::CompileImpl(std::span<std::byte> source_code, core::CompilationOption const& option) {
		
		glslang::TShader shader(ToGlslangShaderType(option.target));

		std::array shader_strings = { reinterpret_cast<char const*>(source_code.data()) };
		shader.setStrings(shader_strings.data(), static_cast<int>(shader_strings.size()));

		auto messages = EShMessages::EShMsgSpvRules | EShMessages::EShMsgVulkanRules;

		TBuiltInResource resources = {};

		switch (option.language) {
		case core::ShaderLanguage::GLSL:
			break;
		case core::ShaderLanguage::HLSL:
			messages = static_cast<EShMessages>(messages | EShMessages::EShMsgReadHlsl);
			shader.setEnvInput(glslang::EShSourceHlsl, ToGlslangShaderType(option.target), glslang::EShClientVulkan, 100);
			break;
		default:
			throw std::runtime_error("Unsupported shader language.");
		}

		if (!shader.parse(&resources, 100, false, static_cast<EShMessages>(messages))) {
			auto log = shader.getInfoLog();
			auto debug_log = shader.getInfoDebugLog();
			throw std::runtime_error("Failed to parse shader: " + std::string(log) + "\n" + std::string(debug_log));
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if(!program.link(static_cast<EShMessages>(messages))) {
			auto log = program.getInfoLog();
			auto debug_log = program.getInfoDebugLog();
			throw std::runtime_error("Failed to link shader program: " + std::string(log) + "\n" + std::string(debug_log));
		}

		std::vector<std::uint32_t> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(ToGlslangShaderType(option.target)), spirv);

		std::vector<std::byte> target;
		target.reserve(spirv.size() * sizeof(std::uint32_t));
		std::memmove(target.data(), spirv.data(), spirv.size() * sizeof(std::uint32_t));

		return target;

	}

	VulkanShaderCompiler::VulkanShaderCompiler()
		: Base() {
		InitializeGlslang();
	}

}