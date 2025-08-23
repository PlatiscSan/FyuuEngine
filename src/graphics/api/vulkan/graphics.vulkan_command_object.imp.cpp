module;

module graphics:vulkan_command_object;
import std;

static vk::UniqueCommandPool CreateCommandPool(
	vk::UniqueDevice const& logical_device,
	std::uint32_t queue_family_index,
	vk::AllocationCallbacks const* allocator = nullptr
) {
	vk::CommandPoolCreateInfo create_info(
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family_index
	);
	return logical_device->createCommandPoolUnique(create_info, allocator);
}

static vk::UniqueCommandBuffer AllocateCommandBuffer(
	vk::UniqueDevice const& logical_device,
	vk::UniqueCommandPool const& command_pool
) {
	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.setCommandPool(*command_pool);
	alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
	alloc_info.setCommandBufferCount(1u);
	auto command_buffers = logical_device->allocateCommandBuffersUnique(alloc_info);
	return std::move(command_buffers[0]);
}

namespace graphics::api::vulkan {

	VulkanCommandObject::VulkanCommandObject(
		vk::UniqueDevice const& logical_device,
		std::uint32_t queue_family_index,
		util::MessageBusPtr const& message_bus,
		vk::AllocationCallbacks const* allocator
	) : m_command_pool(CreateCommandPool(logical_device, queue_family_index, allocator)),
		m_command_buffer(AllocateCommandBuffer(logical_device, m_command_pool)),
		m_message_bus(util::MakeReferred(message_bus)),
		m_is_recording(false) {
	}

	VulkanCommandObject::VulkanCommandObject(VulkanCommandObject&& other) noexcept
		: m_command_pool(std::move(other.m_command_pool)),
		m_command_buffer(std::move(other.m_command_buffer)),
		m_message_bus(std::move(other.m_message_bus)),
		m_is_recording(std::exchange(other.m_is_recording, false)) {
	}

	VulkanCommandObject& VulkanCommandObject::operator=(VulkanCommandObject&& other) noexcept {
		if (this != &other) {
			m_command_pool = std::move(other.m_command_pool);
			m_command_buffer = std::move(other.m_command_buffer);
			m_message_bus = std::move(other.m_message_bus);
			m_is_recording = std::exchange(other.m_is_recording, false);
		}
		return *this;
	}

	void* VulkanCommandObject::GetNativeHandle() const noexcept {
		return m_command_buffer.get();
	}

	API VulkanCommandObject::GetAPI() const noexcept {
		return API::Vulkan;
	}

	void VulkanCommandObject::StartRecording() {

		if (m_is_recording) {
			return; // Already recording
		}

		vk::CommandBufferBeginInfo command_buffer_begin_info{};
		command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		m_command_buffer->begin(command_buffer_begin_info);

		std::optional<vk::RenderPassBeginInfo> render_pass_begin_info;
		if (m_message_bus) {
			RenderPassInfoRequest event{ render_pass_begin_info };
			m_message_bus->Publish(event);
		}

		if (render_pass_begin_info) {
			m_command_buffer->beginRenderPass(&(*render_pass_begin_info), vk::SubpassContents::eInline);
		}

		m_is_recording = true;

	}

	void VulkanCommandObject::EndRecording() {

		if (!m_is_recording) {
			return; // Not recording
		}

		m_command_buffer->endRenderPass();
		m_command_buffer->end();
		m_is_recording = false;

		if(m_message_bus) {
			CommandReadyEvent event{ m_command_buffer };
			m_message_bus->Publish(event);
		}

	}

	void VulkanCommandObject::Reset() {
		if (m_is_recording) {
			throw std::runtime_error("Cannot reset command buffer while recording.");
		}
		m_command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	vk::UniqueCommandBuffer& VulkanCommandObject::CommandBuffer() noexcept {
		return m_command_buffer;
	}

}
