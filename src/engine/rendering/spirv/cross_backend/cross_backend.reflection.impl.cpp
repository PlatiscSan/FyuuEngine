module cross_backend:reflection;

namespace fyuu_engine::spirv {

    static ShaderStage ConvertExecutionModel(spv::ExecutionModel model) noexcept {
        switch (model) {
        case spv::ExecutionModelVertex: return ShaderStage::Vertex;
        case spv::ExecutionModelTessellationControl: return ShaderStage::TessellationControl;
        case spv::ExecutionModelTessellationEvaluation: return ShaderStage::TessellationEvaluation;
        case spv::ExecutionModelGeometry: return ShaderStage::Geometry;
        case spv::ExecutionModelFragment: return ShaderStage::Fragment;
        case spv::ExecutionModelGLCompute: return ShaderStage::Compute;
        case spv::ExecutionModelRayGenerationKHR: return ShaderStage::RayGeneration;
        case spv::ExecutionModelIntersectionKHR: return ShaderStage::Intersection;
        case spv::ExecutionModelAnyHitKHR: return ShaderStage::AnyHit;
        case spv::ExecutionModelClosestHitKHR: return ShaderStage::ClosestHit;
        case spv::ExecutionModelMissKHR: return ShaderStage::Miss;
        case spv::ExecutionModelCallableKHR: return ShaderStage::Callable;
        case spv::ExecutionModelTaskEXT: return ShaderStage::Task;
        case spv::ExecutionModelMeshEXT: return ShaderStage::Mesh;
        default: return ShaderStage::Vertex;
        }
    }

    static spv::ExecutionModel ConvertShaderStage(ShaderStage stage) noexcept {
        switch (stage) {
        case ShaderStage::Vertex: return spv::ExecutionModelVertex;
        case ShaderStage::TessellationControl: return spv::ExecutionModelTessellationControl;
        case ShaderStage::TessellationEvaluation: return spv::ExecutionModelTessellationEvaluation;
        case ShaderStage::Geometry: return spv::ExecutionModelGeometry;
        case ShaderStage::Fragment: return spv::ExecutionModelFragment;
        case ShaderStage::Compute: return spv::ExecutionModelGLCompute;
        case ShaderStage::RayGeneration: return spv::ExecutionModelRayGenerationKHR;
        case ShaderStage::Intersection: return spv::ExecutionModelIntersectionKHR;
        case ShaderStage::AnyHit: return spv::ExecutionModelAnyHitKHR;
        case ShaderStage::ClosestHit: return spv::ExecutionModelClosestHitKHR;
        case ShaderStage::Miss: return spv::ExecutionModelMissKHR;
        case ShaderStage::Callable: return spv::ExecutionModelCallableKHR;
        case ShaderStage::Task: return spv::ExecutionModelTaskEXT;
        case ShaderStage::Mesh: return spv::ExecutionModelMeshEXT;
        default: return spv::ExecutionModelVertex;
        }
    }

	CrossReflection::CrossReflection(SPIRVCodeView spirv_code) 
		: m_compiler(SPIRVCode(spirv_code.begin(), spirv_code.end())) {
	}

	CrossReflection::CrossReflection(SPIRVBinaryView spirv_binary)
		: m_compiler(
			reinterpret_cast<std::uint32_t*>(spirv_binary.data()), 
			spirv_binary.size() / sizeof(std::uint32_t)
		) {
	}

	std::vector<EntryPoint> CrossReflection::QueryEntryPoints() const {

		auto cross_entries = m_compiler.get_entry_points_and_stages();
        std::vector<EntryPoint> ret_entries;
        ret_entries.reserve(cross_entries.size());

        for (auto const& cross_entry : cross_entries) {
            EntryPoint entry{ cross_entry.name, ConvertExecutionModel(cross_entry.execution_model) };
            ret_entries.emplace_back(std::move(entry));
        }

		return ret_entries;

	}

    std::vector<ResourceDescription> CrossReflection::QueryResources(EntryPoint const& entry) {

        m_compiler.set_entry_point(entry.name, ConvertShaderStage(entry.stage));

        std::vector<ResourceDescription> ret_resource_descriptions;

        auto cross_resources = m_compiler.get_shader_resources();


        // Process uniform buffers
        for (auto& cross_resource : cross_resources.uniform_buffers) {
            ResourceDescription desc;
            desc.set = m_compiler.get_decoration(cross_resource.id, spv::DecorationDescriptorSet);
            desc.binding = m_compiler.get_decoration(cross_resource.id, spv::DecorationBinding);
            auto& type = m_compiler.get_type(cross_resource.base_type_id);
            desc.count = !type.array.empty() ? type.array[0] : 1;
            desc.size = m_compiler.get_declared_struct_size(type);
            desc.name = cross_resource.name;
            desc.stage = entry.stage;
            desc.type = ResourceType::UniformBuffer;

            ret_resource_descriptions.push_back(desc);
        }


        // Process storage buffers
        for (auto& resource : cross_resources.storage_buffers) {
            ResourceDescription desc{};
            desc.set = m_compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            desc.binding = m_compiler.get_decoration(resource.id, spv::DecorationBinding);
            auto& type = m_compiler.get_type(resource.base_type_id);
            desc.count = !type.array.empty() ? type.array[0] : 1;
            desc.size = m_compiler.get_declared_struct_size(type);
            desc.name = resource.name;
            desc.stage = entry.stage;
            desc.type = ResourceType::StorageBuffer;

            ret_resource_descriptions.push_back(desc);
        }

        return ret_resource_descriptions;

    }


}