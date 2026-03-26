/* vulkan_logical_device.impl.cpp - Module implementation for Vulkan logical device */

// This file provides the implementation of VulkanLogicalDevice.
module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <array>
#include <vector>
#include <string_view>
#include <optional>
#include <format>
#include <print>
#endif // !defined(__cpp_lib_modules)

#include <vma/vk_mem_alloc.h>

module fyuu_rhi:vulkan_logical_device_impl;

#if !defined(__APPLE__)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_logical_device;
import vulkan;
import plastic.other;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_queue_allocator;
import :vulkan_physical_device;

namespace fyuu_rhi::vulkan {

	// ----------------------------------------------------------------------------
	// Constructor: creates the Vulkan device, queue allocator, dispatcher, and VMA.
	// ----------------------------------------------------------------------------
	VulkanLogicalDevice::VulkanLogicalDevice(
		VulkanPhysicalDevice const& physical_device,
		VulkanQueueOptions const& queue_options
	) : PolymorphicLogicalDeviceBase(this),
		VulkanCommon(this),
		// Step 1: Initialize queue allocator with physical device's queue families and user options.
		m_queue_alloc(physical_device.QueryAllCommandQueueInfo(), queue_options),

		// Step 2: Create the Vulkan device.
		m_impl(
			[this, &physical_device]() {
				// --------------------------------------------------------------------
				// Feature structure chain setup.
				// We need to enable specific Vulkan 1.1, 1.2, 1.3, 1.4 and extension features.
				// The chain is built from the newest (1.4) down to the oldest.
				// --------------------------------------------------------------------

				// Extension-specific features:

				vk::PhysicalDeviceGraphicsPipelineLibraryFeaturesEXT lib_features{};
				vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT fse_features{};
				vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_maintenance1_features{};
				vk::PhysicalDevicePresentModeFifoLatestReadyFeaturesEXT present_mode_fifo_latest_ready_features{};
				vk::PhysicalDeviceDepthClipEnableFeaturesEXT depth_clip_enable_features{};
				vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT eds_features{};
				vk::PhysicalDeviceRobustness2FeaturesEXT robustness2_features{};

				// Core feature structures (Vulkan 1.4, 1.3, 1.2, 1.1):

				vk::PhysicalDeviceVulkan14Features vulkan1_4_features{};
				vk::PhysicalDeviceVulkan13Features vulkan1_3_features{};
				vk::PhysicalDeviceVulkan12Features vulkan1_2_features{};
				vk::PhysicalDeviceVulkan11Features vulkan1_1_features{};
				vk::PhysicalDeviceCustomBorderColorFeaturesEXT custom_border_color{};
		
				
				// Chain them:	custom_border_color -> vulkan1_1 -> vulkan1_2 -> vulkan1_3 -> vulkan1_4 
				//				-> fse_features -> lib_features -> swapchain_maintenance1_features 
				//				-> present_mode_fifo_latest_ready_features -> depth_clip_enable_features
				//				-> eds_featuresv-> robustness2_features
				custom_border_color.setPNext(&vulkan1_1_features);
				vulkan1_1_features.setPNext(&vulkan1_2_features);
				vulkan1_2_features.setPNext(&vulkan1_3_features);
				vulkan1_3_features.setPNext(&vulkan1_4_features);
				vulkan1_4_features.setPNext(&fse_features);
				fse_features.setPNext(&lib_features);
				lib_features.setPNext(&swapchain_maintenance1_features);
				swapchain_maintenance1_features.setPNext(&present_mode_fifo_latest_ready_features);
				present_mode_fifo_latest_ready_features.setPNext(&depth_clip_enable_features);
				depth_clip_enable_features.setPNext(&eds_features);
				eds_features.setPNext(&robustness2_features);

				// Main feature structure (passed to vkCreateDevice).
				vk::PhysicalDeviceFeatures2 device_features2{};
				device_features2.setPNext(&custom_border_color);

				// Query the physical device to fill in the supported features.
				physical_device.GetFeatures(device_features2);

				// Validate that all required features are supported.
				if (vulkan1_2_features.imagelessFramebuffer == vk::False) {
					throw std::runtime_error("Imageless framebuffer is not supported");
				}
				if (vulkan1_3_features.dynamicRendering == vk::False) {
					throw std::runtime_error("Dynamic rendering is not supported");
				}
				if (vulkan1_2_features.scalarBlockLayout == vk::False) {
					throw std::runtime_error("ScalarBlockLayout is not supported");
				}
				if (custom_border_color.customBorderColors == vk::False) {
					throw std::runtime_error("CustomBorderColor is not supported");
				}
				if (vulkan1_2_features.samplerFilterMinmax == vk::False) {
					throw std::runtime_error("Minmax Sampler is not supported");
				}
				if (vulkan1_1_features.shaderDrawParameters == vk::False) {
					throw std::runtime_error("Shader Draw Parameters (baseInstance et al) are not supported.");
				}
				if (fse_features.fragmentShaderPixelInterlock == vk::False) {
					throw std::runtime_error("Fragment Shader Interlock is not supported");
				}
				if (lib_features.graphicsPipelineLibrary == vk::False) {
					throw std::runtime_error("Graphics Pipeline Library is not supported");
				}
				if (swapchain_maintenance1_features.swapchainMaintenance1 == vk::False) {
					throw std::runtime_error("VK_KHR_swapchain_maintenance1 is not supported");
				}
				if (present_mode_fifo_latest_ready_features.presentModeFifoLatestReady == vk::False) {
					throw std::runtime_error("VK_KHR_present_mode_fifo_latest_ready is not supported");
				}
				if (depth_clip_enable_features.depthClipEnable == vk::False) {
					throw std::runtime_error("VK_EXT_depth_clip_enable is not supported");
				}
				if (eds_features.extendedDynamicState == vk::False) {
					throw std::runtime_error("VK_EXT_extended_dynamic_state is not supported");
				}
				if (robustness2_features.nullDescriptor == vk::False) {
					throw std::runtime_error("VK_EXT_robustness2 is not supported");
				}

				// List of required device extensions.
				constexpr static char const* const device_extensions[] = {
					vk::KHRSwapchainExtensionName,				// Swap chain support
					vk::EXTSwapchainMaintenance1ExtensionName,		// Modern swap chain
					vk::EXTPresentModeFifoLatestReadyExtensionName,
					vk::KHRPushDescriptorExtensionName,		   // Push descriptors
					vk::EXTCustomBorderColorExtensionName,		// Custom border colors
					vk::EXTFragmentShaderInterlockExtensionName, // Fragment shader interlock
					vk::KHRPipelineLibraryExtensionName,		 // Pipeline libraries (for GPL)
					vk::EXTGraphicsPipelineLibraryExtensionName, // Graphics pipeline library
					vk::EXTDepthClipEnableExtensionName,
					vk::EXTExtendedDynamicState3ExtensionName,
#if !defined(__ANDROID__)	// Only ~5% of Android devices have this extension, so we skip it on Android.
					vk::EXTMemoryBudgetExtensionName			 // Memory budget query
#endif
				};

				// --------------------------------------------------------------------
				// Queue creation.
				// We create one queue family per command type (graphics, compute, copy)
				// as determined by the queue allocator. The allocator provides the
				// family index, number of queues, and queue priorities.
				// --------------------------------------------------------------------
				std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

				// Graphics (and compute) queue family.
				queue_create_infos.emplace_back(
					vk::DeviceQueueCreateFlags{},
					m_queue_alloc.GetFamily(CommandObjectType::AllCommands),
					m_queue_alloc.GetTotalQueue(CommandObjectType::AllCommands),
					m_queue_alloc.GetPriority(CommandObjectType::AllCommands).data(),
					nullptr
				);

				// Dedicated compute queue family (if separate from graphics).
				queue_create_infos.emplace_back(
					vk::DeviceQueueCreateFlags{},
					m_queue_alloc.GetFamily(CommandObjectType::Compute),
					m_queue_alloc.GetTotalQueue(CommandObjectType::Compute),
					m_queue_alloc.GetPriority(CommandObjectType::Compute).data(),
					nullptr
				);

				// Dedicated copy/transfer queue family.
				queue_create_infos.emplace_back(
					vk::DeviceQueueCreateFlags{},
					m_queue_alloc.GetFamily(CommandObjectType::Copy),
					m_queue_alloc.GetTotalQueue(CommandObjectType::Copy),
					m_queue_alloc.GetPriority(CommandObjectType::Copy).data(),
					nullptr
				);

				// Device creation info structure.
				vk::DeviceCreateInfo device_create_info(
					{},
					queue_create_infos,
					{},
					device_extensions,
					nullptr,			// We use pNext chain instead.
					&device_features2
				);

				// Create the logical device.
				return physical_device.CreateLogicalDevice(device_create_info);

			}()),

