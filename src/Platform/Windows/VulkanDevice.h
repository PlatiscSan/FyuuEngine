#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "Framework/Renderer/GraphicsDevice.h"
#include "WindowsUtility.h"
#include <vulkan/vulkan.h>

namespace Fyuu {

	class VulkanDevice final : public GraphicsDevice {

	public:

		VulkanDevice();

	private:

		VkInstance m_vk_instance;

	};

}

#endif // !VULKAN_DEVICE_H
