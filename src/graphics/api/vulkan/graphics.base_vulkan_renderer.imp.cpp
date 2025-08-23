module;
#include <vulkan/vulkan_core.h>
//#include <vulkan/vulkan.hpp>

module graphics:base_vulkan_renderer;
import std;

vk::Bool32 LoggerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
	VkDebugUtilsMessageTypeFlagsEXT message_type, 
	VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
	void* user_data
) {

	auto logger = static_cast<core::ILogger*>(user_data);
	if (!logger) {
		return vk::False;
	}
	core::LogLevel level;
	switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		level = core::LogLevel::TRACE;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		level = core::LogLevel::INFO;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		level = core::LogLevel::WARNING;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		level = core::LogLevel::ERROR;
		break;
	default:
		level = core::LogLevel::DEBUG;
		break;
	}

	std::string type_str;
	if (message_type & static_cast<VkDebugUtilsMessageTypeFlagsEXT>(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)) {
		type_str += "General|";
	}
	if (message_type & static_cast<VkDebugUtilsMessageTypeFlagsEXT>(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)) {
		type_str += "Validation|";
	}
	if (message_type & static_cast<VkDebugUtilsMessageTypeFlagsEXT>(vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)) {
		type_str += "Performance|";
	}
	if (!type_str.empty()) {
		type_str.pop_back();
	}

	logger->Log(
		level,
		std::source_location(),
		"[Vulkan][{}] {}",
		type_str,
		callback_data->pMessage
	);

	return vk::False;

}

namespace graphics::api::vulkan {

	vk::detail::DispatchLoaderDynamic& DynamicLoader(vk::UniqueInstance const& instance) {
		static std::optional<vk::detail::DispatchLoaderDynamic> dynamic_loader;
		static std::once_flag dynamic_loader_initialized;
		std::call_once(
			dynamic_loader_initialized,
			[&instance]() {
				dynamic_loader.emplace(*instance, vkGetInstanceProcAddr);
			}
		);
		return *dynamic_loader;
	}

	FrameContext::FrameContext(FrameContext&& other) noexcept
		: back_buffer_view(std::move(other.back_buffer_view)),
		frame_buffer(std::move(other.frame_buffer)),
		frame_available_semaphore(std::move(other.frame_available_semaphore)),
		render_finished_semaphore(std::move(other.render_finished_semaphore)),
		ready_commands(std::move(other.ready_commands)),
		in_flight_fence(std::move(other.in_flight_fence)) {

	}

	FrameContext& FrameContext::operator=(FrameContext&& other) noexcept {

		if (this != &other) {

			std::scoped_lock locks(ready_commands_mutex, other.ready_commands_mutex);

			back_buffer_view = std::move(other.back_buffer_view);
			frame_buffer = std::move(other.frame_buffer);
			frame_available_semaphore = std::move(other.frame_available_semaphore);
			render_finished_semaphore = std::move(other.render_finished_semaphore);
			ready_commands = std::move(other.ready_commands);
			in_flight_fence = std::move(other.in_flight_fence);

		}

		return *this;

	}

