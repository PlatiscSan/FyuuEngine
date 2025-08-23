module;
#include <vulkan/vulkan_core.h>
//#include <vulkan/vulkan.hpp>

export module graphics:base_vulkan_renderer;
import std;
export import :vulkan_descriptor_resource;
import defer;

extern vk::Bool32 LoggerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
	void* user_data
);

namespace graphics::api::vulkan {

	extern vk::detail::DispatchLoaderDynamic& DynamicLoader(vk::UniqueInstance const& instance = vk::UniqueInstance(nullptr));

	export struct VulkanCore {
		vk::AllocationCallbacks const* allocator;
		vk::UniqueInstance instance;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> logger_callback;
		vk::UniqueSurfaceKHR surface;
		vk::PhysicalDevice physical_device;

		std::optional<std::uint32_t> graphics_queue_family;
		std::optional<std::uint32_t> present_queue_family;

		vk::UniqueDevice logical_device;
		vk::Queue graphics_queue;
		vk::Queue present_queue;
	};

	export struct SwapChainBundle {
		vk::UniqueSwapchainKHR swap_chain;
		std::vector<vk::Image> back_buffers;
		vk::Format format;
		vk::Extent2D extent;
	};

	struct FrameContext {
		vk::UniqueImageView back_buffer_view;
		vk::UniqueFramebuffer frame_buffer;
		vk::UniqueSemaphore frame_available_semaphore;
		vk::UniqueSemaphore render_finished_semaphore;
		vk::UniqueFence in_flight_fence;

		std::vector<vk::CommandBuffer> ready_commands;
		std::mutex ready_commands_mutex;

		FrameContext() noexcept = default;

		FrameContext(FrameContext&& other) noexcept;
		FrameContext& operator=(FrameContext&& other) noexcept;


	};

	extern SwapChainBundle CreateSwapChainBundle(
		VulkanCore const& core, 
		platform::IWindow* window
	);
	extern vk::UniqueRenderPass CreateRenderPass(
		VulkanCore const& core,
		SwapChainBundle const& swap_chain_bundle
	);

	export class BaseVulkanRenderDevice : public BaseRenderDevice {
	protected:
		VulkanCore m_core;
		SwapChainBundle m_swap_chain_bundle;
		vk::UniqueRenderPass m_render_pass;
		util::Subscriber* m_command_ready_subscription;
		util::Subscriber* m_render_pass_info_request_subscription;
		std::vector<FrameContext> m_frames;
		std::uint32_t m_next_frame_index = 0;
		std::uint32_t m_previous_frame_index = 0u;

		/// @brief flags indicating if the rendering thread can submit commands
		std::atomic_flag m_allow_submission;

		/// @brief should be set in the message loop
		bool m_is_minimized = false;

		/// @brief should be set in the message loop
		bool m_is_window_alive = false;

		void WaitForLastSubmitted();
		void RecreateSwapChain();
		void OnCommandReady(CommandReadyEvent const& e);
		void OnRenderPassInfoRequest(RenderPassInfoRequest const& e);
		void CreateFrames();
		std::uint32_t GetNextFrameIndex();

	public:
		template <
			std::convertible_to<core::LoggerPtr> Logger,
			std::convertible_to<VulkanCore> CompatibleVulkanCore,
			std::convertible_to<SwapChainBundle> CompatibleSwapChainBundle,
			std::convertible_to<vk::UniqueRenderPass> CompatibleRenderPass
		>
		BaseVulkanRenderDevice(
			Logger&& logger,
			platform::IWindow& window,
			CompatibleVulkanCore&& core,
			CompatibleSwapChainBundle&& swap_chain_bundle,
			CompatibleRenderPass&& render_pass
		) : BaseRenderDevice(std::forward<Logger>(logger), window),
			m_core(std::forward<CompatibleVulkanCore>(core)),
			m_swap_chain_bundle(std::forward<CompatibleSwapChainBundle>(swap_chain_bundle)),
			m_render_pass(std::forward<CompatibleRenderPass>(render_pass)),
			m_command_ready_subscription(
				m_message_bus.Subscribe<CommandReadyEvent>(
					[this](CommandReadyEvent const& e) {
						OnCommandReady(e);
					}
				)
			),
			m_render_pass_info_request_subscription(
				m_message_bus.Subscribe<RenderPassInfoRequest>(
					[this](RenderPassInfoRequest const& e) {
						OnRenderPassInfoRequest(e);
					}
				)
			){

			CreateFrames();
			m_next_frame_index = GetNextFrameIndex();

		}

