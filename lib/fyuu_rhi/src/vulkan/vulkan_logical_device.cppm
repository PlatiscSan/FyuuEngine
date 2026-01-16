module;
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

export module fyuu_rhi:vulkan_logical_device;
import std;
import vulkan_hpp;
import <vma/vk_mem_alloc.h>;
import plastic.any_pointer;
import plastic.registrable;
import :device;
import :command_object;
import :vulkan_declaration;
import :vulkan_queue_allocator;


namespace fyuu_rhi::vulkan {

	namespace details {
		using UniqueVMAAllocator = std::unique_ptr<std::remove_pointer_t<VmaAllocator>, decltype(&vmaDestroyAllocator)>;
	}

	export class VulkanLogicalDevice 
		: public plastic::utility::Registrable<VulkanLogicalDevice>,
		public plastic::utility::EnableSharedFromThis<VulkanLogicalDevice>,
		public ILogicalDevice {
		friend class ILogicalDevice;

	private:
		VulkanQueueAllocator m_queue_alloc;
		vk::UniqueDevice m_impl;
		std::shared_ptr<vk::DispatchLoaderDynamic> m_dispatcher;
		details::UniqueVMAAllocator m_video_memory_alloc;

		vk::UniqueDescriptorSetLayout m_bindless_texture_descriptor_set_layout;
		vk::UniqueDescriptorSetLayout m_bindless_buffer_descriptor_set_layout;

	public:
		VulkanLogicalDevice(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			VulkanQueueOptions const& queue_options = VulkanQueueOptions::PlatformDefault()
		);

		VulkanLogicalDevice(
			VulkanPhysicalDevice const& physical_device,
			VulkanQueueOptions const& queue_options = VulkanQueueOptions::PlatformDefault()
		);

		VulkanLogicalDevice(VulkanLogicalDevice&& other) noexcept;

		void SetDebugNameForResource(void* resource, vk::ObjectType type, std::string_view debug_name);

		vk::Device GetNative() const noexcept;

		std::shared_ptr<vk::DispatchLoaderDynamic> GetDispatcher() const noexcept;

		vk::DispatchLoaderDynamic* GetRawDispatcher() const noexcept;

		VmaAllocator GetVideoMemoryAllocator() const noexcept;

		UniqueVulkanCommandQueueInfo AllocateCommandQueue(CommandObjectType type, QueuePriority priority) noexcept;

	};


}