		// Step 3: Initialize the dynamic dispatch loader.
		m_dispatcher(
			[this]() {
				// Start with the instance-level dispatcher (already has global and instance functions).
				auto dispatcher = std::make_shared<vk::detail::DispatchLoaderDynamic>(
					VulkanPhysicalDevice::InstanceLevelDispatcher()
				);
				// Add device-level functions.
				dispatcher->init(*m_impl);
				return dispatcher;
			}()),

		// Step 4: Create the Vulkan Memory Allocator (VMA).
		m_video_memory_alloc(
			[this, &physical_device]() {
				// Provide VMA with the necessary function pointers.
				VmaVulkanFunctions vulkan_functions = {};
				vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
				vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

				VmaAllocatorCreateInfo allocator_create_info = {};
				allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT; // Enable memory budget extension.
				allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;			   // Specify Vulkan API version.
				allocator_create_info.physicalDevice = physical_device.GetNative();		// Physical device.
				allocator_create_info.device = *m_impl;									// Logical device.
				allocator_create_info.instance = physical_device.GetInstance();			// Vulkan instance.
				allocator_create_info.pVulkanFunctions = &vulkan_functions;				// Function pointers.

				VmaAllocator allocator;
				vmaCreateAllocator(&allocator_create_info, &allocator);

				// Wrap in unique_ptr with custom deleter.
				return UniqueVMAAllocator(allocator, vmaDestroyAllocator);
			}()),
		m_vma_tick(0u) {

	}

