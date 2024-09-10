
#include "pch.h"
#include "VulkanCore.h"

static VkInstance s_instance;

static bool IsDeviceSuitable(VkPhysicalDevice device) {

	std::uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	for (auto const& queue_family : queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

		}
	}

	return false;
}


VkInstance const& Fyuu::graphics::vulkan::VulKanInstance() noexcept {
    return s_instance;
}

void Fyuu::graphics::vulkan::InitializeVulkanInstance() {

	std::uint32_t property_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr);
	if (!property_count) {
		throw std::runtime_error("No Vulkan instance extensions available");
	}

	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Fyuu App",
		.applicationVersion = VK_MAKE_VERSION(1,0,0),
		.pEngineName = "Fyuu Engine",
		.engineVersion = VK_MAKE_VERSION(1,0,0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
	};

	auto result = vkCreateInstance(&create_info, nullptr, &s_instance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create vulkan instance");
	}

	std::uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(s_instance, &device_count, nullptr);
	if (!device_count) {
		throw std::runtime_error("No Vulkan devices available");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(s_instance, &device_count, devices.data());

	for (auto& device : devices) {

		VkPhysicalDeviceProperties2 device_property;
		vkGetPhysicalDeviceProperties2(device, &device_property);

		VkPhysicalDeviceFeatures2 device_feature;
		vkGetPhysicalDeviceFeatures2(device, &device_feature);

		if (IsDeviceSuitable(device)) {

		}
	}

}
