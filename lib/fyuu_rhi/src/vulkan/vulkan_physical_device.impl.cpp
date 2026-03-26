/* vulkan_physical_device.impl.cpp */
module;

#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#include <string>
#include <bitset>
#include <sstream>
#include <algorithm>
#endif // !defined(__cpp_lib_modules)

#if !defined(__APPLE__)
#include <vulkan/vulkan.h>
#endif // !defined(__APPLE__)

module fyuu_rhi:vulkan_physical_device_impl;

#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :vulkan_physical_device;
import vulkan;
import plastic.other;
import :types;
import :vulkan_common;
import :vulkan_types;

namespace {
	// Global dynamic dispatcher for Vulkan instance-level functions.
	// It is initialized first with vkGetInstanceProcAddr, then updated after instance creation.
	vk::detail::DispatchLoaderDynamic s_dispatcher;
}

namespace fyuu_rhi::vulkan {

	/**
	 * @brief Implementation of the internal debug callback.
	 * 
	 * This function is called by the Vulkan validation layers whenever a debug message is produced.
	 * It translates the Vulkan severity and type into the engine's LogSeverity, builds a descriptive
	 * message string (including object and label information), and forwards it to the user callback.
	 * 
	 * @param message_severity Vulkan message severity (e.g., eError, eWarning).
	 * @param message_types Vulkan message type flags (e.g., eValidation, ePerformance).
	 * @param callback_data Pointer to a structure containing the message details.
	 * @param user_data Pointer to the user's LogCallback pair (function pointer + user data).
	 * @return vk::False to indicate the message has been handled.
	 */
	vk::Bool32 VulkanPhysicalDevice::InternalLogCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT		message_severity,
		vk::DebugUtilsMessageTypeFlagsEXT				message_types,
		vk::DebugUtilsMessengerCallbackDataEXT const* callback_data,
		void* user_data
	) {
		// Retrieve the user callback function and its user data.
		auto [Func, Func_user_data] = *static_cast<LogCallback*>(user_data);
		if (!Func) {
			return vk::False;
		}

		// Convert Vulkan severity to engine's log severity.
		constexpr auto ToDebugSeverity = [](vk::DebugUtilsMessageSeverityFlagBitsEXT severity) {
			if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
				return LogSeverity::Fatal;
			}
			else if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
				return LogSeverity::Warning;
			}
			else {
				return LogSeverity::Info;
			}
		};

		auto severity = ToDebugSeverity(message_severity);

		// Build a comprehensive message string.
		std::ostringstream message;
		message << "Vulkan ";

		// Append message type(s) as tags.
		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
			message << "[General] ";
		}
		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
			message << "[Validation] ";
		}
		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
			message << "[Performance] ";
		}

		// Append message ID name if available.
		if (callback_data->pMessageIdName) {
			message << std::string(callback_data->pMessageIdName) + ": ";
		}

		// Append the main message text.
		if (callback_data->pMessage) {
			message << callback_data->pMessage;
		}

		// Append information about related Vulkan objects.
		if (callback_data->objectCount > 0) {
			message << "\nRelated Objects:";
			for (uint32_t i = 0; i < callback_data->objectCount; ++i) {
				const auto& object = callback_data->pObjects[i];
				message << "\n  - Object " << i <<
					": Type=" << static_cast<std::uint32_t>(object.objectType) <<
					", Handle=" << object.objectHandle;
				if (object.pObjectName) {
					message << ", Name=\"" << object.pObjectName << "\"";
				}
			}
		}

		// Append queue label information.
		if (callback_data->queueLabelCount > 0) {
			message << "\nQueue Labels:";
			for (std::uint32_t i = 0; i < callback_data->queueLabelCount; ++i) {
				auto const& label = callback_data->pQueueLabels[i];
				message << "\n  - Label " << i << ": ";
				if (label.pLabelName) {
					message << label.pLabelName;
				}
			}
		}

		// Append command buffer label information.
		if (callback_data->cmdBufLabelCount > 0) {
			message << "\nCommand Buffer Labels:";
			for (std::uint32_t i = 0; i < callback_data->cmdBufLabelCount; ++i) {
				auto const& label = callback_data->pCmdBufLabels[i];
				message << "\n  - Label " << std::to_string(i) << ": ";
				if (label.pLabelName) {
					message << label.pLabelName;
				}
			}
		}

		// Forward the constructed message to the user callback.
		Func(severity, message.str(), Func_user_data);

		return vk::False;
	}

	/**
	 * @brief Returns the global dynamic dispatcher for instance-level Vulkan commands.
	 */
	vk::detail::DispatchLoaderDynamic& VulkanPhysicalDevice::InstanceLevelDispatcher() noexcept {
		return s_dispatcher;
	}

	/**
	 * @brief Constructor: creates the Vulkan instance, enables debugging, and selects a physical device.
	 * 
	 * The construction proceeds in several steps, each implemented as a lambda that initializes
	 * a member variable:
	 * 1. Determine which features (validation layers, debug utils) are available.
	 * 2. Create the Vulkan instance with appropriate layers and extensions.
	 * 3. Set up the debug messenger if DebugUtils is enabled.
	 * 4. Enumerate physical devices and select the best one based on type and VRAM.
	 * 
	 * @param init_options Contains application name/version, engine name/version, and logging callback.
	 */
	VulkanPhysicalDevice::VulkanPhysicalDevice(InitOptions const& init_options)
		: PolymorphicPhysicalDeviceBase(this),
		  VulkanCommon(this),
		  m_features(
			// Lambda to detect available features.
			[]() {
				VulkanDeviceFeatures features;

				// Initialize the global dispatcher with vkGetInstanceProcAddr (required before any Vulkan calls).
				plastic::utility::InitializeGlobalInstance(
					[]() {
						s_dispatcher.init(vkGetInstanceProcAddr);
					}
				);

#ifndef _NDEBUG
				// Check for presence of the Khronos validation layer (debug builds only).
				std::vector<vk::LayerProperties> layer_properties
					= vk::enumerateInstanceLayerProperties(s_dispatcher);

				if (std::find_if(
					layer_properties.begin(),
					layer_properties.end(),
					[](vk::LayerProperties& layer_properties) {
						std::string layer_name = layer_properties.layerName;
						return layer_name == "VK_LAYER_KHRONOS_validation";
					}
				) != layer_properties.end()) {
					features.set(static_cast<std::size_t>(VulkanDeviceFeature::Validation));
				}
#endif // !_NDEBUG

				// Check for presence of the VK_EXT_debug_utils extension.
				std::vector<vk::ExtensionProperties> extension_properties = vk::enumerateInstanceExtensionProperties(
					nullptr,
					s_dispatcher
				);

				// Sort extensions alphabetically for easier searching.
				std::sort(
					extension_properties.begin(),
					extension_properties.end(),
					[](vk::ExtensionProperties const& a, vk::ExtensionProperties const& b) {
						return std::strcmp(a.extensionName, b.extensionName) < 0;
					}
				);

				for (vk::ExtensionProperties const& one : extension_properties) {
					std::string_view extension_name = one.extensionName;
					if (extension_name == vk::EXTDebugUtilsExtensionName) {
						features.set(static_cast<std::size_t>(VulkanDeviceFeature::DebugUtils));
					}
				}

				return features;
			}()),
		  m_instance(
			// Lambda to create the Vulkan instance.
			[this, &init_options]() {
				vk::ApplicationInfo application_info(
					init_options.app_name.data(),
					vk::makeApiVersion(
						init_options.app_version.variant,
						init_options.app_version.major,
						init_options.app_version.minor,
						init_options.app_version.patch
					),
					init_options.engine_name.data(),
					vk::makeApiVersion(
						init_options.engine_version.variant,
						init_options.engine_version.major,
						init_options.engine_version.minor,
						init_options.engine_version.patch
					),
					vk::ApiVersion14  // Target Vulkan 1.4
				);

				vk::InstanceCreateInfo instance_create_info({}, &application_info);
				std::vector<char const*> extensions;

				// Add debug utils extension if available.
				if (m_features.test(static_cast<std::size_t>(VulkanDeviceFeature::DebugUtils))) {
					extensions.emplace_back(vk::EXTDebugUtilsExtensionName);
				}

				// Add surface extensions required for swap chain support.
				extensions.emplace_back(vk::KHRSurfaceExtensionName);
#if defined(_WIN32)
				extensions.emplace_back(vk::KHRWin32SurfaceExtensionName);
#elif defined(__linux__)
				extensions.emplace_back(vk::KHRXlibSurfaceExtensionName);
				extensions.emplace_back(vk::KHRXcbSurfaceExtensionName);
				extensions.emplace_back(vk::KHRWaylandSurfaceExtensionName);
#elif defined(__ANDROID__)
				extensions.emplace_back(vk::KHRAndroidSurfaceExtensionName);
#endif // defined(_WIN32)
				// Modern surface
				extensions.emplace_back(vk::KHRGetSurfaceCapabilities2ExtensionName);
				extensions.emplace_back(vk::EXTSurfaceMaintenance1ExtensionName);

				instance_create_info.ppEnabledExtensionNames = extensions.data();
				instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());

#if !defined(_NDEBUG)
				// In debug builds, enable validation layers if they are available.
				constexpr static const char* const validation_layers[] = {
					"VK_LAYER_KHRONOS_validation",
		#ifdef RECONSTRUCT
					"VK_LAYER_LUNARG_gfxreconstruct"
		#endif
		#ifdef APIDUMP
					"VK_LAYER_LUNARG_api_dump",
		#endif
				};

				if (m_features.test(static_cast<std::size_t>(VulkanDeviceFeature::Validation))) {
					instance_create_info.enabledLayerCount = static_cast<std::uint32_t>(std::size(validation_layers));
					instance_create_info.ppEnabledLayerNames = validation_layers;
				}
#endif // !defined(_NDEBUG)

				// Create the instance.
				vk::UniqueInstance instance = vk::createInstanceUnique(instance_create_info, nullptr, s_dispatcher);

				// Re-initialize the global dispatcher with the new instance to load instance-specific commands.
				plastic::utility::InitializeGlobalInstance(
					[&instance]() {
						s_dispatcher.init(*instance);
					}
				);

				return instance;
			}()),
		  m_log_callback(init_options.log_callback),  // Store user callback for debug messenger.
		  m_debug_utils_messenger(
			// Lambda to create the debug utils messenger if DebugUtils is enabled.
			[this]() {
				// Capture all severity levels.
				vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
				);

				// Capture all message types.
				vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
					vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
					vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
					vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
					// eDeviceAddressBinding is omitted (Vulkan 1.2+)
				);

				return m_instance->createDebugUtilsMessengerEXTUnique(
					vk::DebugUtilsMessengerCreateInfoEXT{
						{},
						severity_flags,
						message_type_flags,
						&InternalLogCallback,   // Static callback function
						&m_log_callback		  // User data passed to callback
					},
					nullptr,
					s_dispatcher
				);
			}()),
		  m_impl(
			// Lambda to select the most suitable physical device.
			[this]() {
				std::vector<vk::PhysicalDevice> physical_devices
					= m_instance->enumeratePhysicalDevices(s_dispatcher);

				// Sort devices to pick the best one.
				std::sort(
					physical_devices.begin(), physical_devices.end(),
					[](vk::PhysicalDevice const& d1, vk::PhysicalDevice const& d2) -> bool {
						// Return true if d1 is WORSE than d2 (so best will be last after sorting).

						// Helper to rank device types: discrete GPU > integrated > virtual > CPU > other.
						constexpr static auto TypeRank = [](vk::PhysicalDeviceType type) {
							switch (type) {
							case vk::PhysicalDeviceType::eDiscreteGpu:  return 10;
							case vk::PhysicalDeviceType::eIntegratedGpu: return 5;
							case vk::PhysicalDeviceType::eVirtualGpu: return 3;
							case vk::PhysicalDeviceType::eCpu: return 2;
							case vk::PhysicalDeviceType::eOther: return 0;
							default: return 0;
							}
						};

						vk::PhysicalDeviceProperties features1 = d1.getProperties(s_dispatcher);
						vk::PhysicalDeviceProperties features2 = d2.getProperties(s_dispatcher);

						// Compare by type rank first.
						if (TypeRank(features1.deviceType) < TypeRank(features2.deviceType)) {
							return true; // d1 has lower rank -> worse
						}
						else if (TypeRank(features1.deviceType) > TypeRank(features2.deviceType)) {
							return false; // d1 has higher rank -> better
						}

						// If types are equal, compare total available VRAM (choose the one with more VRAM).
						constexpr static auto GetTotalVRAM = [](vk::PhysicalDevice dev) {
							vk::PhysicalDeviceMemoryProperties mem = dev.getMemoryProperties(s_dispatcher);
							std::size_t total_mem = 0u;
							for (std::uint32_t i = 0; i < mem.memoryHeapCount; ++i) {
								total_mem += mem.memoryHeaps[i].size;
							}
							return total_mem;
						};

						if (GetTotalVRAM(d1) < GetTotalVRAM(d2)) {
							return true; // d1 has less VRAM -> worse
						}

						// Otherwise consider them equivalent.
						return false;
					}
				);

				// The best device is the last element after sorting (since we sorted with "worse" first).
				return physical_devices.back();
			}()) {
		// Constructor body is empty; all initialization done via member initializers.
	}

	/**
	 * @brief Returns the bitset of enabled features.
	 */
	VulkanDeviceFeatures VulkanPhysicalDevice::GetEnabledFeatures() const noexcept {
		return m_features;
	}

	/**
	 * @brief Queries swap chain support for a given surface.
	 * 
	 * @param surface The surface to query.
	 * @return VulkanSwapChainSupportDetails containing capabilities, formats, and present modes.
	 */
	VulkanSwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(vk::SurfaceKHR const& surface) const {
		vk::SurfaceCapabilitiesKHR capabilities = m_impl.getSurfaceCapabilitiesKHR(surface, s_dispatcher);
		std::vector<vk::SurfaceFormatKHR> formats = m_impl.getSurfaceFormatsKHR(surface, s_dispatcher);
		std::vector<vk::PresentModeKHR> present_modes = m_impl.getSurfacePresentModesKHR(surface, s_dispatcher);
		return VulkanSwapChainSupportDetails{ std::move(capabilities), std::move(formats), std::move(present_modes) };
	}

	/**
	 * @brief Returns information about all command queue families available on this device.
	 * 
	 * The function iterates over each queue family and classifies it based on supported flags.
	 * Families that support all of graphics, compute, and transfer are marked as AllCommands.
	 * Families that support compute+transfer are marked as Compute, and families that support
	 * only transfer are marked as Copy.
	 * 
	 * @return A vector of VulkanCommandQueueInfo, each containing the type, family index, and queue count.
	 */
	std::vector<VulkanCommandQueueInfo> VulkanPhysicalDevice::QueryAllCommandQueueInfo() const {
		std::vector<vk::QueueFamilyProperties> queue_families = m_impl.getQueueFamilyProperties(s_dispatcher);
		std::vector<VulkanCommandQueueInfo> queue_infos;

		std::uint32_t length = static_cast<std::uint32_t>(queue_families.size());
		for (std::uint32_t i = 0; i < length; ++i) {
			vk::QueueFamilyProperties const& current_family = queue_families[i];

			bool is_graphics = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eGraphics);
			bool is_compute  = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eCompute);
			bool is_copy	 = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eTransfer);

			if (is_graphics && is_compute && is_copy) {
				queue_infos.push_back({ CommandObjectType::AllCommands, i, current_family.queueCount });
				continue;
			}

			if (is_compute && is_copy) {
				queue_infos.push_back({ CommandObjectType::Compute, i, current_family.queueCount });
				continue;
			}

			if (is_copy) {
				queue_infos.push_back({ CommandObjectType::Copy, i, current_family.queueCount });
				continue;
			}
		}

		return queue_infos;
	}

	/**
	 * @brief Returns the underlying Vulkan instance handle.
	 */
	vk::Instance VulkanPhysicalDevice::GetInstance() const noexcept {
		return *m_instance;
	}

	/**
	 * @brief Returns the underlying Vulkan physical device handle.
	 */
	vk::PhysicalDevice VulkanPhysicalDevice::GetNative() const noexcept {
		return m_impl;
	}

	/**
	 * @brief Returns the vendor ID of the physical device.
	 */
	std::uint32_t VulkanPhysicalDevice::GetVendorID() const noexcept {
		auto props = m_impl.getProperties(s_dispatcher);
		return props.vendorID;
	}

	/**
	 * @brief Returns the device ID of the physical device.
	 */
	std::uint32_t VulkanPhysicalDevice::GetID() const noexcept {
		auto props = m_impl.getProperties(s_dispatcher);
		return props.deviceID;
	}

	/**
	 * @brief Returns the device name as a string.
	 */
	std::string VulkanPhysicalDevice::GetDescription() const {
		auto props = m_impl.getProperties(s_dispatcher);
		return props.deviceName.data();
	}

	/**
	 * @brief Fills a vk::PhysicalDeviceFeatures2 structure with the device's supported features.
	 * 
	 * @param feature_list The structure to be filled (can be chained with extension-specific feature structs).
	 */
	void VulkanPhysicalDevice::GetFeatures(vk::PhysicalDeviceFeatures2& feature_list) const {
		m_impl.getFeatures2(&feature_list, s_dispatcher);
	}

	/**
	 * @brief Creates a logical device from this physical device.
	 * 
	 * @param info Device creation parameters (queues, enabled features, extensions, etc.).
	 * @return A unique handle to the created logical device.
	 */
	vk::UniqueDevice VulkanPhysicalDevice::CreateLogicalDevice(vk::DeviceCreateInfo const& info) const {
		return m_impl.createDeviceUnique(info, nullptr, s_dispatcher);
	}

