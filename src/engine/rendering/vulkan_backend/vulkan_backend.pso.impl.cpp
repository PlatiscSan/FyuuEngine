module vulkan_backend:pso;

namespace fyuu_engine::vulkan {

	/*static vk::ShaderStageFlagBits ToVulkan(core::ShaderStage stage) {

	}

	void* VulkanPipelineStateObject::GetNativeHandle() const noexcept {
		VkPipeline pipeline = *m_pipeline;
		return pipeline;
	}

	void VulkanPipelineStateObjectBuilder::Reflect() {

		std::unordered_map<std::span<std::uint32_t>, spirv_cross::Compiler> compliers;
		for (auto& [_, byte_code] : m_desc.shaders) {
			compliers.try_emplace(byte_code, &byte_code.front(), byte_code.size());
		}

	}

	vk::UniqueShaderModule VulkanPipelineStateObjectBuilder::CreateShaderModule(std::span<std::uint32_t> byte_code) const {

		vk::ShaderModuleCreateInfo info(
			vk::ShaderModuleCreateFlags(),
			byte_code.size(),
			byte_code.data()
		);

		return m_device.createShaderModuleUnique(info, m_allocator);

	}

	VulkanPipelineStateObjectBuilder& VulkanPipelineStateObjectBuilder::SetDevice(vk::Device device) noexcept {
		m_device = device;
		return *this;
	}

	VulkanPipelineStateObjectBuilder& VulkanPipelineStateObjectBuilder::SetAllocator(vk::AllocationCallbacks* allocator) noexcept {
		m_allocator = allocator;
		return *this;
	}

	VulkanPipelineStateObjectBuilder& VulkanPipelineStateObjectBuilder::SetDescription(core::PipelineStateObjectDesc const& desc) noexcept {
		m_desc = desc;
		return *this;
	}

	VulkanPipelineStateObject VulkanPipelineStateObjectBuilder::Build() noexcept {

		std::unordered_map<core::ShaderStage, vk::UniqueShaderModule> shaders;
		std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

		for (auto& [stage, byte_code] : m_desc.shaders) {
			shaders.try_emplace(stage, CreateShaderModule(byte_code));
		}

		return VulkanPipelineStateObject();
	}*/

}
