module;

export module graphics:vulkan_command_object;
export import vulkan_hpp;
export import :interface;
import std;

namespace graphics::api::vulkan {

	export struct CommandReadyEvent {
		vk::UniqueCommandBuffer& command_buffer;
	};

	export struct RenderPassInfoRequest {
		std::optional<vk::RenderPassBeginInfo>& render_pass_begin_info;
	};

	export class VulkanCommandObject final : public ICommandObject {
	private:
		vk::UniqueCommandPool m_command_pool;
		vk::UniqueCommandBuffer m_command_buffer;
		util::MessageBusPtr m_message_bus;
		bool m_is_recording;

	public:
		VulkanCommandObject(
			vk::UniqueDevice const& logical_device, 
			std::uint32_t queue_family_index, 
			util::MessageBusPtr const& message_bus,
			vk::AllocationCallbacks const* allocator = nullptr
		);

		VulkanCommandObject(VulkanCommandObject&& other) noexcept;

		~VulkanCommandObject() noexcept override = default;

		VulkanCommandObject& operator=(VulkanCommandObject&& other) noexcept;

		void* GetNativeHandle() const noexcept override;
		API GetAPI() const noexcept override;
		void StartRecording() override;
		void EndRecording() override;
		void Reset() override;

		vk::UniqueCommandBuffer& CommandBuffer() noexcept;

	};

}