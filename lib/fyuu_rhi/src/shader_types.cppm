module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <string>
#include <filesystem>
#endif // !defined(__cpp_lib_modules)
#include "bitwise_enum_op.hpp"
export module fyuu_rhi:shader_types;
#if defined(__cpp_lib_modules)
import std;
#endif
namespace fs = std::filesystem;
export namespace fyuu_rhi {

	enum class ShaderLanguage : std::uint8_t {
		Unknown,
		HLSL,
		GLSL,
		MSL,	   // Metal Shading Language
		WGSL	   // WebGPU
	};

	enum class ShaderStage : std::uint16_t {
		Unknown,
		Base1 = 1,
		Vertex = Base1,
		Fragment = Base1<< 1,
		Pixel = Fragment,
		Compute = Base1 << 2,
		Geometry = Base1 << 3,
		TessellationControl = Base1 << 4,
		Hull = TessellationControl,
		TessellationEvaluation = Base1 << 5,
		Domain = TessellationEvaluation,
		Amplification = Base1 << 6,
		Mesh = Base1 << 7,
		RayGeneration = Base1 << 8,
		RayIntersection = Base1 << 9,
		RayAnyHit = Base1 << 10,
		RayClosestHit = Base1 << 11,
		RayMiss = Base1 << 12,
		Callable = Base1 << 13,

		RayTracing = RayGeneration  | RayIntersection | RayAnyHit | RayClosestHit | RayMiss | Callable,
		Graphics = Vertex | Fragment | Geometry | TessellationControl | TessellationEvaluation | Amplification | Mesh,
		All = Vertex | Fragment | Compute | Geometry | TessellationControl | TessellationEvaluation | Amplification | Mesh | RayTracing

	};

	DEFINE_BITWISE_ENUM_OPS(ShaderStage)

	enum class HLSLOption : std::uint16_t {
		None = 0,
		Base1 = 1,
		Enable16BitType = Base1
	};

	DEFINE_BITWISE_ENUM_OPS(HLSLOption)

    enum class OptimizationLevel : std::uint8_t {
		None,
		O1,
		O2,
		O3,
		Size,
	};

    struct ShaderCompilationOption {
        std::string entry_point;
        HLSLOption hlsl_option;
        OptimizationLevel optimization;
        std::optional<fs::path> cache_path;
        std::optional<std::string> cache_filename;
        bool opengl_spirv;
    };

} // namespace fyuu_rhi