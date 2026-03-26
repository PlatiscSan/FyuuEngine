/* vulkan_logical_device.cppm - Module interface for Vulkan logical device */

// Module declaration: this file defines the VulkanLogicalDevice class as part of the
// fyuu_rhi module partition :vulkan_logical_device.
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory> 
#include <string_view>
#include <optional>
#include <format> 
#endif // !defined(__cpp_lib_modules)
#include <vma/vk_mem_alloc.h>
export module fyuu_rhi:vulkan_logical_device;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import vulkan;
import plastic.other; 
import :types;
import :vulkan_common; 
import :vulkan_types;
import :vulkan_queue_allocator;
import :vulkan_physical_device;

namespace fyuu_rhi::vulkan {

	/**
	 * @class VulkanLogicalDevice
	 * @brief Represents a logical Vulkan device (vk::Device) with associated resources.
	 *
	 * This class encapsulates:
	 * - The actual Vulkan device (vk::UniqueDevice).
	 * - A queue allocator for managing command queues across families.
	 * - A dynamic dispatch loader for Vulkan functions (supports extensions and core versions).
	 * - A Vulkan Memory Allocator (VMA) handle for GPU memory management.
	 */
	export class VulkanLogicalDevice
		: public PolymorphicLogicalDeviceBase,
		public VulkanCommon<VulkanLogicalDevice> {
	private:
		// Type alias for a unique_ptr that automatically destroys the VMA allocator.
		using UniqueVMAAllocator = std::unique_ptr<std::remove_pointer_t<VmaAllocator>, decltype(&vmaDestroyAllocator)>;

		// Queue allocator: handles queue family selection and queue allocation.
		VulkanQueueAllocator m_queue_alloc;

		// The actual Vulkan device (unique handle, automatically destroyed).
		vk::UniqueDevice m_impl;

		// Shared pointer to the dynamic dispatch loader.
		// This loader contains function pointers for all used Vulkan commands,
		// including those from extensions. It is shared to allow other components
		// (e.g., command buffers) to call Vulkan functions without storing their own.
		std::shared_ptr<vk::detail::DispatchLoaderDynamic> m_dispatcher;

		// Vulkan Memory Allocator handle for GPU memory allocations.
		UniqueVMAAllocator m_video_memory_alloc;

		std::size_t m_vma_tick;

	public:
		/**
		 * @brief Constructs a VulkanLogicalDevice from a given physical device.
		 *
		 * The constructor performs the following steps:
		 * 1. Queries required features and ensures they are supported.
		 * 2. Sets up queue creation infos based on the queue allocator.
		 * 3. Creates the Vulkan device with the required extensions and features.
		 * 4. Initializes the dynamic dispatch loader for device-level functions.
		 * 5. Creates the VMA allocator.
		 *
		 * @param physical_device The physical device (GPU) to use.
		 * @param queue_options   Options for queue allocation (e.g., number of queues, priorities).
		 */
		VulkanLogicalDevice(
			VulkanPhysicalDevice const& physical_device,
			VulkanQueueOptions const& queue_options = VulkanQueueOptions::PlatformDefault()
		);

		/**
		 * @brief Assigns a debug name to a Vulkan resource (object) using vkSetDebugUtilsObjectNameEXT.
		 *
		 * Only active in debug builds; in release builds this is a no-op.
		 *
		 * @param resource   Pointer to the Vulkan object (e.g., vk::Buffer, vk::Image).
		 * @param type       Vulkan object type (e.g., vk::ObjectType::eBuffer).
		 * @param debug_name The name to assign.
		 */
		void SetDebugNameForResource(void const* resource, vk::ObjectType type, std::string_view debug_name) const;

		/// @return The native Vulkan device handle (vk::Device).
		vk::Device GetNative() const noexcept;

		/// @return Shared pointer to the dynamic dispatch loader.
		std::shared_ptr<vk::detail::DispatchLoaderDynamic> GetDispatcher() const noexcept;

		/// @return Reference to the dynamic dispatch loader.
		vk::detail::DispatchLoaderDynamic const& GetRawDispatcher() const noexcept;

		/// @return The VMA allocator handle.
		VmaAllocator GetVideoMemoryAllocator() const noexcept;

		/**
		 * @brief Allocates a command queue (logical queue) of the specified type and priority.
		 *
		 * @param type      The type of commands the queue will handle (e.g., graphics, compute, copy).
		 * @param priority  The priority level for the queue.
		 * @return A unique handle representing the allocated queue.
		 */
		UniqueVulkanCommandQueueInfo AllocateCommandQueue(CommandObjectType type, QueuePriority priority) noexcept;

		/**
		 * @brief Maps a command object type to the corresponding queue family index.
		 *
		 * @param type The command object type.
		 * @return The queue family index that supports this type.
		 */
		std::uint32_t CommandObjectTypeToQueueFamily(CommandObjectType type) const noexcept;

		/**
		 * @brief Creates a binary semaphore (VkSemaphore) with optional debug name.
		 *
		 * Binary semaphores are used for GPU-GPU synchronization (e.g., between command buffers).
		 *
		 * @param debug_name Optional debug name.
		 * @return A unique handle to the semaphore.
		 */
		vk::UniqueSemaphore CreateBinarySemaphore(std::string_view debug_name = {}) const;

		/**
		 * @brief Creates a timeline semaphore with optional debug name.
		 *
		 * Timeline semaphores allow waiting for specific counter values.
		 *
		 * @param debug_name Optional debug name.
		 * @return A unique handle to the semaphore.
		 */
		vk::UniqueSemaphore CreateTimelineSemaphore(std::string_view debug_name = {}) const;

		/**
		 * @brief Waits for one or more semaphores to reach specific values.
		 *
		 * @param info    Structure describing which semaphores and values to wait for.
		 * @param timeout Timeout in nanoseconds (default: infinite).
		 * @throws std::runtime_error if waiting fails or times out.
		 */
		void WaitForSemaphores(vk::SemaphoreWaitInfo const& info, std::uint64_t timeout = (std::numeric_limits<std::uint64_t>::max)()) const;

		/**
		 * @brief Creates a buffer with the given creation info.
		 *
		 * @param info Buffer creation parameters.
		 * @return A unique handle to the buffer.
		 */
		vk::UniqueBuffer CreateBuffer(vk::BufferCreateInfo const& info, std::string_view debug_name = {}) const;

		/**
		 * @brief Creates an image (texture) with the given creation info.
		 *
		 * @param info Image creation parameters.
		 * @return A unique handle to the image.
		 */
		vk::UniqueImage CreateTexture(vk::ImageCreateInfo const& info, std::string_view debug_name = {}) const;

		/**
		 * @brief Queries memory requirements for a buffer.
		 *
		 * @param buffer The buffer handle.
		 * @return Memory requirements structure.
		 */
		vk::MemoryRequirements QueryMemoryRequirements(vk::Buffer const& buffer) const noexcept;

		/**
		 * @brief Queries memory requirements for an image.
		 *
		 * @param texture The image handle.
		 * @return Memory requirements structure.
		 */
		vk::MemoryRequirements QueryMemoryRequirements(vk::Image const& texture) const noexcept;

		/**
		 * @brief Binds device memory to a buffer.
		 *
		 * @param buffer The buffer handle.
		 * @param memory The device memory handle.
		 * @param offset Byte offset into the memory.
		 */
		void BindMemory(vk::Buffer buffer, vk::DeviceMemory memory, std::size_t offset) const;

		/**
		 * @brief Binds device memory to an image.
		 *
		 * @param texture The image handle.
		 * @param memory  The device memory handle.
		 * @param offset  Byte offset into the memory.
		 */
		void BindMemory(vk::Image texture, vk::DeviceMemory memory, std::size_t offset) const;

		vk::UniqueSwapchainKHR CreateSwapChain(vk::SwapchainCreateInfoKHR const& info, std::string_view debug_name = {}) const;

		std::vector<vk::Image> GetSwapChainBackBuffers(vk::SwapchainKHR const& swap_chain) const;

		vk::UniqueImageView CreateTextureView(vk::ImageViewCreateInfo const& info, std::string_view debug_name = {}) const;

		vk::UniqueBufferView CreateBufferView(vk::BufferViewCreateInfo const& info, std::string_view debug_name = {}) const;

		vk::UniqueFence CreateFence(std::string_view debug_name = {}) const;

		void WaitForFence(vk::Fence& fence, std::uint64_t timeout = (std::numeric_limits<std::uint64_t>::max)()) const;

		void ResetFence(vk::Fence& fence) const;

		std::optional<std::size_t> AcquireNextBackBufferIndex(vk::SwapchainKHR& swap_chain, vk::Semaphore& image_ready_semaphore, vk::Fence& image_ready_fence, std::uint64_t timeout = (std::numeric_limits<std::uint64_t>::max)()) const; 

		vk::UniqueShaderModule CreateShader(vk::ShaderModuleCreateInfo const& info) const;

		vk::UniqueSampler CreateSampler(vk::SamplerCreateInfo const& info) const;

		vk::UniqueDescriptorSetLayout CreateDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const& info) const;

		vk::UniquePipelineLayout CreatePipelineLayout(vk::PipelineLayoutCreateInfo const& info) const;

		vk::UniquePipeline CreatePipeline(vk::PipelineCache cache, vk::GraphicsPipelineCreateInfo const& info) const;

		vk::UniquePipeline CreatePipeline(vk::PipelineCache cache, vk::ComputePipelineCreateInfo const& info) const;

		vk::UniqueCommandPool CreateCommandPool(CommandObjectType type, std::string_view debug_name = {}) const;

		vk::UniqueCommandBuffer CreateCommandBuffer(vk::CommandPool const& pool, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, std::string_view debug_name = {}) const;

		void ResetCommandPool(vk::CommandPool pool, vk::CommandPoolResetFlags flags) const;

		void VMATick();

	};

} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)