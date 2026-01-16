module fyuu_rhi:vulkan_command_object;
import :vulkan_physical_device;
import :vulkan_logical_device;

namespace fyuu_rhi::vulkan {
	VulkanCommandBuffer::VulkanCommandBuffer(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
		std::uint32_t family
	) : m_allocator(
		[&logical_device, family]() {
			vk::CommandPoolCreateInfo pool_info(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				family,
				nullptr
			);
			return logical_device->GetNative().createCommandPoolUnique(
				pool_info,
				nullptr,
				*logical_device->GetRawDispatcher()
			);
		}()),
		m_impl(
			[this, &logical_device]() {
				vk::CommandBufferAllocateInfo alloc_info(
					*m_allocator,
					vk::CommandBufferLevel::ePrimary,
					1u,
					nullptr
				);

				std::vector<vk::UniqueCommandBuffer> command_buffers =
					logical_device->GetNative().allocateCommandBuffersUnique(
						alloc_info,
						*logical_device->GetRawDispatcher()
					);

				return std::move(command_buffers.front());

			}()) {
	}

	VulkanCommandBuffer::VulkanCommandBuffer(vk::UniqueCommandBuffer&& vk_command_buffer)
		: m_allocator(),
		m_impl(std::move(vk_command_buffer)) {
	}

	VulkanCommandBuffer::VulkanCommandBuffer(vk::CommandBuffer vk_command_buffer)
		: m_allocator(),
		m_impl(vk_command_buffer) {
	}

	vk::UniqueCommandBuffer fyuu_rhi::vulkan::VulkanCommandBuffer::Release() noexcept {
		return std::move(m_impl);
	}

	vk::CommandBuffer VulkanCommandBuffer::ReleaseNative() noexcept {
		return m_impl.release();
	}

	vk::CommandBuffer VulkanCommandBuffer::GetNative() const noexcept {
		return *m_impl;
	}

	VulkanCommandAllocator::VulkanCommandAllocator(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
		std::uint32_t family,
		std::size_t capacity
	) : m_impl(
		[this, &logical_device, family]() {
			vk::CommandPoolCreateInfo pool_info(
				vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				family,
				nullptr
			);
			return logical_device->GetNative().createCommandPoolUnique(
				pool_info,
				nullptr,
				*logical_device->GetRawDispatcher()
			);

		}()),
		m_command_buffers(
			[this, &logical_device, capacity]() {
				vk::CommandBufferAllocateInfo alloc_info(
					*m_impl,
					vk::CommandBufferLevel::ePrimary,
					static_cast<std::uint32_t>(capacity),
					nullptr
				);

				return logical_device->GetNative().allocateCommandBuffersUnique(
					alloc_info,
					*logical_device->GetRawDispatcher()
				);
			}()) {
	}

	VulkanCommandAllocator::UniqueCommandBuffer VulkanCommandAllocator::Allocate() {
		std::unique_lock<std::mutex> lock(m_command_buffer_mutex);
		m_command_buffer_cv.wait(
			lock,
			[this]() noexcept {
				return !m_command_buffers.empty();
			}
		);
		vk::UniqueCommandBuffer vk_command_buffer = std::move(m_command_buffers.back());
		m_command_buffers.pop_back();
		return UniqueCommandBuffer(VulkanCommandBuffer(std::move(vk_command_buffer)), GetID());
	}

	VulkanCommandAllocator::CommandBufferGC::CommandBufferGC(std::size_t allocator_id) noexcept
		: m_allocator_id(allocator_id) {

	}

	VulkanCommandAllocator::CommandBufferGC::CommandBufferGC(CommandBufferGC const& other) noexcept
		: m_allocator_id(other.m_allocator_id) {

	}

	VulkanCommandAllocator::CommandBufferGC::CommandBufferGC(CommandBufferGC&& other) noexcept
		: m_allocator_id(std::exchange(other.m_allocator_id, 0ull)) {

	}

	void VulkanCommandAllocator::CommandBufferGC::operator()(VulkanCommandBuffer& command_buffer) noexcept {
		plastic::utility::AnyPointer<VulkanCommandAllocator> allocator =
			plastic::utility::QueryObject<VulkanCommandAllocator>(m_allocator_id);
		std::unique_lock<std::mutex> lock(allocator->m_command_buffer_mutex);
		allocator->m_command_buffers.emplace_back(std::move(command_buffer.Release()));
		allocator->m_command_buffer_cv.notify_one();
	}

	void VulkanCommandQueue::WaitUntilCompletedImpl() const {
		m_impl.waitIdle(*m_dispatcher);
	}

	void VulkanCommandQueue::ExecuteCommandImpl(VulkanCommandBuffer& command) {

		vk::CommandBuffer vk_command_buffer = command.GetNative();

		vk::SubmitInfo submit_info(
			/* waitSemaphoreCount_ */	0u,
			/* pWaitSemaphores_ */		nullptr,
			/* pWaitDstStageMask_ */	nullptr,
			/* commandBufferCount_ */	1u,
			/* pCommandBuffers_ */		&vk_command_buffer,
			/* signalSemaphoreCount_ */	0u,
			/* pSignalSemaphores_*/		nullptr,
			/* pNext_ */				nullptr
		);

		m_impl.submit(submit_info, nullptr, *m_dispatcher);
		WaitUntilCompleted();

	}

	VulkanCommandQueue::VulkanCommandQueue(
		plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
		CommandObjectType type,
		QueuePriority priority
	) : m_logical_device_id(logical_device->GetID()),
		m_queue_info(logical_device->AllocateCommandQueue(type, priority)),
		m_impl(
			logical_device->GetNative().getQueue(
				m_queue_info->family,
				m_queue_info->index,
				*logical_device->GetRawDispatcher()
			)
		),
		m_dispatcher(logical_device->GetDispatcher()) {
	}

	VulkanCommandQueue::VulkanCommandQueue(VulkanLogicalDevice& logical_device, CommandObjectType type, QueuePriority priority)
		: m_logical_device_id(logical_device.GetID()),
		m_queue_info(logical_device.AllocateCommandQueue(type, priority)),
		m_impl(
			logical_device.GetNative().getQueue(
				m_queue_info->family,
				m_queue_info->index,
				*logical_device.GetRawDispatcher()
			)
		),
		m_dispatcher(logical_device.GetDispatcher()) {
	}

	std::uint32_t VulkanCommandQueue::GetFamily() const noexcept {
		return m_queue_info->family;
	}

	std::uint32_t VulkanCommandQueue::GetIndex() const noexcept {
		return m_queue_info->index;
	}

	vk::Queue VulkanCommandQueue::GetNative() const noexcept {
		return m_impl;
	}


}