#if defined(_WIN32)
	vk::UniqueSurfaceKHR VulkanPhysicalDevice::CreateSurface(vk::Win32SurfaceCreateInfoKHR const& info) const {
		return m_instance->createWin32SurfaceKHRUnique(info, nullptr, s_dispatcher);
	}
#elif defined(__linux__)
	vk::UniqueSurfaceKHR VulkanPhysicalDevice::CreateSurface(vk::WaylandSurfaceCreateInfoKHR const& info) const {
		return m_instance->createWaylandSurfaceKHRUnique(info, nullptr, s_dispatcher);
	}
	vk::UniqueSurfaceKHR VulkanPhysicalDevice::CreateSurface(vk::XlibSurfaceCreateInfoKHR const& info) const {
		return m_instance->createXlibSurfaceKHRUnique(info, nullptr, s_dispatcher);
	}
#elif defined(__ANDROID__)
	vk::UniqueSurfaceKHR VulkanPhysicalDevice::CreateSurface(vk::AndroidSurfaceCreateInfoKHR const& info) const {
		return m_instance->createAndroidSurfaceKHRUnique(info, nullptr, s_dispatcher);
	}
#endif // defined(_WIN32)
	vk::Bool32 VulkanPhysicalDevice::IsQueueSupportsPresenting(std::uint32_t queue_family, vk::SurfaceKHR const& valid_surface) const noexcept {
		return m_impl.getSurfaceSupportKHR(
			queue_family,
			valid_surface,
			s_dispatcher
		);
	}

	std::vector<vk::PresentModeKHR> VulkanPhysicalDevice::GetCompatiblePresentModes(vk::SurfaceKHR const& surface, vk::PresentModeKHR main_present_mode) const {
		
		vk::SurfacePresentModeEXT present_mode_info(main_present_mode);
		vk::PhysicalDeviceSurfaceInfo2KHR surface_info(surface, &present_mode_info);

		std::vector<vk::PresentModeKHR> compatible_modes;
		vk::SurfacePresentModeCompatibilityEXT compatibility_info;

		{
			vk::SurfaceCapabilities2KHR surface_caps({}, &compatibility_info);
			(void)m_impl.getSurfaceCapabilities2KHR(&surface_info, &surface_caps, s_dispatcher);
			compatible_modes.resize(compatibility_info.presentModeCount);
		}

		compatibility_info.pPresentModes = compatible_modes.data();
		vk::SurfaceCapabilities2KHR surface_caps({}, &compatibility_info);
		(void)m_impl.getSurfaceCapabilities2KHR(&surface_info, &surface_caps, s_dispatcher);
		
		return compatible_modes;

	}

} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)