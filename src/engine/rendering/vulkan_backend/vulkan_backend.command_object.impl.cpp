module vulkan_backend:command_object;
import vulkan_hpp;
import static_hash_map;

namespace fyuu_engine::vulkan {

	static vk::ImageLayout ResourceStateToVulkanImageLayout(core::ResourceState state) try {

		static concurrency::StaticHashMap<core::ResourceState, vk::ImageLayout, 10> map = {
			{core::ResourceState::Common, vk::ImageLayout::eGeneral},
			{core::ResourceState::Present, vk::ImageLayout::ePresentSrcKHR},
			{core::ResourceState::OutputTarget, vk::ImageLayout::eColorAttachmentOptimal},
		};

		return map.UnsafeAt(state);

	}
	catch (std::out_of_range const&) {
		return vk::ImageLayout::eUndefined;
	}

	static vk::PipelineStageFlagBits ResourceStateToVulkanPipelineStage(
		core::ResourceState const& state
	) try {

		using Map = concurrency::StaticHashMap<core::ResourceState, vk::PipelineStageFlagBits, 10>;

		static Map map = {
			{core::ResourceState::Common,        vk::PipelineStageFlagBits::eTopOfPipe},
			{core::ResourceState::VertexBuffer,  vk::PipelineStageFlagBits::eVertexInput},
			{core::ResourceState::IndexBuffer,   vk::PipelineStageFlagBits::eVertexInput},
			{core::ResourceState::OutputTarget,  vk::PipelineStageFlagBits::eColorAttachmentOutput},
			{core::ResourceState::Present,       vk::PipelineStageFlagBits::eColorAttachmentOutput},
			{core::ResourceState::CopySrc,       vk::PipelineStageFlagBits::eTransfer},
			{core::ResourceState::CopyDest,      vk::PipelineStageFlagBits::eTransfer},
		};

		return map.UnsafeAt(state);

	}
	catch (std::out_of_range const&) {
		return {};
	}

