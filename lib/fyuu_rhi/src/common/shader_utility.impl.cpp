/* shader_infer.impl.cpp */
module;
#if !defined(__cpp_lib_modules)
#include <string_view>
#include <filesystem>
#include <algorithm> 
#endif // !defined(__cpp_lib_modules)
#include <spirv_cross.hpp>
module fyuu_rhi:shader_utility;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;

namespace fs = std::filesystem;

namespace fyuu_rhi {
	
	/**
	 * @brief Infer shader stage from file extension (GLSL style).
	 * @param path File path.
	 * @return ShaderStage or Unknown if not recognized.
	 */
	ShaderStage InferStageFromExtension(fs::path const& path) {
		std::string ext = path.extension().string();
		for (auto& c : ext) c = static_cast<char>(std::tolower(c));

		if (ext == ".hlsl" || ext == ".hlsli" || ext == ".fx")
			return ShaderStage::Unknown;

		if (ext == ".vert") return ShaderStage::Vertex;
		if (ext == ".frag") return ShaderStage::Pixel;
		if (ext == ".geom") return ShaderStage::Geometry;
		if (ext == ".comp") return ShaderStage::Compute;
		if (ext == ".tesc") return ShaderStage::TessellationControl;
		if (ext == ".tese") return ShaderStage::TessellationEvaluation;
		if (ext == ".mesh") return ShaderStage::Mesh;
		if (ext == ".task") return ShaderStage::Amplification;
		if (ext == ".rgen") return ShaderStage::RayGeneration;
		if (ext == ".rmiss") return ShaderStage::RayMiss;
		if (ext == ".rchit") return ShaderStage::RayClosestHit;
		if (ext == ".rahit") return ShaderStage::RayAnyHit;
		if (ext == ".rint") return ShaderStage::RayIntersection;
		if (ext == ".rcall") return ShaderStage::Callable;
	
		return ShaderStage::Unknown;
	}

	/**
	 * @brief Infer shader stage from HLSL filename (e.g., "vertex.hlsl", "ps_main").
	 * @param path File path.
	 * @return ShaderStage or Unknown.
	 */
	ShaderStage InferStageFromHLSLFilename(fs::path const& path) {
		std::string stem = path.stem().string();
		for (auto& c : stem) c = static_cast<char>(std::tolower(c));
	
		if (stem.find("vertex") != std::string::npos || stem.find("_vs") != std::string::npos || stem == "vs")
			return ShaderStage::Vertex;
		if (stem.find("pixel") != std::string::npos || stem.find("fragment") != std::string::npos || stem.find("_ps") != std::string::npos || stem == "ps")
			return ShaderStage::Pixel;
		if (stem.find("compute") != std::string::npos || stem.find("_cs") != std::string::npos || stem == "cs")
			return ShaderStage::Compute;
		if (stem.find("geometry") != std::string::npos || stem.find("_gs") != std::string::npos || stem == "gs")
			return ShaderStage::Geometry;
		if (stem.find("tessellation") != std::string::npos || stem.find("_hs") != std::string::npos || stem == "hs")
			return ShaderStage::TessellationControl;
		if (stem.find("domain") != std::string::npos || stem.find("_ds") != std::string::npos || stem == "ds")
			return ShaderStage::TessellationEvaluation;
		if (stem.find("mesh") != std::string::npos || stem.find("_ms") != std::string::npos || stem == "ms")
			return ShaderStage::Mesh;
		if (stem.find("amplification") != std::string::npos || stem.find("task") != std::string::npos || stem.find("_as") != std::string::npos || stem == "as")
			return ShaderStage::Amplification;
		if (stem.find("raygeneration") != std::string::npos || stem.find("_rgen") != std::string::npos || stem == "rgen")
			return ShaderStage::RayGeneration;
		if (stem.find("raymiss") != std::string::npos || stem.find("_rmiss") != std::string::npos || stem == "rmiss")
			return ShaderStage::RayMiss;
		if (stem.find("rayclosesthit") != std::string::npos || stem.find("_rchit") != std::string::npos || stem == "rchit")
			return ShaderStage::RayClosestHit;
		if (stem.find("rayanyhit") != std::string::npos || stem.find("_rahit") != std::string::npos || stem == "rahit")
			return ShaderStage::RayAnyHit;
		if (stem.find("rayintersection") != std::string::npos || stem.find("_rint") != std::string::npos || stem == "rint")
			return ShaderStage::RayIntersection;
		if (stem.find("callable") != std::string::npos || stem.find("_rcall") != std::string::npos || stem == "rcall")
			return ShaderStage::Callable;
	
		return ShaderStage::Unknown;
	}

	/**
	 * @brief Infer shader stage from a SPIR-V file name.
	 *        Handles names like "foo.vert.spv" or "vertex.spv".
	 * @param path File path.
	 * @return ShaderStage or Unknown.
	 */
	ShaderStage InferStageFromSPIRVFilename(fs::path const& path) {
		std::string filename = path.filename().string();
		if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".spv") {
			filename = filename.substr(0, filename.size() - 4);
		}
		fs::path temp(filename);
		std::string ext = temp.extension().string();
		if (!ext.empty()) {
			return InferStageFromExtension(temp);
		}
		return InferStageFromHLSLFilename(temp);
	}
	
	/**
	 * @brief Convert RHI ShaderStage to SPIR-V ExecutionModel.
	 * @param shader_stage The RHI stage.
	 * @return Corresponding SPIR-V execution model.
	 * @throws std::runtime_error if the stage is unknown.
	 */
	spv::ExecutionModel ShaderStageToSPVExecutionModel(ShaderStage shader_stage) {
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

} // namespace fyuu_rhi
