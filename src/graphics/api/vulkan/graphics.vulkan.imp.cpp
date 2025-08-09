module;
#include <vulkan/vulkan_core.h>

module graphics:vulkan;
import std;

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> present_modes;
};

static char const* VkDebugUtilsMessageTypeToString(vk::DebugUtilsMessageTypeFlagBitsEXT type) {

	struct DebugUtilsMessageTypeMapElement {
		vk::DebugUtilsMessageTypeFlagBitsEXT type;
		char const* type_str;
	};

	static DebugUtilsMessageTypeMapElement map[] = {
		{vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, "General"},
		{vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation, "Validation"},
		{vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, "Performance"},
		{vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding, "Device address binding"}
	};

	for (auto const& element : map) {
		if (type == element.type) {
			return element.type_str;
		}
	}

	return "Unknown";

}

static vk::DebugUtilsMessengerEXT CreateDebugCallback(
	vk::UniqueInstance const& instance,
	void* user_data,
	PFN_vkDebugUtilsMessengerCallbackEXT callback
) {

	auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance.get(), "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessengerEXT) {
		throw std::runtime_error("bad address of vkCreateDebugUtilsMessengerEXT");
	}

	VkDebugUtilsMessengerEXT debug_callback;

	vk::DebugUtilsMessengerCreateInfoEXT create_info(
		vk::DebugUtilsMessengerCreateFlagsEXT(),
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		callback,
		user_data
	);

	if (vkCreateDebugUtilsMessengerEXT(instance.get(), reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT const*>(&create_info), nullptr, &debug_callback) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug callback!");
	}

	return vk::DebugUtilsMessengerEXT(debug_callback);

}

