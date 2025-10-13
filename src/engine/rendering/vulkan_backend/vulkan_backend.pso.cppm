export module vulkan_backend:pso;
export import rendering;
import std;
import vulkan_hpp;

namespace fyuu_engine::vulkan {

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