	static vk::UniqueCommandPool CreateCommandPool(
		vk::Device const& logical_device, 
		std::uint32_t queue_family, 
		vk::AllocationCallbacks const* allocator
	) {
		vk::CommandPoolCreateInfo info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family);
		return logical_device.createCommandPoolUnique(info, allocator);
	}

	static std::vector<vk::UniqueCommandBuffer> CreateCommandBuffers(
		vk::Device const& logical_device, 
		vk::CommandPool const& command_pool,
		std::uint32_t buffer_count
	) {
		vk::CommandBufferAllocateInfo alloc_info(command_pool, vk::CommandBufferLevel::ePrimary, buffer_count);
		return logical_device.allocateCommandBuffersUnique(alloc_info);
		
	}

	static vk::PrimitiveTopology PrimitiveTopologyToVulkanPrimitiveTopology(core::PrimitiveTopology primitive_topology) {
		switch (primitive_topology) {
		case fyuu_engine::core::PrimitiveTopology::TriangleStrip:
			return vk::PrimitiveTopology::eTriangleStrip;

		case fyuu_engine::core::PrimitiveTopology::TriangleList:
			return vk::PrimitiveTopology::eTriangleList;

		case fyuu_engine::core::PrimitiveTopology::LineStrip:
			return vk::PrimitiveTopology::eLineStrip;

		case fyuu_engine::core::PrimitiveTopology::LineList:
			return vk::PrimitiveTopology::eLineList;

		case fyuu_engine::core::PrimitiveTopology::PointList:
			return vk::PrimitiveTopology::ePointList;
		}
	}

	void VulkanCommandObject::BeginRecordingImpl() {

		m_message_bus->Publish<VulkanFrameIndexRequest>(m_current_frame_index);
		vk::UniqueCommandBuffer& command_buffer = m_command_buffers[m_current_frame_index];

		command_buffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		command_buffer->begin(begin_info);

	}

	void VulkanCommandObject::EndRecordingImpl() {

		vk::UniqueCommandBuffer& command_buffer = m_command_buffers[m_current_frame_index];

		command_buffer->end();

		if (m_message_bus) {
			m_message_bus->Publish<VulkanCommandBufferReady>(command_buffer);
		}

	}

	void VulkanCommandObject::ResetImpl() {
		m_command_buffers[m_current_frame_index]->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	}

	void VulkanCommandObject::SetViewportImpl(core::Viewport const& viewport) {

		vk::ArrayProxy<vk::Viewport> viewports(1u, reinterpret_cast<vk::Viewport const*>(&viewport));
		m_command_buffers[m_current_frame_index]->setViewport(0, viewports);

	}

	void VulkanCommandObject::SetScissorRectImpl(core::Rect const& rect) {

		vk::ArrayProxy<vk::Rect2D> rects(1u, reinterpret_cast<vk::Rect2D const*>(&rect));
		m_command_buffers[m_current_frame_index]->setScissor(0, rects);

	}

	void VulkanCommandObject::BarrierImpl(VulkanBufferAllocation const& buffer, core::ResourceBarrierDesc const& desc) {

		auto& top = m_command_buffers.back();

	}

	void VulkanCommandObject::BarrierImpl(VulkanFrameBuffer const& frame_buffer, core::ResourceBarrierDesc const& desc) {

		auto& top = m_command_buffers.back();

		vk::ImageMemoryBarrier barrier(
			vk::AccessFlagBits::eColorAttachmentWrite,
			{},
			ResourceStateToVulkanImageLayout(desc.before),
			ResourceStateToVulkanImageLayout(desc.after),
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			frame_buffer.vk_image,
			vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		);

		top->pipelineBarrier(
			ResourceStateToVulkanPipelineStage(desc.before),
			ResourceStateToVulkanPipelineStage(desc.after),
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

	}

	void VulkanCommandObject::BarrierImpl(std::monostate const& texture, core::ResourceBarrierDesc const& desc)
	{
	}

	void VulkanCommandObject::BeginRenderPassImpl(VulkanFrameBuffer const& frame_buffer, std::span<float> clear_value) {

		vk::ClearColorValue vk_clear_color_value(clear_value[0], clear_value[1], clear_value[2], clear_value[3]);
		vk::ClearValue vk_clear_value(vk_clear_color_value);

		vk::RenderPassBeginInfo begin_info(
			frame_buffer.vk_render_pass, 
			frame_buffer.vk_frame_buffer, 
			frame_buffer.area,
			1u,
			&vk_clear_value
		);

		m_command_buffers[m_current_frame_index]->beginRenderPass(begin_info, vk::SubpassContents::eInline);

	}

	void VulkanCommandObject::EndRenderPassImpl(VulkanFrameBuffer const& frame_buffer) {
		m_command_buffers[m_current_frame_index]->endRenderPass();
	}

	void VulkanCommandObject::BindVertexBufferImpl(VulkanBufferAllocation const& vertex_buffer, core::VertexDesc const& desc) {
		std::size_t offset = vertex_buffer.GetOffset();
		vk::Buffer buffer = vertex_buffer.GetBuffer();
		m_command_buffers[m_current_frame_index]->bindVertexBuffers(desc.slot, 1, &buffer, &offset);
	}

	void VulkanCommandObject::SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology) {
		m_command_buffers[m_current_frame_index]->setPrimitiveTopology(PrimitiveTopologyToVulkanPrimitiveTopology(primitive_topology));
	}

	void VulkanCommandObject::DrawImpl(
		std::uint32_t index_count, std::uint32_t instance_count,
		std::uint32_t start_index, std::int32_t base_vertex,
		std::uint32_t start_instance
	) {
		auto& command_buffer = m_command_buffers[m_current_frame_index];

		command_buffer->drawIndexed(
			index_count,
			instance_count,
			start_index,
			base_vertex,
			start_instance
		);
	}

	void VulkanCommandObject::ClearImpl(VulkanFrameBuffer const& frame_buffer, std::span<float> rgba, core::Rect const& rect) {

		auto& command_buffer = m_command_buffers[m_current_frame_index];
		vk::Offset2D offset = { rect.x, rect.y };
		vk::Extent2D extent = { rect.width, rect.height };
		vk::ClearColorValue clear_color(rgba[0], rgba[1], rgba[2], rgba[3]);
		vk::ImageSubresourceRange subresource_range(
			vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
		);

		vk::ClearRect clear_rect(vk::Rect2D(offset, extent), 0, 1);
		std::array<vk::ClearValue, 1> clear_values = { vk::ClearValue(clear_color) };

		command_buffer->clearColorImage(
			frame_buffer.vk_image,
			vk::ImageLayout::eTransferDstOptimal,
			&clear_color,
			1, &subresource_range
		);

	}

	void VulkanCommandObject::CopyImpl(VulkanBufferAllocation const& src, VulkanBufferAllocation const& dest) {

		auto& command_buffer = m_command_buffers[m_current_frame_index];

		vk::BufferCopy copy_region(src.GetOffset(), dest.GetOffset(), std::min(src.GetSize(), dest.GetSize()));
		command_buffer->copyBuffer(src.GetBuffer(), dest.GetBuffer(), 1, &copy_region);

	}

	VulkanCommandObject::VulkanCommandObject(
		util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus, 
		std::uint32_t frame_count,
		vk::Device const& logical_device, 
		std::uint32_t queue_family, 
		vk::AllocationCallbacks const* allocator
	) : m_message_bus(util::MakeReferred(message_bus)),
		m_logical_device(logical_device),
		m_command_pool(CreateCommandPool(m_logical_device, queue_family, allocator)),
		m_command_buffers(CreateCommandBuffers(m_logical_device, *m_command_pool, frame_count)) {
	}

}
