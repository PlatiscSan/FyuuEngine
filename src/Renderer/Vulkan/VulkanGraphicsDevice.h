#ifndef VULKAN_GRAPHICS_DEVICE_H
#define VULKAN_GRAPHICS_DEVICE_H

#include "Framework/Graphics/GraphicsDevice.h"
#include "../WindowsUtility.h"

namespace Fyuu::graphics {

	class VulkanGraphicsDevice final : public GraphicsDevice {

	public:

		VulkanGraphicsDevice();

		

	private:

		VkInstance m_instance;

		static bool IsDeviceSuitable(VkPhysicalDevice device);

	};

}

#endif // !VULKAN_GRAPHICS_DEVICE_H