	// ----------------------------------------------------------------------------
	// SetDebugNameForResource: assign a debug name to a Vulkan object.
	// ----------------------------------------------------------------------------
	void VulkanLogicalDevice::SetDebugNameForResource(void const* resource, vk::ObjectType type, std::string_view debug_name) const {
#if !defined(_NDEBUG)
		// Only assign names in debug builds to avoid overhead.
		vk::DebugUtilsObjectNameInfoEXT object_name(
			type,
			reinterpret_cast<std::uint64_t>(resource),
			debug_name.data()
		);
		// Use the dynamic dispatcher to call the extension function.
		m_impl->setDebugUtilsObjectNameEXT(object_name, *m_dispatcher);
#endif // !defined(_NDEBUG)
	}

	// ----------------------------------------------------------------------------
	// Simple getters.
	// ----------------------------------------------------------------------------
	vk::Device VulkanLogicalDevice::GetNative() const noexcept {
		return *m_impl;
	}

	std::shared_ptr<vk::detail::DispatchLoaderDynamic> VulkanLogicalDevice::GetDispatcher() const noexcept {
		return m_dispatcher;
	}

	vk::detail::DispatchLoaderDynamic const& VulkanLogicalDevice::GetRawDispatcher() const noexcept {
		return *m_dispatcher;
	}

	VmaAllocator VulkanLogicalDevice::GetVideoMemoryAllocator() const noexcept {
		return m_video_memory_alloc.get();
	}

	// ----------------------------------------------------------------------------
	// Queue allocation.
	// ----------------------------------------------------------------------------
	UniqueVulkanCommandQueueInfo VulkanLogicalDevice::AllocateCommandQueue(CommandObjectType type, QueuePriority priority) noexcept {
		return m_queue_alloc.Allocate(type, priority);
	}

	std::uint32_t VulkanLogicalDevice::CommandObjectTypeToQueueFamily(CommandObjectType type) const noexcept {
		return m_queue_alloc.GetFamily(type);
	}

	// ----------------------------------------------------------------------------
	// Semaphore creation.
	// ----------------------------------------------------------------------------
	vk::UniqueSemaphore VulkanLogicalDevice::CreateBinarySemaphore(std::string_view debug_name) const {
		vk::SemaphoreCreateInfo semaphore_info{};
		auto semaphore = m_impl->createSemaphoreUnique(semaphore_info, nullptr, *m_dispatcher);
		if (!debug_name.empty()) {
			SetDebugNameForResource(semaphore.get(), semaphore->objectType, debug_name);
		}
		return semaphore;
	}

	vk::UniqueSemaphore VulkanLogicalDevice::CreateTimelineSemaphore(std::string_view debug_name) const {
		// Timeline semaphores require VK_KHR_timeline_semaphore (core in 1.2) and a type info.
		vk::SemaphoreTypeCreateInfo timeline_info(
			vk::SemaphoreType::eTimeline,
			0ull	// initial value
		);
		vk::SemaphoreCreateInfo semaphore_info(
			vk::SemaphoreCreateFlags{},
			&timeline_info
		);
		auto semaphore = m_impl->createSemaphoreUnique(semaphore_info, nullptr, *m_dispatcher);
		if (!debug_name.empty()) {
			SetDebugNameForResource(semaphore.get(), semaphore->objectType, debug_name);
		}
		return semaphore;
	}

