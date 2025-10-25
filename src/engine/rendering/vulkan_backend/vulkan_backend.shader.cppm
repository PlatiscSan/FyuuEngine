export module vulkan_backend:shader;
export import rendering;
import std;

namespace fyuu_engine::vulkan {
	
	export class VulkanShaderCompiler : public core::BaseShaderCompiler<VulkanShaderCompiler> {
		friend class core::BaseShaderCompiler<VulkanShaderCompiler>;
	private:
		std::vector<std::byte> CompileImpl(std::span<std::byte> source_code, core::CompilationOption const& option);
		
	public:
		using Base = core::BaseShaderCompiler<VulkanShaderCompiler>;
		VulkanShaderCompiler();
	};

	export class VulkanShaderReflection : public core::BaseShaderReflection<VulkanShaderReflection> {
		friend class core::BaseShaderReflection<VulkanShaderReflection>;
	};
}