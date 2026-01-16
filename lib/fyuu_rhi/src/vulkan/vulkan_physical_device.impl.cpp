module;
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#endif // defined(_WIN32)

module fyuu_rhi:vulkan_physical_device;
import plastic.other;
import :vulkan_surface;

/// @brief this must be declared if we are going to use dynamic dispatch
vk::detail::DispatchLoaderDynamic vk::detail::defaultDispatchLoaderDynamic;

namespace fyuu_rhi::vulkan {

	static vk::Bool32 InternalLogCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT		message_severity,
		vk::DebugUtilsMessageTypeFlagsEXT				message_types,
		vk::DebugUtilsMessengerCallbackDataEXT const* callback_data,
		void* user_data
	) {

		auto user_callback = static_cast<LogCallback>(user_data);
		if (!user_callback) {
			return vk::False;
		}

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

		/*
		*	get severity
		*/ 

		auto severity = ToDebugSeverity(message_severity);

		 /*
		 *	construct message string
		 */

		std::ostringstream message;
		message << "Vulkan ";

		/*
		*	get message type
		*/

		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
			message << "[General] ";
		}
		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
			message << "[Validation] ";
		}
		if (message_types & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
			message << "[Performance] ";
		}

		/*
		*	get id if exists
		*/
		if (callback_data->pMessageIdName) {
			message << std::string(callback_data->pMessageIdName) + ": ";
		}

		/*
		*	main message
		*/
		if (callback_data->pMessage) {
			message << callback_data->pMessage;
		}

		/*
		*	object info
		*/
		if (callback_data->objectCount > 0) {
			message << "\nRelated Objects:";
			for (uint32_t i = 0; i < callback_data->objectCount; ++i) {
				const auto& object = callback_data->pObjects[i];
				message << "\n  - Object " << i +
					": Type=" << static_cast<std::uint32_t>(object.objectType) <<
					", Handle=" << object.objectHandle;
				if (object.pObjectName) {
					message << ", Name=\"" << object.pObjectName << "\"";
				}
			}
		}

		/*
		*	queue label info
		*/
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

		/*
		*	command buffer tag info
		*/
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

		user_callback(severity, message.str());

		return vk::False;

	}

	static VulkanDeviceFeatures QueryVulkanDeviceFeatures() {

		VulkanDeviceFeatures features;

		plastic::utility::InitializeGlobalInstance(
			[]() {
				vk::detail::defaultDispatchLoaderDynamic.init(vkGetInstanceProcAddr);
			}
		);

#ifndef _NDEBUG

		/*
		*	this feature is used in logical device.
		*/

		std::vector<vk::LayerProperties> layer_properties
			= vk::enumerateInstanceLayerProperties(vk::detail::defaultDispatchLoaderDynamic);

		if (std::find_if(
			layer_properties.begin(),
			layer_properties.end(),
			[](vk::LayerProperties& layer_properties) {
				std::string layer_name = layer_properties.layerName;
				return layer_name == "VK_LAYER_KHRONOS_validation";
			}
		) != layer_properties.end()) {
			features.set(static_cast<std::size_t>(VulkanDeviceFeature::VulkanFeatureValidation));
		}
#endif // !_NDEBUG

		/*
		*	look for debug utils
		*/

		std::vector<vk::ExtensionProperties> extension_properties = vk::enumerateInstanceExtensionProperties(
			nullptr,
			vk::detail::defaultDispatchLoaderDynamic
		);

		// sort the extensions alphabetically

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
				features.set(static_cast<std::size_t>(VulkanDeviceFeature::VulkanFeatureDebugUtils));
			}

		}

		return features;

	}

	static vk::UniqueInstance CreateInstance(RHIInitOptions const& init_options, VulkanDeviceFeatures features) {

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
			vk::ApiVersion14
		);

		vk::InstanceCreateInfo instance_create_info({}, &application_info);
		std::vector<char const*> extensions;

		if (features.test(VulkanDeviceFeature::VulkanFeatureDebugUtils)) {
			extensions.emplace_back(vk::EXTDebugUtilsExtensionName);
		}

		/*
		*	swap chain supports
		*/

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

		instance_create_info.ppEnabledExtensionNames = extensions.data();
		instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());

		vk::UniqueInstance instance = vk::createInstanceUnique(instance_create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);

		plastic::utility::InitializeGlobalInstance(
			[&instance]() {
				vk::detail::defaultDispatchLoaderDynamic.init(*instance);
			}
		);

		return instance;

	}

	static vk::UniqueDebugUtilsMessengerEXT SetLogCallback(
		vk::Instance const& instance,
		LogCallback const& callback
	) {

		vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		);

		vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
		);

		return instance.createDebugUtilsMessengerEXTUnique(
			vk::DebugUtilsMessengerCreateInfoEXT{
				{},
				severity_flags,
				message_type_flags,
				&InternalLogCallback,
				callback
			},
			nullptr,
			vk::detail::defaultDispatchLoaderDynamic
		);

	}

	static vk::PhysicalDevice ChoosePhysicalDevice(vk::Instance const& instance) {

		std::vector<vk::PhysicalDevice> physical_devices 
			= instance.enumeratePhysicalDevices(vk::detail::defaultDispatchLoaderDynamic);

		// sort devices
		std::sort(
			physical_devices.begin(), physical_devices.end(),
			[](vk::PhysicalDevice const& d1, vk::PhysicalDevice const& d2) -> bool {
				// return true if d1 is WORSE than d2

				// first check: discrete vs integrated -- pick discrete

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

				vk::PhysicalDeviceProperties features1 = d1.getProperties(vk::detail::defaultDispatchLoaderDynamic);
				vk::PhysicalDeviceProperties features2 = d2.getProperties(vk::detail::defaultDispatchLoaderDynamic);

				if (TypeRank(features1.deviceType) < TypeRank(features2.deviceType)) {
					return true;
				}
				else if (TypeRank(features1.deviceType) > TypeRank(features2.deviceType)) {
					return false;
				}

				constexpr static auto GetTotalVRAM = [](vk::PhysicalDevice dev) {

					vk::PhysicalDeviceMemoryProperties mem = dev.getMemoryProperties(vk::detail::defaultDispatchLoaderDynamic);

					std::size_t total_mem = 0u;
					for (std::uint32_t i = 0; i < mem.memoryHeapCount; ++i) {
						total_mem += mem.memoryHeaps[i].size;
					}
					return total_mem;
					};

				// tiebreak: we have two discrete GPUs
				// return the one with less VRAM
				if (GetTotalVRAM(d1) < GetTotalVRAM(d2)) {
					return true;
				}

				// give up and say they're equivalent
				return false;
			}
		);

		return physical_devices.back();

	}

	VulkanPhysicalDevice::VulkanPhysicalDevice(RHIInitOptions const& init_options)
		: m_features(QueryVulkanDeviceFeatures()),
		m_instance(CreateInstance(init_options, m_features)),
		m_debug_utils_messenger(SetLogCallback(*m_instance, init_options.log_callback)),
		m_impl(ChoosePhysicalDevice(*m_instance)) {
	}

	VulkanDeviceFeatures VulkanPhysicalDevice::GetEnabledFeatures() const noexcept {
		return m_features;
	}

	VulkanSwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(vk::SurfaceKHR const& surface) const {

		vk::SurfaceCapabilitiesKHR capabilities = m_impl.getSurfaceCapabilitiesKHR(
			surface,
			vk::detail::defaultDispatchLoaderDynamic
		);

		std::vector<vk::SurfaceFormatKHR> formats = m_impl.getSurfaceFormatsKHR(
			surface,
			vk::detail::defaultDispatchLoaderDynamic
		);

		std::vector<vk::PresentModeKHR> present_modes = m_impl.getSurfacePresentModesKHR(
			surface,
			vk::detail::defaultDispatchLoaderDynamic
		);

		return VulkanSwapChainSupportDetails{ std::move(capabilities), std::move(formats), std::move(present_modes) };

	}

	VulkanSwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(vk::UniqueSurfaceKHR const& surface) const {
		return QuerySwapChainSupport(*surface);
	}

	VulkanSwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(VulkanSurface const& surface) const {
		return QuerySwapChainSupport(surface.GetNative());
	}

	VulkanSwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(
		plastic::utility::AnyPointer<VulkanSurface> const& surface
	)  const {
		return QuerySwapChainSupport(*surface);
	}

	std::vector<VulkanCommandQueueInfo> VulkanPhysicalDevice::QueryAllCommandQueueInfo() const {

		std::vector<vk::QueueFamilyProperties> queue_families = m_impl.getQueueFamilyProperties(
			vk::detail::defaultDispatchLoaderDynamic
		);

		std::vector<VulkanCommandQueueInfo> queue_infos;

		std::uint32_t length = static_cast<std::uint32_t>(queue_families.size());
		for (std::uint32_t i = 0; i < length; ++i) {

			vk::QueueFamilyProperties const& current_family = queue_families[i];

			bool is_graphics = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eGraphics);
			bool is_compute = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eCompute);
			bool is_copy = static_cast<bool>(current_family.queueFlags & vk::QueueFlagBits::eTransfer);

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

		///*
		//*	test output
		//*/

		//for (auto& info : queue_infos) {
		//	std::cout << "Found queue - Type: "
		//		<< static_cast<std::uint32_t>(info.type)
		//		<< ", Presentable: " << info.presentable
		//		<< ", Family: " << (info.family.has_value() ? std::to_string(info.family.value()) : "N/A")
		//		<< std::endl;
		//}

		return queue_infos;

	}

	vk::Instance VulkanPhysicalDevice::GetInstance() const noexcept {
		return *m_instance;
	}

	vk::PhysicalDevice VulkanPhysicalDevice::GetNative() const noexcept {
		return m_impl;
	}

}