	SwapChainBundle CreateSwapChainBundle(
		VulkanCore const& core,
		platform::IWindow* window
	) {

		/*
		*	Query swap chain support
		*/

		vk::SurfaceCapabilitiesKHR capabilities = core.physical_device.getSurfaceCapabilitiesKHR(*core.surface);
		std::vector<vk::SurfaceFormatKHR> formats = core.physical_device.getSurfaceFormatsKHR(*core.surface);
		std::vector<vk::PresentModeKHR> present_modes = core.physical_device.getSurfacePresentModesKHR(*core.surface);

		/*
		*	Choose surface format
		*/

		vk::SurfaceFormatKHR surface_format;
		if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
			surface_format = vk::SurfaceFormatKHR(vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear);
		}
		else {
			bool found = false;
			for (auto const& format : formats) {
				if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
					surface_format = format;
					found = true;
					break;
				}
			}
			if (!found) {
				surface_format = formats[0]; // Fallback to the first available format
			}
		}

		/*
		*	Choose present mode
		*/

		vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo; // Default to FIFO
		for (auto const& mode : present_modes) {
			if (mode == vk::PresentModeKHR::eMailbox) {
				present_mode = mode; // Prefer Mailbox if available
				break;
			}
			else if (mode == vk::PresentModeKHR::eImmediate) {
				present_mode = mode; // Fallback to Immediate
			}
		}

		/*
		*	Choose extent
		*/

		vk::Extent2D extent;

		if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
			extent = capabilities.currentExtent;
		}
		else {

			auto [width, height] = window->GetWidthAndHeight();

			vk::Extent2D actual_extent = { width, height };

			actual_extent.width = (std::max)(capabilities.minImageExtent.width, (std::min)(capabilities.maxImageExtent.width, actual_extent.width));
			actual_extent.height = (std::max)(capabilities.minImageExtent.height, (std::min)(capabilities.maxImageExtent.height, actual_extent.height));

			extent = actual_extent;
		}

		/*
		*	Create swap chain
		*/

		vk::SwapchainCreateInfoKHR swap_chain_info(
			vk::SwapchainCreateFlagsKHR(),
			*core.surface,
			capabilities.minImageCount + 1, // Use one more than the minimum
			surface_format.format,
			surface_format.colorSpace,
			extent,
			1, // Image array layers
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive, // Single queue family
			0, nullptr,
			capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque, // Opaque composite alpha
			present_mode,
			vk::True, // Clipped
			nullptr // Old swap chain (not used here)
		);

		if (core.graphics_queue_family != core.present_queue_family) {

			std::array queue_family_indices = { *core.graphics_queue_family, *core.present_queue_family };

			swap_chain_info.imageSharingMode = vk::SharingMode::eConcurrent;
			swap_chain_info.queueFamilyIndexCount = static_cast<std::uint32_t>(queue_family_indices.size());
			swap_chain_info.pQueueFamilyIndices = queue_family_indices.data();

		}

		SwapChainBundle swap_chain_bundle = {
			.swap_chain = core.logical_device->createSwapchainKHRUnique(swap_chain_info, core.allocator),
			.back_buffers = core.logical_device->getSwapchainImagesKHR(*(swap_chain_bundle.swap_chain)),
			.format = surface_format.format,
			.extent = extent
		};

		return swap_chain_bundle;

	}

	vk::UniqueRenderPass graphics::api::vulkan::CreateRenderPass(
		VulkanCore const& core, 
		SwapChainBundle const& swap_chain_bundle
	) {

		vk::AttachmentDescription color_attachment(
			vk::AttachmentDescriptionFlags(),
			swap_chain_bundle.format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		);

		vk::AttachmentReference color_attachment_ref(
			0, // attachment index
			vk::ImageLayout::eColorAttachmentOptimal
		);

		vk::SubpassDescription subpass(
			vk::SubpassDescriptionFlags(),
			vk::PipelineBindPoint::eGraphics,
			0, nullptr, // input attachments
			1, &color_attachment_ref, // color attachments
			nullptr, // resolve attachments
			nullptr, // depth-stencil attachment
			0, nullptr // preserve attachments
		);

		vk::SubpassDependency dependency(
			vk::SubpassExternal,
			0,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		);

		vk::RenderPassCreateInfo render_pass_info(
			vk::RenderPassCreateFlags(),
			1, &color_attachment, // attachments
			1, &subpass, // subpasses
			1, &dependency // dependencies
		);

		return core.logical_device->createRenderPassUnique(render_pass_info);
	}

	void BaseVulkanRenderDevice::WaitForLastSubmitted() {
		FrameContext& frame = m_frames[m_previous_frame_index];
		(void)m_core.logical_device->waitForFences(
			1, &(*frame.in_flight_fence), 
			vk::True, 
			std::numeric_limits<std::uint64_t>::max()
		);
	}

	void BaseVulkanRenderDevice::RecreateSwapChain() {
		
		WaitForLastSubmitted();

		m_core.logical_device->waitIdle();

		m_frames.clear();
		m_swap_chain_bundle.back_buffers.clear();
		m_swap_chain_bundle.swap_chain.reset();
		m_render_pass.reset();

		m_swap_chain_bundle = CreateSwapChainBundle(m_core, m_window);
		m_render_pass = CreateRenderPass(m_core, m_swap_chain_bundle);
		CreateFrames();
		m_next_frame_index = GetNextFrameIndex();

	}

	void BaseVulkanRenderDevice::OnCommandReady(CommandReadyEvent const& e) {

		/*
		*	Wait until the present thread allow to submit again
		*/
		while (!m_allow_submission.test(std::memory_order::acquire)) {
			m_allow_submission.wait(false, std::memory_order::relaxed);
		}

		/*
		*	Submit to the command buffer of current frame
		*/

		FrameContext& frame = m_frames[m_next_frame_index];
		std::lock_guard<std::mutex> lock(frame.ready_commands_mutex);
		frame.ready_commands.emplace_back(std::move(*e.command_buffer));

	}

	void BaseVulkanRenderDevice::OnRenderPassInfoRequest(RenderPassInfoRequest const& e) {

		/*
		*	Handle RenderPassInfoRequest event from command object
		*/

		FrameContext& next_frame = m_frames[m_next_frame_index];
		auto& render_pass_begin_info = e.render_pass_begin_info;

		render_pass_begin_info.emplace(
			*m_render_pass,
			*next_frame.frame_buffer
		);

		render_pass_begin_info->renderArea.offset = vk::Offset2D{ 0, 0 };
		render_pass_begin_info->renderArea.extent = m_swap_chain_bundle.extent;

		std::array<vk::ClearValue, 2> clear_values{};
		clear_values[0].color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
		clear_values[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
		render_pass_begin_info->clearValueCount = static_cast<std::uint32_t>(clear_values.size());
		render_pass_begin_info->pClearValues = clear_values.data();

	}

	void BaseVulkanRenderDevice::CreateFrames() {

		auto const& back_buffers = m_swap_chain_bundle.back_buffers;
		std::size_t frame_count = back_buffers.size();
		m_frames.reserve(frame_count);

		for (std::size_t i = 0; i < frame_count; ++i) {

			/*
			*	Create back buffer view
			*/

			vk::ImageViewCreateInfo back_buffer_view_create_info(
				vk::ImageViewCreateFlags(),
				back_buffers[i],
				vk::ImageViewType::e2D,
				m_swap_chain_bundle.format,
				vk::ComponentMapping(), // default component 
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor,
					0, 1, // baseMipLevel, levelCount
					0, 1  // baseArrayLayer, layerCount
				)
			);

			vk::UniqueImageView back_buffer_view = m_core.logical_device->createImageViewUnique(
				back_buffer_view_create_info, m_core.allocator
			);

			/*
			*	Create frame buffer
			*/

			std::array attachments = { *back_buffer_view };

			vk::FramebufferCreateInfo frame_buffer_info(
				vk::FramebufferCreateFlags(),
				*m_render_pass,
				static_cast<std::uint32_t>(attachments.size()), attachments.data(),
				m_swap_chain_bundle.extent.width,
				m_swap_chain_bundle.extent.height,
				1 // layers
			);

			vk::UniqueFramebuffer frame_buffer = m_core.logical_device->createFramebufferUnique(
				frame_buffer_info, m_core.allocator
			);

			/*
			*	Create sync objects
			*/

			vk::SemaphoreCreateInfo semaphore_info;
			vk::UniqueSemaphore next_frame_available_semaphore = m_core.logical_device->createSemaphoreUnique(
				semaphore_info, m_core.allocator
			);
			vk::UniqueSemaphore frame_available_semaphore = m_core.logical_device->createSemaphoreUnique(
				semaphore_info, m_core.allocator
			);
			vk::UniqueSemaphore render_finished_semaphore = m_core.logical_device->createSemaphoreUnique(
				semaphore_info, m_core.allocator
			);

			vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
			vk::UniqueFence in_flight_fence = m_core.logical_device->createFenceUnique(
				fence_info, m_core.allocator
			);

			FrameContext context;
			context.back_buffer_view = std::move(back_buffer_view);
			context.frame_buffer = std::move(frame_buffer);
			context.frame_available_semaphore = std::move(frame_available_semaphore);
			context.render_finished_semaphore = std::move(render_finished_semaphore);
			context.in_flight_fence = std::move(in_flight_fence);

			m_frames.emplace_back(std::move(context));

		}

	}

	std::uint32_t BaseVulkanRenderDevice::GetNextFrameIndex() {

		auto& previous_frame = m_frames[m_previous_frame_index];

		/*
		*	Wait for last commands submission done
		*/
		(void)m_core.logical_device->waitForFences(
			1, &(*previous_frame.in_flight_fence),
			vk::True, std::numeric_limits<std::uint64_t>::max()
		);

		/*
		*	Get next frame
		*/

		auto [_, next_frame_index] = m_core.logical_device->acquireNextImageKHR(
			*m_swap_chain_bundle.swap_chain,
			std::numeric_limits<uint64_t>::max(),
			*previous_frame.frame_available_semaphore,
			nullptr
		);

		return next_frame_index;

	}

	BaseVulkanRenderDevice::BaseVulkanRenderDevice(BaseVulkanRenderDevice&& other) noexcept
		: BaseRenderDevice(std::move(other)),
		m_core(std::move(other.m_core)),
		m_swap_chain_bundle(std::move(other.m_swap_chain_bundle)),
		m_render_pass(std::move(other.m_render_pass)),
		m_frames(std::move(other.m_frames)),
		m_previous_frame_index(std::exchange(other.m_previous_frame_index, 0)),
		m_next_frame_index(std::exchange(other.m_next_frame_index, 0)) {

		/*
		*	clean up previous subscription
		*/

		m_message_bus.Unsubscribe(other.m_command_ready_subscription);
		m_message_bus.Unsubscribe(other.m_render_pass_info_request_subscription);
		other.m_command_ready_subscription = nullptr;
		other.m_render_pass_info_request_subscription = nullptr;

		m_command_ready_subscription = m_message_bus.Subscribe<CommandReadyEvent>(
			[this](CommandReadyEvent const& e) {
				OnCommandReady(e);
			}
		);

		m_render_pass_info_request_subscription = m_message_bus.Subscribe<RenderPassInfoRequest>(
			[this](RenderPassInfoRequest const& e) {
				OnRenderPassInfoRequest(e);
			}
		);
		

	}

	BaseVulkanRenderDevice& BaseVulkanRenderDevice::operator=(BaseVulkanRenderDevice&& other) noexcept {
		
		if (this != &other) {

			WaitForLastSubmitted();
			m_core.logical_device->waitIdle();

			static_cast<BaseRenderDevice&&>(*this) = std::move(static_cast<BaseRenderDevice&&>(other));

			m_core = std::move(other.m_core);
			m_swap_chain_bundle = std::move(other.m_swap_chain_bundle);
			m_render_pass = std::move(other.m_render_pass);
			m_frames = std::move(other.m_frames);
			m_previous_frame_index = std::exchange(other.m_previous_frame_index, 0);
			m_next_frame_index = std::exchange(other.m_next_frame_index, 0);

			/*
			*	clean up previous subscription
			*/

			m_message_bus.Unsubscribe(other.m_command_ready_subscription);
			m_message_bus.Unsubscribe(other.m_render_pass_info_request_subscription);
			other.m_command_ready_subscription = nullptr;
			other.m_render_pass_info_request_subscription = nullptr;

			m_command_ready_subscription = m_message_bus.Subscribe<CommandReadyEvent>(
				[this](CommandReadyEvent const& e) {
					OnCommandReady(e);
				}
			);

			m_render_pass_info_request_subscription = m_message_bus.Subscribe<RenderPassInfoRequest>(
				[this](RenderPassInfoRequest const& e) {
					OnRenderPassInfoRequest(e);
				}
			);

		}

		return *this;

	}

	BaseVulkanRenderDevice::~BaseVulkanRenderDevice() noexcept {

		if (!m_frames.empty()) {
			WaitForLastSubmitted();
			m_core.logical_device->waitIdle();
		}

		m_frames.clear();

	}

	void BaseVulkanRenderDevice::Clear(float r, float g, float b, float a) {

		vk::ClearAttachment clear_attachment(
			vk::ImageAspectFlagBits::eColor,
			0,
			vk::ClearColorValue{ r,g,b,a }
		);

		vk::ClearRect clear_rect(
			{ { 0, 0 }, m_swap_chain_bundle.extent },
			0,
			1
		);

		// command object for the present thread
		auto& command_object = static_cast<VulkanCommandObject&>(AcquireCommandObject());

		auto& command_buffer = command_object.CommandBuffer();
		command_buffer->clearAttachments(1u, &clear_attachment, 1u, &clear_rect);

	}

	void BaseVulkanRenderDevice::SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) {

		vk::Viewport viewport(
			static_cast<float>(x), static_cast<float>(y),
			static_cast<float>(width), static_cast<float>(height),
			0.0f, 1.0f
		);

		vk::Rect2D scissor(
			{ 0, 0 },
			m_swap_chain_bundle.extent
		);

		// command object for the present thread
		auto& command_object = static_cast<VulkanCommandObject&>(AcquireCommandObject());

		auto& command_buffer = command_object.CommandBuffer();
		command_buffer->setViewport(0, 1, &viewport);
		command_buffer->setScissor(0, 1, &scissor);

	}

	bool BaseVulkanRenderDevice::BeginFrame() try {

		if (m_is_minimized || !m_is_window_alive) {
			return false;
		}

		auto& next_frame = m_frames[m_next_frame_index];

		/*
		*	wait for next frame to be ready
		*/
		(void)m_core.logical_device->waitForFences(
			1, &(*next_frame.in_flight_fence),
			vk::True, std::numeric_limits<std::uint64_t>::max()
		);

		/*
		*	reset it
		*/
		(void)m_core.logical_device->resetFences(1, &(*next_frame.in_flight_fence));

		/*
		*	proceed next frame
		*/
		
		// command object for the present thread
		auto& command_object = static_cast<VulkanCommandObject&>(AcquireCommandObject());
		command_object.StartRecording();
		
		/*
		*	notify the awaiting rendering thread
		*/

		m_allow_submission.test_and_set(std::memory_order::release);
		m_allow_submission.notify_all();

		return true;

	}
	catch (vk::OutOfDateKHRError const& ex) {
		if (!m_is_minimized && m_is_window_alive) {
			m_logger->Info(
				std::source_location::current(),
				"{}",
				ex.what()
			);
			RecreateSwapChain();
		}
		return false;
	}
	catch (vk::SystemError const& ex) {
		m_logger->Error(
			std::source_location::current(),
			"vulkan system error {}",
			ex.what()
		);
		return false;
	}

	void BaseVulkanRenderDevice::EndFrame() try {

		FrameContext& next_frame = m_frames[m_next_frame_index];
		FrameContext& previous_frame = m_frames[m_previous_frame_index];

		// command object for the present thread
		auto& command_object = static_cast<VulkanCommandObject&>(AcquireCommandObject());

		/*
		*	End the recording in the present thread.
		*/

		command_object.EndRecording();

		/*
		*	No more submission to this frame
		*/

		m_allow_submission.clear(std::memory_order::release);
		std::vector<vk::CommandBuffer> ready_commands;

		{
			/*
			*	Fetch all commands
			*/

			std::lock_guard<std::mutex> lock(next_frame.ready_commands_mutex);
			ready_commands = std::move(next_frame.ready_commands);

		}

		/*
		*	Present
		*/

		vk::SubmitInfo submit_info{};
		std::array wait_semaphores = { *previous_frame.frame_available_semaphore};
		std::array wait_stages = { vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput) };
		submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
		submit_info.pWaitSemaphores = wait_semaphores.data();
		submit_info.pWaitDstStageMask = wait_stages.data();

		submit_info.commandBufferCount = static_cast<std::uint32_t>(ready_commands.size());
		submit_info.pCommandBuffers = ready_commands.data();

		std::array signal_semaphores = { *next_frame.render_finished_semaphore };
		submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
		submit_info.pSignalSemaphores = signal_semaphores.data();

		m_core.graphics_queue.submit(submit_info, *next_frame.in_flight_fence);

		vk::PresentInfoKHR present_info = {};
		present_info.waitSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
		present_info.pWaitSemaphores = signal_semaphores.data();

		std::array swap_chains = { *m_swap_chain_bundle.swap_chain };
		present_info.pSwapchains = swap_chains.data();
		present_info.swapchainCount = static_cast<std::uint32_t>(swap_chains.size());
		present_info.pImageIndices = &m_next_frame_index;

		(void)m_core.present_queue.presentKHR(present_info);

		m_previous_frame_index = m_next_frame_index;
		m_next_frame_index = GetNextFrameIndex();

	}
	catch (vk::OutOfDateKHRError const& ex) {
		if (!m_is_minimized && m_is_window_alive) {
			m_logger->Info(
				std::source_location::current(),
				"{}",
				ex.what()
			);
			RecreateSwapChain();
		}
	}
	catch (vk::SystemError const& err) {
		m_logger->Error(
			std::source_location::current(),
			"vulkan system error {}",
			err.what()
		);
	}

	ICommandObject& BaseVulkanRenderDevice::AcquireCommandObject() {

		static thread_local std::vector<VulkanCommandObject> command_objects;
		static thread_local std::once_flag command_objects_initialized;
		std::call_once(
			command_objects_initialized,
			[this]() {
				std::size_t frame_count = m_frames.size();
				command_objects.reserve(frame_count);
				for (std::size_t i = 0; i < frame_count; ++i) {
					command_objects.emplace_back(
						m_core.logical_device,
						*m_core.graphics_queue_family,
						&m_message_bus,
						m_core.allocator
					);
				}
			}
		);
		return command_objects[m_next_frame_index];
	}

	API BaseVulkanRenderDevice::GetAPI() const noexcept {
		return API::Vulkan;
	}

	VulkanCore const& BaseVulkanRenderDevice::Core() const noexcept {
		return m_core;
	}

	vk::UniqueRenderPass const& BaseVulkanRenderDevice::RenderPass() const noexcept {
		return m_render_pass;
	}

	DescriptorResourcePool BaseVulkanRenderDevice::CreateDescriptorResourcePool(
		vk::DescriptorType type, 
		std::uint32_t count, 
		vk::ShaderStageFlags stage_flags
	) const {
		return DescriptorResourcePool(m_core.logical_device, type, count, stage_flags, m_core.allocator);
	}

	vk::UniqueDescriptorPool BaseVulkanRenderDevice::CreateDescriptorPool(
		vk::DescriptorType type, 
		std::uint32_t count, 
		vk::ShaderStageFlags stage_flags
	) const {

		vk::DescriptorPoolSize pool_size(type, count);

		vk::DescriptorPoolCreateInfo pool_info(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			count, // maxSets
			pool_size
		);

		return m_core.logical_device->createDescriptorPoolUnique(pool_info, m_core.allocator);

	}

	std::size_t BaseVulkanRenderDevice::FrameCount() const noexcept {
		return m_frames.size();
	}

}