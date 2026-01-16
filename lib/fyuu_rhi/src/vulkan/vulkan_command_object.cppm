export module fyuu_rhi:vulkan_command_object;
import std;
import vulkan_hpp;
import plastic.any_pointer;
import plastic.registrable;
import plastic.unique_resource;
import :command_object;
import :vulkan_declaration;
import :vulkan_queue_allocator;

namespace fyuu_rhi::vulkan {

	export class VulkanCommandBuffer 
		: public ICommandBuffer {
	private:
		/// @brief for single time use
		vk::UniqueCommandPool m_allocator;
		vk::UniqueCommandBuffer m_impl;

	public:
		VulkanCommandBuffer() = default;
		VulkanCommandBuffer(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			std::uint32_t family
		);

		VulkanCommandBuffer(
			VulkanLogicalDevice const& logical_device,
			std::uint32_t family
		);

		VulkanCommandBuffer(vk::UniqueCommandBuffer&& vk_command_buffer);
		VulkanCommandBuffer(vk::CommandBuffer vk_command_buffer);
		vk::UniqueCommandBuffer Release() noexcept;
		vk::CommandBuffer ReleaseNative() noexcept;
		vk::CommandBuffer GetNative() const noexcept;
	};

	export class VulkanCommandAllocator
		: public plastic::utility::Registrable<VulkanCommandAllocator>,
		public ICommandAllocator {
	private:
		vk::UniqueCommandPool m_impl;
		std::vector<vk::UniqueCommandBuffer> m_command_buffers;
		std::mutex m_command_buffer_mutex;
		std::condition_variable m_command_buffer_cv;

	public:
		class CommandBufferGC;
		using UniqueCommandBuffer = plastic::utility::UniqueResource<VulkanCommandBuffer, CommandBufferGC>;

		VulkanCommandAllocator(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			std::uint32_t family,
			std::size_t capacity
		);

		UniqueCommandBuffer Allocate();

	};

	class VulkanCommandAllocator::CommandBufferGC {
	private:
		std::size_t m_allocator_id;

	public:
		CommandBufferGC(std::size_t allocator_id) noexcept;
		CommandBufferGC(CommandBufferGC const& other) noexcept;
		CommandBufferGC(CommandBufferGC&& other) noexcept;
		void operator()(VulkanCommandBuffer& command_buffer) noexcept;
	};

	export class VulkanCommandQueue
		: public plastic::utility::Registrable<VulkanCommandQueue>,
		public plastic::utility::EnableSharedFromThis<VulkanCommandQueue>,
		public ICommandQueue {
		friend class ICommandQueue;
	private:
		std::size_t m_logical_device_id;
		UniqueVulkanCommandQueueInfo m_queue_info;
		vk::Queue m_impl;
		std::shared_ptr<vk::DispatchLoaderDynamic> m_dispatcher;

		void WaitUntilCompletedImpl() const;

		void ExecuteCommandImpl(VulkanCommandBuffer& command);

	public:
		VulkanCommandQueue(
			plastic::utility::AnyPointer<VulkanLogicalDevice> const& logical_device,
			CommandObjectType type,
			QueuePriority priority
		);

		VulkanCommandQueue(
			VulkanLogicalDevice& logical_device,
			CommandObjectType type,
			QueuePriority priority
		);

		std::uint32_t GetFamily() const noexcept;
		std::uint32_t GetIndex() const noexcept;

		vk::Queue GetNative() const noexcept;

	};

}