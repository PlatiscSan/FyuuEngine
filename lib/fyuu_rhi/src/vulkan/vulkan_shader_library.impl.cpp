module fyuu_rhi:vulkan_shader_library;
import :glslang;
import :vulkan_physical_device;
import :vulkan_logical_device;

namespace fyuu_rhi::vulkan {

	static vk::UniqueShaderModule CreateShaderModule(
		vk::Device const& logical_device,
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		CompilationOptions const& options,
		vk::DispatchLoaderDynamic const& dispatcher
	) {

		std::vector<std::uint32_t> spir_v_src;

		switch (language) {
		case fyuu_rhi::ShaderLanguage::GLSL:
			spir_v_src = common::CompileGLSLToSPIRV(src, shader_stage, options);
			break;
		case fyuu_rhi::ShaderLanguage::HLSL:
			spir_v_src = common::CompileHLSLToSPIRV(src, shader_stage, options);
			break;
		default:
			break;
		}

		vk::ShaderModuleCreateInfo create_info(
			{},
			spir_v_src.size(),
			spir_v_src.data()
		);

		return logical_device.createShaderModuleUnique(create_info, nullptr, dispatcher);

	}

	VulkanShaderLibrary::VulkanShaderLibrary(
		vk::Device const& logical_device,
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		CompilationOptions const& options,
		vk::DispatchLoaderDynamic const& dispatcher
	): m_impl(CreateShaderModule(logical_device, src, shader_stage, language, options, dispatcher)) {
	}

	VulkanShaderLibrary::VulkanShaderLibrary(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
		std::string_view src,
		ShaderStage shader_stage,
		ShaderLanguage language,
		plastic::utility::AnyPointer<CompilationOptions> const& options
	) : VulkanShaderLibrary(logical_device->GetNative(), src, shader_stage, language, *options, *logical_device->GetRawDispatcher()) {
	}

	VulkanShaderLibrary::VulkanShaderLibrary(
		VulkanLogicalDevice const& logical_device, 
		std::string_view src, 
		ShaderStage shader_stage, 
		ShaderLanguage language, 
		CompilationOptions const& options
	) : VulkanShaderLibrary(logical_device.GetNative(), src, shader_stage, language, options, *logical_device.GetRawDispatcher()) {
	}
}