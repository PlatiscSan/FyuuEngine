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

//#include <vma/vk_mem_alloc.h>

module fyuu_rhi:vulkan_logical_device;
import vulkan_hpp;
import plastic.other;

namespace fyuu_rhi::vulkan {

	static vk::UniqueDevice CreateVulkanLogicalDevice(
		VulkanQueueAllocator const& queue_alloc,
		VulkanQueueOptions const& queue_options,
		vk::PhysicalDevice const& physical_device,
		VulkanDeviceFeatures const& features
	) {

		vk::PhysicalDeviceGraphicsPipelineLibraryFeaturesEXT lib_features{};
		vk::PhysicalDeviceFragmentShaderInterlockFeaturesEXT fse_features{};
		vk::PhysicalDeviceVulkan14Features vulkan1_4_features{};
		vk::PhysicalDeviceVulkan13Features vulkan1_3_features{};
		vk::PhysicalDeviceVulkan12Features vulkan1_2_features{};
		vk::PhysicalDeviceVulkan11Features vulkan1_1_features{};
		vk::PhysicalDeviceCustomBorderColorFeaturesEXT custom_border_color{};

		fse_features.setPNext(&lib_features);
		vulkan1_4_features.setPNext(&fse_features);
		vulkan1_3_features.setPNext(&vulkan1_4_features);
		vulkan1_2_features.setPNext(&vulkan1_3_features);
		vulkan1_1_features.setPNext(&vulkan1_2_features);
		custom_border_color.setPNext(&vulkan1_1_features);

		vk::PhysicalDeviceFeatures2 device_features2{};
		device_features2.setPNext(&custom_border_color);

		physical_device.getFeatures2(&device_features2);

		if (vulkan1_2_features.imagelessFramebuffer == vk::False) {
			throw std::runtime_error("Imageless framebuffer is not supported");
		}
		if (vulkan1_3_features.dynamicRendering == vk::False) {
			throw std::runtime_error("Dynamic rendering is not supported");
		}
		if (vulkan1_2_features.scalarBlockLayout == vk::False) {
			throw std::runtime_error("ScalarBlockLayout is not supported");
		}
		if (custom_border_color.customBorderColors == vk::False) {
			throw std::runtime_error("CustomBorderColor is not supported");
		}
		if (vulkan1_2_features.samplerFilterMinmax == vk::False) {
			throw std::runtime_error("Minmax Sampler is not supported");
		}
		if (vulkan1_1_features.shaderDrawParameters == vk::False) {
			throw std::runtime_error("Shader Draw Parameters (baseInstance et al) are not supported.");
		}
		if (fse_features.fragmentShaderPixelInterlock == vk::False) {
			throw std::runtime_error("Fragment Shader Interlock is not supported");
		}
		if (lib_features.graphicsPipelineLibrary == vk::False) {
			throw std::runtime_error("Graphics Pipeline Library is not supported");
		}

		constexpr static char const* const device_extensions[] = {
			vk::KHRSwapchainExtensionName,
			vk::KHRPushDescriptorExtensionName,
			vk::EXTCustomBorderColorExtensionName,
			vk::EXTFragmentShaderInterlockExtensionName,
			vk::KHRPipelineLibraryExtensionName,
			vk::EXTGraphicsPipelineLibraryExtensionName,
#if !defined(__ANDROID__)    // only 5% of android devices have this extension so we have to go without it
			vk::EXTMemoryBudgetExtensionName
#endif
		};

		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

		queue_create_infos.emplace_back(
			vk::DeviceQueueCreateFlags{},
			queue_alloc.GetFamily(CommandObjectType::AllCommands),
			queue_alloc.GetTotalQueue(CommandObjectType::AllCommands),
			queue_alloc.GetPriority(CommandObjectType::AllCommands).data(),
			nullptr
		);

		queue_create_infos.emplace_back(
			vk::DeviceQueueCreateFlags{},
			queue_alloc.GetFamily(CommandObjectType::Compute),
			queue_alloc.GetTotalQueue(CommandObjectType::Compute),
			queue_alloc.GetPriority(CommandObjectType::Compute).data(),
			nullptr
		);

		queue_create_infos.emplace_back(
			vk::DeviceQueueCreateFlags{},
			queue_alloc.GetFamily(CommandObjectType::Copy),
			queue_alloc.GetTotalQueue(CommandObjectType::Copy),
			queue_alloc.GetPriority(CommandObjectType::Copy).data(),
			nullptr
		);

		vk::DeviceCreateInfo device_create_info(
			/* flags_ */					{},
			/* queueCreateInfoCount_ */		static_cast<std::uint32_t>(queue_create_infos.size()),
			/* pQueueCreateInfos_ */		queue_create_infos.data(),
			/* enabledLayerCount_ */		0u,
			/* ppEnabledLayerNames_ */		nullptr,
			/* enabledExtensionCount_ */	static_cast<std::uint32_t>(std::size(device_extensions)),
			/* ppEnabledExtensionNames_ */	device_extensions,
			/* pEnabledFeatures_ */			nullptr,
			/* pNext_ */					&device_features2
		);

		constexpr static const char* const validation_layers[] = {
			"VK_LAYER_KHRONOS_validation",
#ifdef RECONSTRUCT
			"VK_LAYER_LUNARG_gfxreconstruct"
#endif
#ifdef APIDUMP
			"VK_LAYER_LUNARG_api_dump",
#endif
		};

		if (features.test(static_cast<std::size_t>(VulkanDeviceFeature::VulkanFeatureValidation))) {
			device_create_info.enabledLayerCount = static_cast<std::uint32_t>(std::size(validation_layers));
			device_create_info.ppEnabledLayerNames = validation_layers;
		}

		return physical_device.createDeviceUnique(device_create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);

	}

