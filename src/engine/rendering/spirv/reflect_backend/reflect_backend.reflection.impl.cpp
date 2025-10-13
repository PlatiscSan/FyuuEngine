module reflect_backend:reflection;

namespace fyuu_engine::spirv {

	static ShaderStage ConvertShaderStageFlagBits(SpvReflectShaderStageFlagBits stage) noexcept {
		switch (stage) {
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
			return ShaderStage::Vertex;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return ShaderStage::TessellationControl;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return ShaderStage::TessellationEvaluation;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
			return ShaderStage::Geometry;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
			return ShaderStage::Fragment;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
			return ShaderStage::Compute;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
			return ShaderStage::RayGeneration;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
			return ShaderStage::AnyHit;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
			return ShaderStage::ClosestHit;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
			return ShaderStage::Miss;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
			return ShaderStage::Intersection;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
			return ShaderStage::Callable;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:
			return ShaderStage::Task;
		case SpvReflectShaderStageFlagBits::SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:
			return ShaderStage::Mesh;
		default:
			return ShaderStage::Vertex;
		}
	}

	ReflectReflection::ReflectReflection(SPIRVCodeView spirv_code)
		: m_module(spirv_code.size() * sizeof(std::uint32_t), spirv_code.data()) {
	}

	ReflectReflection::ReflectReflection(SPIRVBinaryView spirv_binary)
		: m_module(spirv_binary.size(), spirv_binary.data()) {
	}

	std::vector<EntryPoint> ReflectReflection::QueryEntryPoints() const {
		
		std::vector<EntryPoint> ret_entries;
		std::uint32_t reflection_entry_count = m_module.GetEntryPointCount();
		ret_entries.reserve(reflection_entry_count);
		for (std::uint32_t i = 0; i < reflection_entry_count; ++i) {
			EntryPoint entry{m_module.GetEntryPointName(i), ConvertShaderStageFlagBits(m_module.GetEntryPointShaderStage(i)) };
			ret_entries.emplace_back(std::move(entry));
		}

		return ret_entries;

	}

	std::vector<ResourceDescription> ReflectReflection::QueryResources(EntryPoint const& entry) {

		std::vector<ResourceDescription> resources;
		return resources;
	}

}