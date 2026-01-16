export module fyuu_rhi:vulkan_physical_device;
import std;
import vulkan_hpp;
import plastic.any_pointer;
import :vulkan_declaration;
import :device;
import :command_object;

namespace fyuu_rhi::vulkan {

	export enum VulkanDeviceFeature : std::uint8_t {
		VulkanFeatureValidation,
		VulkanFeatureDebugUtils,
		VulkanFeatureCounts
	};

	export struct VulkanSwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;
	};

	export using VulkanDeviceFeatures = std::bitset<static_cast<std::size_t>(VulkanDeviceFeature::VulkanFeatureCounts)>;

	export struct VulkanCommandQueueInfo {
		CommandObjectType type;
		std::optional<std::uint32_t> family;
		std::uint32_t num_available;
	};

	export class VulkanPhysicalDevice
		: public plastic::utility::EnableSharedFromThis<VulkanPhysicalDevice>,
		public IPhysicalDevice {
		friend class IPhysicalDevice;

	private:
		VulkanDeviceFeatures m_features;
		vk::UniqueInstance m_instance;
		vk::UniqueDebugUtilsMessengerEXT m_debug_utils_messenger;
		vk::PhysicalDevice m_impl;

	public:
		VulkanPhysicalDevice(RHIInitOptions const& init_options);

		VulkanDeviceFeatures GetEnabledFeatures() const noexcept;

		VulkanSwapChainSupportDetails QuerySwapChainSupport(vk::SurfaceKHR const& surface) const;

		VulkanSwapChainSupportDetails QuerySwapChainSupport(vk::UniqueSurfaceKHR const& surface) const;

		VulkanSwapChainSupportDetails QuerySwapChainSupport(VulkanSurface const& surface) const;

		VulkanSwapChainSupportDetails QuerySwapChainSupport(plastic::utility::AnyPointer<VulkanSurface> const& surface) const;

		std::vector<VulkanCommandQueueInfo> QueryAllCommandQueueInfo() const;

		vk::Instance GetInstance() const noexcept;

		vk::PhysicalDevice GetNative() const noexcept;

	};

}