		BaseVulkanRenderDevice(BaseVulkanRenderDevice const&) = delete;
		BaseVulkanRenderDevice(BaseVulkanRenderDevice&& other) noexcept;

		BaseVulkanRenderDevice& operator=(BaseVulkanRenderDevice const&) = delete;
		BaseVulkanRenderDevice& operator=(BaseVulkanRenderDevice&& other) noexcept;

		virtual ~BaseVulkanRenderDevice() noexcept;

		virtual void Clear(float r, float g, float b, float a) override;
		virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) override;

		virtual bool BeginFrame() override;
		virtual void EndFrame() override;

		virtual ICommandObject& AcquireCommandObject() override;

		API GetAPI() const noexcept override;

		VulkanCore const& Core() const noexcept;

		vk::UniqueRenderPass const& RenderPass() const noexcept;

		DescriptorResourcePool CreateDescriptorResourcePool(
			vk::DescriptorType type, 
			std::uint32_t count,
			vk::ShaderStageFlags stage_flags = vk::ShaderStageFlagBits::eAll
		) const;

		vk::UniqueDescriptorPool CreateDescriptorPool(
			vk::DescriptorType type,
			std::uint32_t count,
			vk::ShaderStageFlags stage_flags = vk::ShaderStageFlagBits::eAll
		) const;

