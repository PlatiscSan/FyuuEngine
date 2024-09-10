#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include <vulkan/vulkan.h>

#ifdef _WIN32
#pragma comment(lib, "vulkan-1.lib")
#endif // _WIN32

namespace Fyuu::graphics::vulkan {

	VkInstance const& VulKanInstance() noexcept;

	void InitializeVulkanInstance();

}


#endif // !VULKAN_CORE_H
