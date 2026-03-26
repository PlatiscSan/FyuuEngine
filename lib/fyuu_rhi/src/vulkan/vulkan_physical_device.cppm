/* vulkan_physical_device.cppm */
module;

#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#include <string>
#include <sstream>
#include <bitset>
#endif
#if !defined(__APPLE__)
#include <vulkan/vulkan.h>
#endif // !defined(__APPLE__)

export module fyuu_rhi:vulkan_physical_device;

#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif

import vulkan;
import plastic.other;
import :types;
import :vulkan_common;
import :vulkan_types;

// Export the contents of the namespace
export namespace fyuu_rhi::vulkan {

	/**
	 * @brief Enumeration of configurable Vulkan device features.
	 */
	enum class VulkanDeviceFeature : std::uint8_t {
		Validation,   ///< Validation layers (debug builds only)
		DebugUtils,   ///< VK_EXT_debug_utils extension
		Counts		///< Number of features (used for bitset size)
	};

	/**
	 * @brief Bitset representing which features are enabled.
	 */
	using VulkanDeviceFeatures = std::bitset<static_cast<std::size_t>(VulkanDeviceFeature::Counts)>;

	/**
	 * @brief Represents a Vulkan physical device and manages instance creation.
	 * 
	 * This class is responsible for:
	 * - Creating a Vulkan instance with appropriate layers and extensions.
	 * - Setting up a debug messenger (if enabled).
	 * - Enumerating and selecting the most suitable physical device.
	 * - Providing methods to query device properties and create a logical device.
	 */
	class VulkanPhysicalDevice
		: public PolymorphicPhysicalDeviceBase,
		  public VulkanCommon<VulkanPhysicalDevice> {

	private:
		VulkanDeviceFeatures m_features;			   ///< Enabled feature flags.
		vk::UniqueInstance m_instance;				  ///< Vulkan instance handle.
		LogCallback m_log_callback;					  ///< User-provided logging callback.
		vk::UniqueDebugUtilsMessengerEXT m_debug_utils_messenger; ///< Debug messenger (if enabled).
		vk::PhysicalDevice m_impl;					   ///< Selected Vulkan physical device.

		/**
		 * @brief Internal static callback for Vulkan debug messages.
		 * 
		 * Converts Vulkan debug severity/types into the engine's log format
		 * and forwards to the user-provided callback.
		 * 
		 * @param message_severity Severity of the message.
		 * @param message_types Type flags of the message.
		 * @param callback_data Detailed message data.
		 * @param user_data Pointer to the user's LogCallback pair.
		 * @return VK_FALSE (always, to indicate the message was handled).
		 */
		static vk::Bool32 InternalLogCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT		message_severity,
			vk::DebugUtilsMessageTypeFlagsEXT				message_types,
			vk::DebugUtilsMessengerCallbackDataEXT const* callback_data,
			void* user_data
		);

	public:
		/**
		 * @brief Returns a reference to the dynamic dispatch loader for instance-level functions.
		 * 
		 * The loader is initialized during instance creation and is used for all
		 * Vulkan calls that require instance-level dispatch.
		 */
		static vk::detail::DispatchLoaderDynamic& InstanceLevelDispatcher() noexcept;

		/**
		 * @brief Constructs the VulkanPhysicalDevice, creating the instance and selecting a physical device.
		 * 
		 * @param init_options Initialization options (application name, version, logging callback, etc.).
		 */
		VulkanPhysicalDevice(InitOptions const& init_options);

		/**
		 * @brief Returns the bitset of enabled Vulkan device features.
		 */
		VulkanDeviceFeatures GetEnabledFeatures() const noexcept;

		/**
		 * @brief Queries swap chain support details for a given surface.
		 * 
		 * @param surface The surface to query against.
		 * @return VulkanSwapChainSupportDetails containing capabilities, formats, and present modes.
		 */
		VulkanSwapChainSupportDetails QuerySwapChainSupport(vk::SurfaceKHR const& surface) const;

		/**
		 * @brief Retrieves information about all available command queues on this physical device.
		 * 
		 * @return A vector of VulkanCommandQueueInfo describing each queue family and its capabilities.
		 */
		std::vector<VulkanCommandQueueInfo> QueryAllCommandQueueInfo() const;

		/**
		 * @brief Returns the underlying Vulkan instance handle.
		 */
		vk::Instance GetInstance() const noexcept;

		/**
		 * @brief Returns the underlying Vulkan physical device handle.
		 */
		vk::PhysicalDevice GetNative() const noexcept;

		/**
		 * @brief Returns the vendor ID of the physical device.
		 */
		std::uint32_t GetVendorID() const noexcept;

		/**
		 * @brief Returns the device ID of the physical device.
		 */
		std::uint32_t GetID() const noexcept;

		/**
		 * @brief Returns a human-readable description (device name) of the physical device.
		 */
		std::string GetDescription() const;

		/**
		 * @brief Fills a vk::PhysicalDeviceFeatures2 structure with the device's supported features.
		 * 
		 * @param feature_list The structure to be filled (chainable for extended features).
		 */
		void GetFeatures(vk::PhysicalDeviceFeatures2& feature_list) const;

		/**
		 * @brief Creates a logical device from this physical device using the provided creation info.
		 * 
		 * @param info Device creation parameters (queues, features, extensions, etc.).
		 * @return A unique handle to the created logical device.
		 */
		vk::UniqueDevice CreateLogicalDevice(vk::DeviceCreateInfo const& info) const;

#if defined(_WIN32)
		vk::UniqueSurfaceKHR CreateSurface(vk::Win32SurfaceCreateInfoKHR const& info) const;
#elif defined(__linux__)
		vk::UniqueSurfaceKHR CreateSurface(vk::WaylandSurfaceCreateInfoKHR const& info) const;
		vk::UniqueSurfaceKHR CreateSurface(vk::XlibSurfaceCreateInfoKHR const& info) const;
#elif defined(__ANDROID__)
		vk::UniqueSurfaceKHR CreateSurface(vk::AndroidSurfaceCreateInfoKHR const& info) const;
#endif // defined(_WIN32)

		vk::Bool32 IsQueueSupportsPresenting(std::uint32_t queue_family, vk::SurfaceKHR const& valid_surface) const noexcept;

		std::vector<vk::PresentModeKHR> GetCompatiblePresentModes(vk::SurfaceKHR const& surface, vk::PresentModeKHR main_present_mode) const;

	};

}
#endif // !defined(__APPLE__)