static graphics::api::vulkan::QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice const& device, vk::SurfaceKHR const& surface) {

	graphics::api::vulkan::QueueFamilyIndices indices;

	auto queue_families = device.getQueueFamilyProperties();

	int i = 0;
	for (auto const& queue_family : queue_families) {
		if (queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphics_family.emplace(i);
		}

		if (queue_family.queueCount > 0 && device.getSurfaceSupportKHR(i, surface)) {
			indices.present_family.emplace(i);
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

static SwapChainSupportDetails QuerySwapChainSupport(
	vk::PhysicalDevice const& device, 
	vk::SurfaceKHR const& surface
) {

	SwapChainSupportDetails details;
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.present_modes = device.getSurfacePresentModesKHR(surface);

	return details;
}

static bool IsDeviceSuitable(
	vk::PhysicalDevice const& device, 
	vk::SurfaceKHR const& surface, 
	std::vector<char const*> const& extensions
) {

	graphics::api::vulkan::QueueFamilyIndices indices = FindQueueFamilies(device, surface);

	bool extensions_supported = graphics::api::vulkan::CheckDeviceExtensionSupport(device, extensions);

	bool swapChainAdequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(device, surface);
		swapChainAdequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	return indices.IsComplete() && extensions_supported && swapChainAdequate;

}

static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& available_formats) {
	if (available_formats.size() == 1 && available_formats[0].format == vk::Format::eUndefined) {
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (auto const& available_format : available_formats) {
		if (available_format.format == vk::Format::eB8G8R8A8Unorm && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return available_format;
		}
	}

	return available_formats[0];
}

static vk::PresentModeKHR ChooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& available_present_modes) {
	vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

	for (auto const& available_present_mode : available_present_modes) {
		if (available_present_mode == vk::PresentModeKHR::eMailbox) {
			return available_present_mode;
		}
		else if (available_present_mode == vk::PresentModeKHR::eImmediate) {
			bestMode = available_present_mode;
		}
	}

	return bestMode;
}

static vk::Extent2D ChooseSwapExtent(platform::IWindow* window, vk::SurfaceCapabilitiesKHR const& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		auto [width, height] = window->GetWidthAndHeight();

		vk::Extent2D actual_extent = { width, height };

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}

namespace graphics::api::vulkan {

	bool CheckLayerSupport(std::string_view layer_name) {

		static auto available_layers = vk::enumerateInstanceLayerProperties();

		for (auto const& layer_properties : available_layers) {
			if (layer_name == layer_properties.layerName) {
				return true;
			}
		}

		return false;

	}

	bool CheckExtensionSupport(std::string_view extension_name) {

		static auto available_extensions = vk::enumerateInstanceExtensionProperties();

		for (auto const& layer_properties : available_extensions) {
			if (extension_name == layer_properties.extensionName) {
				return true;
			}
		}

		return false;

	}

	bool CheckDeviceExtensionSupport(vk::PhysicalDevice const& device, std::vector<char const*> const& extensions) {

		std::set<std::string> required_extensions(extensions.begin(), extensions.end());

		auto extension_properties = device.enumerateDeviceExtensionProperties();

		for (auto const& extension : extension_properties) {
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();

	}

	vk::PhysicalDevice PickPhysicalDevice(
		vk::UniqueInstance const& instance,
		vk::SurfaceKHR const& surface,
		std::vector<char const*> const& extensions
	) {

		auto devices = instance->enumeratePhysicalDevices();
		if (devices.size() == 0) {
			throw std::runtime_error("No GPUs with Vulkan support");
		}

		for (auto const& device : devices) {
			if (IsDeviceSuitable(device, surface, extensions)) {
				return device;
			}
		}

		throw std::runtime_error("No suitable GPU");

	}

	vk::UniqueDevice CreateLogicDevice(
		vk::PhysicalDevice const& physical_device, 
		vk::SurfaceKHR const& surface, 
		std::vector<char const*> const& extensions
	) {

		std::vector<char const*> layers;
		QueueFamilyIndices indices = FindQueueFamilies(physical_device, surface);

		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

		float queue_priority = 1.0f;

		for (std::uint32_t queue_family : unique_queue_families) {
			queue_create_infos.push_back({
				vk::DeviceQueueCreateFlags(),
				queue_family,
				1, // queueCount
				&queue_priority
				});
		}

		vk::PhysicalDeviceFeatures device_features{};
		vk::DeviceCreateInfo create_info(
			vk::DeviceCreateFlags(),
			static_cast<std::uint32_t>(queue_create_infos.size()),
			queue_create_infos.data()
		);

		create_info.pEnabledFeatures = &device_features;
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
		layers.emplace_back("VK_LAYER_LUNARG_standard_validation");
#endif // !NDEBUG

		create_info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
		create_info.ppEnabledLayerNames = layers.data();

		return physical_device.createDeviceUnique(create_info);

	}

	vk::Queue GetGraphicsQueue(
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface,
		vk::UniqueDevice const& device
	) {
		QueueFamilyIndices indices = FindQueueFamilies(physical_device, surface);
		return device->getQueue(indices.graphics_family.value(), 0);
	}

	vk::Queue GetPresentQueue(
		vk::PhysicalDevice const& physical_device, 
		vk::SurfaceKHR const& surface, 
		vk::UniqueDevice const& device
	) {
		QueueFamilyIndices indices = FindQueueFamilies(physical_device, surface);
		return device->getQueue(indices.present_family.value(), 0);
	}

	vk::SwapchainKHR CreateSwapChain(
		platform::IWindow* window,
		vk::PhysicalDevice const& physical_device, 
		vk::SurfaceKHR const& surface,
		vk::UniqueDevice const& device
	) {

		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device, surface);
		
		vk::SurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
		vk::PresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
		vk::Extent2D extent = ChooseSwapExtent(window, swap_chain_support.capabilities);

		std::uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
		if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
			image_count = swap_chain_support.capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR create_info(
			vk::SwapchainCreateFlagsKHR(),
			surface,
			image_count,
			surface_format.format,
			surface_format.colorSpace,
			extent,
			1, // imageArrayLayers
			vk::ImageUsageFlagBits::eColorAttachment
		);

		QueueFamilyIndices indices = FindQueueFamilies(physical_device, surface);
		std::uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

		if (indices.graphics_family != indices.present_family) {
			create_info.imageSharingMode = vk::SharingMode::eConcurrent;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			create_info.imageSharingMode = vk::SharingMode::eExclusive;
		}

		create_info.preTransform = swap_chain_support.capabilities.currentTransform;
		create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;

		create_info.oldSwapchain = vk::SwapchainKHR(nullptr);

		return device->createSwapchainKHR(create_info);

	}

	vk::Format GetSwapChainImageFormat(
		vk::PhysicalDevice const& physical_device, 
		vk::SurfaceKHR const& surface
	) {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device, surface);
		return ChooseSwapSurfaceFormat(swap_chain_support.formats).format;
	}

	vk::Extent2D GetSwapChainExtent(
		platform::IWindow* window,
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface
	) {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device, surface);
		return ChooseSwapExtent(window, swap_chain_support.capabilities);
	}

	std::vector<vk::ImageView> CreateSwapChainImageViews(
		vk::UniqueDevice const& device,
		std::vector<vk::Image> const& swap_chain_image, 
		vk::Format swap_chain_image_format
	) {
		
		std::vector<vk::ImageView> swap_chain_image_views(swap_chain_image.size());
		
		for (std::size_t i = 0; i < swap_chain_image.size(); ++i) {
			vk::ImageViewCreateInfo create_info = {};
			create_info.image = swap_chain_image[i];
			create_info.viewType = vk::ImageViewType::e2D;
			create_info.format = swap_chain_image_format;
			create_info.components.r = vk::ComponentSwizzle::eIdentity;
			create_info.components.g = vk::ComponentSwizzle::eIdentity;
			create_info.components.b = vk::ComponentSwizzle::eIdentity;
			create_info.components.a = vk::ComponentSwizzle::eIdentity;
			create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			try {
				swap_chain_image_views[i] = device->createImageView(create_info);
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("Failed to create image views");
			}
		}

		return swap_chain_image_views;

	}

	vk::RenderPass CreateRenderPass(vk::UniqueDevice const& device, vk::Format const& swap_chain_image_format) {

		vk::AttachmentDescription color_attachment = {};
		color_attachment.format = swap_chain_image_format;
		color_attachment.samples = vk::SampleCountFlagBits::e1;
		color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
		color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
		color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		color_attachment.initialLayout = vk::ImageLayout::eUndefined;
		color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		vk::SubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		//dependency.srcAccessMask = 0;
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		vk::RenderPassCreateInfo render_pass_info = {};
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		return device->createRenderPass(render_pass_info);

	}

	vk::DebugUtilsMessengerEXT BaseVulkanRenderDevice::SetupLogger(BaseVulkanRenderDevice* device) {
		return CreateDebugCallback(
			device->m_instance,
			device,
			[](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT const* callback_data, void* user_data) -> VkBool32 {
				auto device = static_cast<BaseVulkanRenderDevice*>(user_data);
				if (!device) {
					return vk::False;
				}
				if (auto& logger = device->m_logger) {

					switch (message_severity) {
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
						logger->Debug(
							std::source_location::current(), "[{}] - {}", 
							VkDebugUtilsMessageTypeToString(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(message_type)), 
							callback_data->pMessage
						);
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
						logger->Info(
							std::source_location::current(), "[{}] - {}",
							VkDebugUtilsMessageTypeToString(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(message_type)),
							callback_data->pMessage
						);
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
						logger->Warning(
							std::source_location::current(), "[{}] - {}",
							VkDebugUtilsMessageTypeToString(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(message_type)),
							callback_data->pMessage
						);
						break;
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
						logger->Error(
							std::source_location::current(), "[{}] - {}",
							VkDebugUtilsMessageTypeToString(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(message_type)),
							callback_data->pMessage
						);
						break;
					default:
						logger->Trace(
							std::source_location::current(), "[{}] - {}",
							VkDebugUtilsMessageTypeToString(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(message_type)),
							callback_data->pMessage
						);
						break;
					}
				}
				return vk::False;
			}
		);
	}

	void BaseVulkanRenderDevice::Clear(float r, float g, float b, float a)
	{
	}

	void BaseVulkanRenderDevice::SetViewport(int x, int y, uint32_t width, uint32_t height)
	{
	}

	bool BaseVulkanRenderDevice::BeginFrame()
	{
		return false;
	}

	void BaseVulkanRenderDevice::EndFrame()
	{
	}

}