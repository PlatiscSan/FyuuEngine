module;

export module graphics:vulkan;
import :interface;
import window;
import std;
export import vulkan_hpp;

namespace graphics::api::vulkan {

	struct QueueFamilyIndices {
		std::optional<std::uint32_t> graphics_family;
		std::optional<std::uint32_t> present_family;

		bool IsComplete() const noexcept {
			return graphics_family.has_value() && present_family.has_value();
		}
	};

	extern bool CheckLayerSupport(std::string_view layer_name);
	extern bool CheckExtensionSupport(std::string_view extension_name);
	extern bool CheckDeviceExtensionSupport(vk::PhysicalDevice const& device, std::vector<char const*> const& extensions);

	template <std::convertible_to<std::vector<char const*>> Extensions>
	vk::UniqueInstance CreateVkInstance(Extensions&& extensions, vk::AllocationCallbacks const* allocator = nullptr) {

		std::vector<char const*> layers;
		vk::InstanceCreateInfo create_info;

#ifndef NDEBUG
		if (CheckLayerSupport("VK_LAYER_LUNARG_standard_validation")) {
			layers.emplace_back("VK_LAYER_LUNARG_standard_validation");
		}
		extensions.emplace_back(vk::EXTDebugReportExtensionName);
#endif // !NDEBUG

		if (CheckExtensionSupport(vk::KHRGetDisplayProperties2ExtensionName)) {
			extensions.emplace_back(vk::KHRGetDisplayProperties2ExtensionName);
		}

		if (CheckExtensionSupport(vk::KHRPortabilityEnumerationExtensionName)) {
			extensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
			create_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
		}

		create_info.enabledLayerCount = layers.size();
		create_info.ppEnabledLayerNames = layers.data();
		create_info.enabledExtensionCount = extensions.size();
		create_info.ppEnabledLayerNames = extensions.data();

		return vk::createInstanceUnique(create_info, allocator);

	}

	extern vk::PhysicalDevice PickPhysicalDevice(
		vk::UniqueInstance const& instance,
		vk::SurfaceKHR const& surface,
		std::vector<char const*> const& extensions
	);

	extern vk::UniqueDevice CreateLogicDevice(
		vk::PhysicalDevice const& physical_device, 
		vk::SurfaceKHR const& surface, 
		std::vector<char const*> const& extensions
	);

	extern vk::Queue GetGraphicsQueue(
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface, 
		vk::UniqueDevice const& device
	);

	extern vk::Queue GetPresentQueue(
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface, 
		vk::UniqueDevice const& device
	);

	extern vk::SwapchainKHR CreateSwapChain(
		platform::IWindow* window,
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface,
		vk::UniqueDevice const& device
	);

	extern vk::Format GetSwapChainImageFormat(
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface
	);

	extern vk::Extent2D GetSwapChainExtent(
		platform::IWindow* window,
		vk::PhysicalDevice const& physical_device,
		vk::SurfaceKHR const& surface
	);

	extern std::vector<vk::ImageView> CreateSwapChainImageViews(
		vk::UniqueDevice const& device,
		std::vector<vk::Image> const& swap_chain_image,
		vk::Format swap_chain_image_format
	);
	
	extern vk::RenderPass CreateRenderPass(vk::UniqueDevice const& device, vk::Format const& swap_chain_image_format);

	export class BaseVulkanRenderDevice : public BaseRenderDevice {
	private:
		vk::AllocationCallbacks const* m_allocator;
		vk::UniqueInstance m_instance;
		vk::DebugUtilsMessengerEXT m_callback;
		vk::SurfaceKHR m_surface;
		vk::PhysicalDevice m_physical_device;
		vk::UniqueDevice m_device;
		
		vk::Queue m_graphics_queue;
		vk::Queue m_present_queue;
		
		vk::SwapchainKHR m_swap_chain;
		std::vector<vk::Image> m_swap_chain_images;
		vk::Format m_swap_chain_image_format;
		vk::Extent2D m_swap_chain_extent;
		std::vector<vk::ImageView> m_swap_chain_image_views;

		vk::RenderPass m_render_pass;
		vk::Pipeline m_graphics_pipeline;
		std::vector<vk::Framebuffer> m_swap_chain_frame_buffers;

		vk::UniqueDescriptorPool m_descriptor_pool;
		vk::PipelineCache m_pipeline_cache;

		static vk::DebugUtilsMessengerEXT SetupLogger(BaseVulkanRenderDevice* device);

	public:
		template <std::convertible_to<core::LoggerPtr> Logger, std::convertible_to<std::vector<char const*>> Extensions, std::convertible_to<std::function<vk::SurfaceKHR(vk::Instance)>> SurfaceCreator>
		BaseVulkanRenderDevice(
			Logger&& logger, 
			platform::IWindow& window, 
			vk::AllocationCallbacks const* allocator, 
			Extensions&& extensions, 
			SurfaceCreator&& surface_creator
		) : BaseRenderDevice(std::forward<Logger>(logger), window),
			m_allocator(allocator),
			m_instance(CreateVkInstance(std::forward<Extensions>(extensions), m_allocator)),
			m_callback(BaseVulkanRenderDevice::SetupLogger(this)),
			m_surface(surface_creator(m_instance.get())),
			m_physical_device(PickPhysicalDevice(m_instance, m_surface, std::forward<Extensions>(extensions))),
			m_device(CreateLogicDevice(m_physical_device, m_surface, std::forward<Extensions>(extensions))),
			m_graphics_queue(GetGraphicsQueue(m_physical_device, m_surface, m_device)),
			m_present_queue(GetPresentQueue(m_physical_device, m_surface, m_device)),
			m_swap_chain(CreateSwapChain(m_window, m_physical_device, m_surface, m_device)),
			m_swap_chain_images(m_device->getSwapchainImagesKHR(m_swap_chain)),
			m_swap_chain_image_format(GetSwapChainImageFormat(m_physical_device, m_surface)),
			m_swap_chain_extent(GetSwapChainExtent(m_window, m_physical_device, m_surface)),
			m_swap_chain_image_views(CreateSwapChainImageViews(m_device, m_swap_chain_images, m_swap_chain_image_format)),
			m_render_pass(CreateRenderPass(m_device, m_swap_chain_image_format)) {

		}

		virtual ~BaseVulkanRenderDevice() noexcept override = default;

		virtual void Clear(float r, float g, float b, float a) override;
		virtual void SetViewport(int x, int y, uint32_t width, uint32_t height) override;

		virtual bool BeginFrame() override;

		virtual void EndFrame() override;

	};

}