		std::size_t FrameCount() const noexcept;

	};

	export template <class DerivedBuilder, class DerivedVulkanRenderDevice>
		class BaseVulkanRenderDeviceBuilder {
		protected:
			static inline std::array const s_validation_layers = { "VK_LAYER_KHRONOS_validation" };

			std::optional<DerivedVulkanRenderDevice> m_render_device;
			std::string m_application_name;
			std::string m_engine_name;
			std::vector<char const*> m_instance_extensions = { vk::EXTDebugUtilsExtensionName };
			std::vector<char const*> m_device_extensions;
			vk::AllocationCallbacks const* m_allocator = nullptr;
			core::LoggerPtr m_logger;
			platform::IWindow* m_window = nullptr;
			bool m_enable_validation_layers;	
		
			static std::pair<std::optional<std::uint32_t>, std::optional<std::uint32_t>> 
				FindQueueFamilies(
				vk::PhysicalDevice const& physical_device,
				vk::UniqueSurfaceKHR const& surface
			) {

				std::optional<std::uint32_t> graphics_queue_family;
				std::optional<std::uint32_t> present_queue_family;

				auto queue_families = physical_device.getQueueFamilyProperties();

				for (std::uint32_t i = 0; i < queue_families.size(); ++i) {
					auto const& queue_family = queue_families[i];

					if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
						graphics_queue_family.emplace(i);
					}

					if (physical_device.getSurfaceSupportKHR(i, *surface)) {
						present_queue_family.emplace(i);
					}
					
					if (graphics_queue_family.has_value() && present_queue_family.has_value()) {
						break;
					}
				}

				return { graphics_queue_family, present_queue_family };

			}

			bool IsDeviceSuitable(
				vk::PhysicalDevice const& physical_device, 
				vk::UniqueSurfaceKHR const& surface
			) const {

				/*
				*	query device extensions
				*/

				auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
				bool has_required_extensions
					= std::all_of(
						m_device_extensions.begin(), m_device_extensions.end(),
						[&available_extensions](char const* ext) {
							return std::any_of(
								available_extensions.begin(), available_extensions.end(),
								[&ext](vk::ExtensionProperties const& prop) {
									return std::strcmp(prop.extensionName, ext) == 0;
								}
							);
						}
					);

				if (!has_required_extensions) {
					return false; // Skip the required extensions are not available
				}

				/*
				*	query swap chain support
				*/

				try {
					vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
				}
				catch (vk::SystemError const&) {
					return false; // Skip if surface capabilities cannot be queried
				}

				auto formats = physical_device.getSurfaceFormatsKHR(*surface);
				auto present_modes = physical_device.getSurfacePresentModesKHR(*surface);
				if (formats.empty() || present_modes.empty()) {
					return false; // Skip if no formats or present modes are available
				}

				return true;

			}

			vk::UniqueInstance CreateInstance() const {

				vk::ApplicationInfo app_info(
					m_application_name.c_str(),
					vk::makeVersion(1, 0, 0), // application version
					m_engine_name.c_str(),
					vk::makeVersion(1, 0, 0), // engine version
					vk::makeVersion(1, 0, 0) // Vulkan API version
				);

				char const* const* instance_extensions = m_instance_extensions.data();

				vk::InstanceCreateInfo instance_info(
					vk::InstanceCreateFlags(),
					&app_info,
					0, nullptr, // enabled layers
					static_cast<std::uint32_t>(m_instance_extensions.size()), instance_extensions
				);

				auto available_layers = vk::enumerateInstanceLayerProperties();

				if (m_enable_validation_layers) {

					bool validation_layers_available
						= std::any_of(
							s_validation_layers.begin(), s_validation_layers.end(),
							[&available_layers](char const* layer_name){
								return std::any_of(
									available_layers.begin(), available_layers.end(),
									[layer_name](vk::LayerProperties const& prop) {
										return std::strcmp(prop.layerName, layer_name) == 0;
									}
								);
							}
						);

					if (!validation_layers_available) {
						throw std::runtime_error("validation layer not available");
					}

					instance_info.enabledLayerCount = static_cast<std::uint32_t>(s_validation_layers.size());
					instance_info.ppEnabledLayerNames = s_validation_layers.data();

				}

				return vk::createInstanceUnique(instance_info, m_allocator);

			}

			vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> 
				BindLogger(vk::UniqueInstance const& instance) {

				vk::DebugUtilsMessengerCreateInfoEXT create_info(
					vk::DebugUtilsMessengerCreateFlagsEXT(),
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
					vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
					LoggerCallback,
					m_logger.Get()
				);

				return instance->createDebugUtilsMessengerEXTUnique(create_info, m_allocator, DynamicLoader(instance));

			}

			vk::PhysicalDevice PickPhysicalDevice(
				vk::UniqueInstance const& instance,
				vk::UniqueSurfaceKHR const& surface
			) const {

				vk::PhysicalDevice picked_physical_device;

				auto physical_devices = instance->enumeratePhysicalDevices();
				for (auto const& physical_device : physical_devices) {

					auto properties = physical_device.getProperties();

					if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
						continue; // Skip non-discrete GPUs
					}

					if (!IsDeviceSuitable(physical_device, surface)) {
						continue;
					}

					picked_physical_device = physical_device;
					break;

				}

				if (!picked_physical_device) {
					/*
					*	no discrete GPU found, try to use the first GPU
					*/

					if (physical_devices.empty() || IsDeviceSuitable(physical_devices[0], surface)) {
						throw std::runtime_error("No Vulkan physical devices found.");
					}

					picked_physical_device = physical_devices[0];

				}

				return picked_physical_device;

			}

			vk::UniqueDevice CreateLogicalDevice(
				vk::PhysicalDevice const& physical_device,
				vk::UniqueSurfaceKHR const& surface,
				std::uint32_t graphics_queue_index,
				std::uint32_t present_queue_index
			) const {

				float queue_priority = 1.0f;
				std::set<std::uint32_t> unique_queue_families = { graphics_queue_index, present_queue_index };
				std::vector<vk::DeviceQueueCreateInfo> queue_infos;
				for (auto const& unique_queue_family : unique_queue_families) {
					vk::DeviceQueueCreateInfo queue_info(
						vk::DeviceQueueCreateFlags(),
						unique_queue_family,
						1,
						&queue_priority
					);
					queue_infos.emplace_back(std::move(queue_info));
				}

				vk::PhysicalDeviceFeatures device_features;

				vk::DeviceCreateInfo create_info(
					vk::DeviceCreateFlags(),
					static_cast<std::uint32_t>(queue_infos.size()),
					queue_infos.data()
				);

				create_info.pEnabledFeatures = &device_features;
				create_info.enabledExtensionCount = static_cast<std::uint32_t>(m_device_extensions.size());
				create_info.ppEnabledExtensionNames = m_device_extensions.data();

				if (m_enable_validation_layers) {
					create_info.enabledLayerCount = static_cast<std::uint32_t>(s_validation_layers.size());
					create_info.ppEnabledLayerNames = s_validation_layers.data();
				}

				return physical_device.createDeviceUnique(create_info, m_allocator);

			}

			VulkanCore CreateCore() {

				vk::UniqueInstance instance = CreateInstance();
				auto logger_callback = BindLogger(instance);

				/*
				*	create surface.
				*
				*	Note:	The derived class must implement the CreateSurface function whose signature is:
				*			vk::UniqueSurfaceKHR CreateSurface(vk::Instance& instance).
				*			A valid vk::UniqueSurfaceKHR which is created by instance should be returned.
				*/

				vk::UniqueSurfaceKHR surface = static_cast<DerivedBuilder&>(*this).CreateSurface(*instance);
				vk::PhysicalDevice physical_device = PickPhysicalDevice(instance, surface);
				auto [graphics_family, present_family] = FindQueueFamilies(physical_device, surface);
				vk::UniqueDevice logical_device = CreateLogicalDevice(physical_device, surface, *graphics_family, *present_family);
				vk::Queue graphics_queue = logical_device->getQueue(*graphics_family, 0);
				vk::Queue present_queue = logical_device->getQueue(*present_family, 0);

				VulkanCore core = {
					.allocator = m_allocator,
					.instance = std::move(instance),
					.logger_callback = std::move(logger_callback),
					.surface = std::move(surface),
					.physical_device = std::move(physical_device),
					.graphics_queue_family = std::move(graphics_family),
					.present_queue_family = std::move(present_family),
					.logical_device = std::move(logical_device),
					.graphics_queue = std::move(graphics_queue),
					.present_queue = std::move(present_queue)
				};

				return core;

			}

		public:
			template <class ApplicationName>
				requires std::constructible_from<std::string, ApplicationName>
			DerivedBuilder& SetApplicationName(ApplicationName&& application_name) noexcept {
				m_application_name = std::forward<ApplicationName>(application_name);
				return static_cast<DerivedBuilder&>(*this);
			}

			template <class EngineName>
				requires std::constructible_from<std::string, EngineName>
			DerivedBuilder& SetEngineName(EngineName&& engine_name) noexcept {
				m_engine_name = std::forward<EngineName>(engine_name);
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& AddInstanceExtension(char const* extension) noexcept {
				m_instance_extensions.push_back(extension);
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& AddDeviceExtension(char const* extension) noexcept {
				m_device_extensions.push_back(extension);
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& SetAllocator(vk::AllocationCallbacks const* allocator) noexcept {
				m_allocator = allocator;
				return static_cast<DerivedBuilder&>(*this);
			}

			template <std::convertible_to<core::LoggerPtr> Logger>
			DerivedBuilder& SetLogger(Logger&& logger) noexcept {
				m_logger = util::MakeReferred(logger);
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& SetWindow(platform::IWindow* window) noexcept {
				m_window = window;
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& EnableValidationLayers(bool enable = true) noexcept {
				m_enable_validation_layers = enable;
				return static_cast<DerivedBuilder&>(*this);
			}

			DerivedBuilder& Build() {

				/*
				*	Note:	The derived class must implement the BuildImp function whose signature is:
				*			void BuildImp(VulkanCore, SwapChainBundle, vk::UniqueRenderPass).
				*			the m_render_device should be constructed in BuildImp function.
				*/

				VulkanCore core = CreateCore();
				SwapChainBundle swap_chain_bundle = CreateSwapChainBundle(core, m_window);
				vk::UniqueRenderPass render_pass = CreateRenderPass(core, swap_chain_bundle);

				static_cast<DerivedBuilder&>(*this).BuildImp(
					std::move(core), 
					std::move(swap_chain_bundle),
					std::move(render_pass)
				);
				return static_cast<DerivedBuilder&>(*this);
			
			}

			std::optional<DerivedVulkanRenderDevice> GetRenderDevice() noexcept {
				util::Defer defer(
					[this]() {
						m_render_device.reset();
					}
				);
				return std::move(m_render_device);
			}


	};

}