	static std::shared_ptr<vk::DispatchLoaderDynamic> CreateDispatcher(vk::Device const& device) {
		auto dispatcher = std::make_shared<vk::DispatchLoaderDynamic>(vk::detail::defaultDispatchLoaderDynamic);
		dispatcher->init(device);
		return dispatcher;
	}


	static details::UniqueVMAAllocator CreateVideoMemoryAllocator(
		vk::Instance const& instance,
		vk::PhysicalDevice const& physical_device,
		vk::Device const& logical_device
	) {

		VmaVulkanFunctions vulkan_functions = {};
		vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
		allocator_create_info.physicalDevice = physical_device;
		allocator_create_info.device = logical_device;
		allocator_create_info.instance = instance;
		allocator_create_info.pVulkanFunctions = &vulkan_functions;

		VmaAllocator allocator;
		vmaCreateAllocator(&allocator_create_info, &allocator);

		return details::UniqueVMAAllocator(allocator, vmaDestroyAllocator);

	}

	static vk::UniqueDescriptorSetLayout CreateDescriptorSetLayout(vk::DescriptorType type) {
		
		//vk::DescriptorBindingFlags binding_flags 
		//	= vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind;

		//vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create(1u, &binding_flags);

		//VkDescriptorSetLayoutBinding set_layout_binding{
		//	.binding = 0,
		//	.descriptorType = descriptorType,
		//	.descriptorCount = nTextureDescriptors,
		//	.stageFlags = VK_SHADER_STAGE_ALL,
		//	.pImmutableSamplers = VK_NULL_HANDLE,
		//};
		//VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info{
		//	.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		//	.pNext = &binding_flags_create,
		//	.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		//	.bindingCount = 1,
		//	.pBindings = &set_layout_binding
		//};

		return {};

	}

	VulkanLogicalDevice::VulkanLogicalDevice(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		VulkanQueueOptions const& queue_options
	) : m_queue_alloc(physical_device->QueryAllCommandQueueInfo(), queue_options),
		m_impl(
			CreateVulkanLogicalDevice(
				m_queue_alloc,
				queue_options,
				physical_device->GetNative(),
				physical_device->GetEnabledFeatures()
			)
		),
		m_dispatcher(CreateDispatcher(*m_impl)),
		m_video_memory_alloc(CreateVideoMemoryAllocator(physical_device->GetInstance(), physical_device->GetNative(), *m_impl)) {
	}

	VulkanLogicalDevice::VulkanLogicalDevice(
		VulkanPhysicalDevice const& physical_device, 
		VulkanQueueOptions const& queue_options
	) : m_queue_alloc(physical_device.QueryAllCommandQueueInfo(), queue_options),
		m_impl(
			CreateVulkanLogicalDevice(
				m_queue_alloc,
				queue_options,
				physical_device.GetNative(),
				physical_device.GetEnabledFeatures()
			)
		),
		m_dispatcher(CreateDispatcher(*m_impl)),
		m_video_memory_alloc(CreateVideoMemoryAllocator(physical_device.GetInstance(), physical_device.GetNative(), *m_impl)) {
	}

	VulkanLogicalDevice::VulkanLogicalDevice(VulkanLogicalDevice&& other) noexcept
		: Registrable(std::move(other)),
		m_queue_alloc(std::move(other.m_queue_alloc)),
		m_impl(std::move(other.m_impl)),
		m_dispatcher(std::move(m_dispatcher)),
		m_video_memory_alloc(std::move(other.m_video_memory_alloc)) {
	}

	void VulkanLogicalDevice::SetDebugNameForResource(void* resource, vk::ObjectType type, std::string_view debug_name) {

#if !defined(_NDEBUG)

		vk::DebugUtilsObjectNameInfoEXT object_name(
			{},
			reinterpret_cast<uint64_t>(resource),
			debug_name.data()
		);

		m_impl->setDebugUtilsObjectNameEXT(object_name, *m_dispatcher);

#endif // !!defined(_NDEBUG)

	}

	vk::Device VulkanLogicalDevice::GetNative() const noexcept {
		return *m_impl;
	}

	std::shared_ptr<vk::DispatchLoaderDynamic> VulkanLogicalDevice::GetDispatcher() const noexcept {
		return m_dispatcher;
	}

	vk::DispatchLoaderDynamic* VulkanLogicalDevice::GetRawDispatcher() const noexcept {
		return m_dispatcher.get();
	}

	VmaAllocator VulkanLogicalDevice::GetVideoMemoryAllocator() const noexcept {
		return m_video_memory_alloc.get();
	}

	UniqueVulkanCommandQueueInfo VulkanLogicalDevice::AllocateCommandQueue(CommandObjectType type, QueuePriority priority) noexcept {
		return m_queue_alloc.Allocate(type, priority);
	}


}
