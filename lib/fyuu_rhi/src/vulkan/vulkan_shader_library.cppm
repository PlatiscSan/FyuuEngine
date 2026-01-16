export module fyuu_rhi:vulkan_shader_library;
import std;
import vulkan_hpp;
import :pipeline;
import :vulkan_declaration;
import plastic.any_pointer;

namespace fyuu_rhi::vulkan {

	export class VulkanShaderLibrary
		: public plastic::utility::EnableSharedFromThis<VulkanShaderLibrary>,
		public IShaderLibrary {
	private:
		vk::UniqueShaderModule m_impl;

	public:
		VulkanShaderLibrary(
			vk::Device const& logical_device,
			std::string_view src,
			ShaderStage shader_stage,
			ShaderLanguage language,
			CompilationOptions const& options,
			vk::DispatchLoaderDynamic const& dispatcher
		);

		VulkanShaderLibrary(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			std::string_view src,
			ShaderStage shader_stage,
			ShaderLanguage language,
			plastic::utility::AnyPointer<CompilationOptions> const& options
		);

		VulkanShaderLibrary(
			VulkanLogicalDevice const& logical_device,
			std::string_view src,
			ShaderStage shader_stage,
			ShaderLanguage language,
			CompilationOptions const& options
		);

	};

}