	// ----------------------------------------------------------------------------
	// Semaphore wait.
	// ----------------------------------------------------------------------------
	void VulkanLogicalDevice::WaitForSemaphores(vk::SemaphoreWaitInfo const& info, std::uint64_t timeout) const {
		vk::Result result = m_impl->waitSemaphores(info, timeout, *m_dispatcher);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::waitSemaphores failed or timed out, result: {}", static_cast<std::size_t>(result)));
		}
	}

	// ----------------------------------------------------------------------------
	// Buffer and image creation.
	// ----------------------------------------------------------------------------
	vk::UniqueBuffer VulkanLogicalDevice::CreateBuffer(vk::BufferCreateInfo const& info, std::string_view debug_name) const {
		auto buffer = m_impl->createBufferUnique(info, nullptr, *m_dispatcher);
		if (!debug_name.empty()) {
			SetDebugNameForResource(buffer.get(), buffer->objectType, debug_name);
		}
		return buffer;
	}

	vk::UniqueImage VulkanLogicalDevice::CreateTexture(vk::ImageCreateInfo const& info, std::string_view debug_name) const {
		auto texture = m_impl->createImageUnique(info, nullptr, *m_dispatcher);
		if (!debug_name.empty()) {
			SetDebugNameForResource(texture.get(), texture->objectType, debug_name);
		}
		return texture;
	}

	// ----------------------------------------------------------------------------
	// Memory requirements queries.
	// ----------------------------------------------------------------------------
	vk::MemoryRequirements VulkanLogicalDevice::QueryMemoryRequirements(vk::Buffer const& buffer) const noexcept {
		return m_impl->getBufferMemoryRequirements(buffer, *m_dispatcher);
	}

	vk::MemoryRequirements VulkanLogicalDevice::QueryMemoryRequirements(vk::Image const& texture) const noexcept {
		return m_impl->getImageMemoryRequirements(texture, *m_dispatcher);
	}

	// ----------------------------------------------------------------------------
	// Binding memory to buffers/images.
	// ----------------------------------------------------------------------------
	void VulkanLogicalDevice::BindMemory(vk::Buffer buffer, vk::DeviceMemory memory, std::size_t offset) const {
		m_impl->bindBufferMemory(buffer, memory, offset, *m_dispatcher);
	}

	void VulkanLogicalDevice::BindMemory(vk::Image texture, vk::DeviceMemory memory, std::size_t offset) const {
		m_impl->bindImageMemory(texture, memory, offset, *m_dispatcher);
	}

	vk::UniqueSwapchainKHR VulkanLogicalDevice::CreateSwapChain(vk::SwapchainCreateInfoKHR const& info, std::string_view debug_name) const {
		auto swap_chain = m_impl->createSwapchainKHRUnique(
			info,
			nullptr,
			*m_dispatcher
		);
		if (!debug_name.empty()) {
			SetDebugNameForResource(swap_chain.get(), swap_chain->objectType, debug_name);
		}
		return swap_chain;
	}

	std::vector<vk::Image> VulkanLogicalDevice::GetSwapChainBackBuffers(vk::SwapchainKHR const& swap_chain) const {
		return m_impl->getSwapchainImagesKHR(
			swap_chain, 
			*m_dispatcher
		);
	}

	vk::UniqueImageView VulkanLogicalDevice::CreateTextureView(vk::ImageViewCreateInfo const& info, std::string_view debug_name) const {
		auto view = m_impl->createImageViewUnique(
			info,
			nullptr,
			*m_dispatcher
		);
		if (!debug_name.empty()) {
			SetDebugNameForResource(view.get(), view->objectType, debug_name);
		}
		return view;
	}

	vk::UniqueBufferView VulkanLogicalDevice::CreateBufferView(vk::BufferViewCreateInfo const& info, std::string_view debug_name) const {
		auto view = m_impl->createBufferViewUnique(
			info,
			nullptr,
			*m_dispatcher
		);
		if (!debug_name.empty()) {
			SetDebugNameForResource(view.get(), view->objectType, debug_name);
		}
		return view;
	}

	vk::UniqueFence VulkanLogicalDevice::CreateFence(std::string_view debug_name) const {
		auto fence = m_impl->createFenceUnique(
			{},
			nullptr,
			*m_dispatcher
		);
		if (!debug_name.empty()) {
			SetDebugNameForResource(fence.get(), fence->objectType, debug_name);
		}
		return fence;
	}

	void VulkanLogicalDevice::WaitForFence(vk::Fence& fence, std::uint64_t timeout) const {
		vk::Result result = m_impl->waitForFences(1u, &fence, vk::True, timeout, *m_dispatcher);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::waitForFences failed or timed out, result: {}", static_cast<std::size_t>(result)));
		}
	}

	void VulkanLogicalDevice::ResetFence(vk::Fence& fence) const {
		vk::Result result = m_impl->resetFences(1u, &fence, *m_dispatcher);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::resetFences failed, result: {}", static_cast<std::size_t>(result)));
		}
	}

	std::optional<std::size_t> VulkanLogicalDevice::AcquireNextBackBufferIndex(vk::SwapchainKHR& swap_chain, vk::Semaphore& image_ready_semaphore, vk::Fence& image_ready_fence, std::uint64_t timeout) const {
		auto [result, index] = m_impl->acquireNextImageKHR(
			swap_chain,
			timeout,
			image_ready_semaphore,
			image_ready_fence,
			*m_dispatcher
		);

		if (result == vk::Result::eErrorOutOfDateKHR) {
			return std::nullopt;
		}

		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::acquireNextImageKHR failed or timed out, result: {}", static_cast<std::size_t>(result)));
		}

		return index;

	}

	vk::UniqueShaderModule VulkanLogicalDevice::CreateShader(vk::ShaderModuleCreateInfo const& info) const {
		return m_impl->createShaderModuleUnique(info, nullptr, *m_dispatcher);
	}

	vk::UniqueSampler VulkanLogicalDevice::CreateSampler(vk::SamplerCreateInfo const &info) const {
		return m_impl->createSamplerUnique(info, nullptr, *m_dispatcher);
	}

	vk::UniqueDescriptorSetLayout VulkanLogicalDevice::CreateDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo const& info) const {
		return m_impl->createDescriptorSetLayoutUnique(info, nullptr, *m_dispatcher);
	}

	vk::UniquePipelineLayout VulkanLogicalDevice::CreatePipelineLayout(vk::PipelineLayoutCreateInfo const& info) const {
		return m_impl->createPipelineLayoutUnique(info, nullptr, *m_dispatcher);
	}

	vk::UniquePipeline VulkanLogicalDevice::CreatePipeline(vk::PipelineCache cache, vk::GraphicsPipelineCreateInfo const& info) const {
		auto&& [result, pipeline] = m_impl->createGraphicsPipelineUnique(cache, info, nullptr, *m_dispatcher);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::createGraphicsPipelineUnique failed, result: {}", static_cast<std::size_t>(result)));
		}

		return std::move(pipeline);
	}

	vk::UniquePipeline VulkanLogicalDevice::CreatePipeline(vk::PipelineCache cache, vk::ComputePipelineCreateInfo const& info) const {
		auto&& [result, pipeline] = m_impl->createComputePipelineUnique(cache, info, nullptr, *m_dispatcher);
		if (result != vk::Result::eSuccess) {
			throw std::runtime_error(std::format("vk::Device::createGraphicsPipelineUnique failed, result: {}", static_cast<std::size_t>(result)));
		}

		return std::move(pipeline);
	}

	vk::UniqueCommandPool VulkanLogicalDevice::CreateCommandPool(CommandObjectType type, std::string_view debug_name) const {
		vk::CommandPoolCreateInfo info(
			{},
			m_queue_alloc.GetFamily(type)
		);

		auto pool = m_impl->createCommandPoolUnique(info, nullptr, *m_dispatcher);

		if (!debug_name.empty()) {
			SetDebugNameForResource(pool.get(), pool->objectType, debug_name);
		}
	
		return pool;

	}

	vk::UniqueCommandBuffer VulkanLogicalDevice::CreateCommandBuffer(vk::CommandPool const& pool, vk::CommandBufferLevel level, std::string_view debug_name) const {
		
		vk::CommandBufferAllocateInfo alloc_info(
			pool,
			level,
			1u
		);
	
		auto buffers = m_impl->allocateCommandBuffersUnique(alloc_info, *m_dispatcher);
	
		auto cmd_buffer = std::move(buffers.front());

		if (!debug_name.empty()) {
			SetDebugNameForResource(cmd_buffer.get(), cmd_buffer->objectType, debug_name);
		}
	
		return cmd_buffer;

	}

	void VulkanLogicalDevice::ResetCommandPool(vk::CommandPool pool, vk::CommandPoolResetFlags flags) const {
		m_impl->resetCommandPool(pool, flags, *m_dispatcher);
	}

	void VulkanLogicalDevice::VMATick() {
		vmaSetCurrentFrameIndex(m_video_memory_alloc.get(), m_vma_tick++);
	}

} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)