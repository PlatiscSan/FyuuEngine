module;
#include <vulkan/vulkan_core.h>

module vulkan_backend:renderer;
import vulkan_hpp;
import :command_object;
import concurrent_hash_map;
import defer;

namespace fyuu_engine::vulkan {

	using ThreadCommandObjects = concurrency::ConcurrentHashMap<std::thread::id, VulkanCommandObject>;
	using InstanceCommandObjects = concurrency::ConcurrentHashMap<void*, ThreadCommandObjects>;
	static InstanceCommandObjects s_command_objects;

	vk::Bool32 LoggerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
		VkDebugUtilsMessageTypeFlagsEXT message_type, 
		VkDebugUtilsMessengerCallbackDataEXT const* callback_data, 
		void* user_data
	) {
		auto logger_ptr = static_cast<core::IRendererLogger*>(user_data);
		if (!logger_ptr) {
			return vk::False;
		}

		core::LogLevel level;
		switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity)) {
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
			level = core::LogLevel::Trace;
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			level = core::LogLevel::Info;
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			level = core::LogLevel::Warning;
			break;
		case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			level = core::LogLevel::Error;
			break;
		default:
			level = core::LogLevel::Debug;
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

		std::string formatted_message = std::format("[Vulkan][{}] {}", type_str, callback_data->pMessage);

		static constexpr std::source_location dummy_loc;

		logger_ptr->Log(level, formatted_message, dummy_loc);

		return vk::False;
	}

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

	SwapChainBundle CreateSwapChainBundle(
		VulkanCore const& core,
		std::uint32_t width,
		std::uint32_t height
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
			.format = surface_format.format,
			.extent = extent
		};

		return swap_chain_bundle;

	}

	vk::UniqueRenderPass CreateRenderPass(
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

}