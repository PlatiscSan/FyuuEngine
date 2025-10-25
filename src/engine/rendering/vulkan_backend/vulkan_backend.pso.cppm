export module vulkan_backend:pso;
export import rendering;
import std;
import vulkan_hpp;

namespace fyuu_engine::vulkan {

	export struct VulkanPipelineStateObject {
		vk::UniquePipeline pipeline;
	};

	export class VulkanPipelineStateObjectBuilder
		: public core::BasePipelineStateObjectBuilder<VulkanPipelineStateObjectBuilder, VulkanPipelineStateObject> {
		friend class core::BasePipelineStateObjectBuilder<VulkanPipelineStateObjectBuilder, VulkanPipelineStateObject>;
	private:
		std::vector<std::byte> m_vertex_shader;
		std::vector<std::byte> m_fragment_shader;

		std::atomic_flag m_vertex_shader_ready = {};
		std::atomic_flag m_fragment_shader_ready = {};

		void LoadShader(
			std::filesystem::path const& path,
			std::vector<std::byte>& output,
			std::atomic_flag& flag
		);

		VulkanPipelineStateObjectBuilder& LoadVertexShaderImpl(std::filesystem::path const& path);

		VulkanPipelineStateObjectBuilder& LoadFragmentShaderImpl(std::filesystem::path const& path);

		VulkanPipelineStateObjectBuilder& LoadPixelShaderImpl(std::filesystem::path const& path);
	};

	/*export class VulkanPipelineStateObject final : public core::IPipelineStateObject {
	private:
		vk::UniquePipeline m_pipeline;

	public:
		template <class... Args>
			requires std::constructible_from<vk::UniquePipeline, Args...>
		VulkanPipelineStateObject(Args&&... args)
			: m_pipeline(std::forward<Args>(args)...) {

		}

		void* GetNativeHandle() const noexcept override;

	};

	export class VulkanPipelineStateObjectBuilder {
	private:
		vk::Device m_device;
		vk::AllocationCallbacks* m_allocator;
		core::PipelineStateObjectDesc m_desc;

		void Reflect();
		vk::UniqueShaderModule CreateShaderModule(std::span<std::uint32_t> byte_code) const;

	public:
		VulkanPipelineStateObjectBuilder& SetDevice(vk::Device device) noexcept;
		VulkanPipelineStateObjectBuilder& SetAllocator(vk::AllocationCallbacks* allocator) noexcept;
		VulkanPipelineStateObjectBuilder& SetDescription(core::PipelineStateObjectDesc const& desc) noexcept;

		VulkanPipelineStateObject Build() noexcept;
	};*/

}
