export module fyuu_rhi:glslang;
import std;
import :pipeline;

namespace fyuu_rhi::common {

	export extern std::vector<std::uint32_t> CompileGLSLToSPIRV(
		std::string_view src, 
		ShaderStage shader_stage, 
		CompilationOptions const& options
	);

	export extern std::vector<std::uint32_t> CompileHLSLToSPIRV(
		std::string_view src,
		ShaderStage shader_stage,
		CompilationOptions const& options
	);

}