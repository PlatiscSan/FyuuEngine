module;
#include <vulkan/vulkan_core.h>

export module vulkan_backend:renderer;
import vulkan_hpp;
import std;
export import rendering;
export import :command_object;
import defer;
import circular_buffer;

namespace fyuu_engine::vulkan {

	extern vk::Bool32 LoggerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
		void* user_data
	);

	extern vk::detail::DispatchLoaderDynamic& DynamicLoader(vk::UniqueInstance const& instance = vk::UniqueInstance(nullptr));

	export struct VulkanRendererTraits {
		using CommandObjectType = VulkanCommandObject;
		using CollectiveBufferType = UniqueVulkanBuffer;
		using OutputTargetType = VulkanFrameBuffer;
	};

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
		vk::Format format;
		vk::Extent2D extent;
	};

	export extern SwapChainBundle CreateSwapChainBundle(
		VulkanCore const& core,
		std::uint32_t width,
		std::uint32_t height
	);

	export extern vk::UniqueRenderPass CreateRenderPass(
		VulkanCore const& core,
		SwapChainBundle const& swap_chain_bundle
	);

	export struct FrameContext {
		vk::UniqueImageView back_buffer_view;
		vk::UniqueFramebuffer frame_buffer;
		vk::UniqueSemaphore frame_available_semaphore;
		vk::UniqueSemaphore render_finished_semaphore;
		vk::UniqueFence in_flight_fence;

		concurrency::CircularBuffer<vk::CommandBuffer, 128u> ready_commands;

	};


	export enum class BufferPoolType : std::uint8_t {
		Unknown,
		/// @brief 
		SmallBuffer,
		/// @brief 
		MediumBuffer,
		/// @brief 
		LargeBuffer,
		/// @brief 
		SmallTexture,
		/// @brief 
		MediumTexture,
		/// @brief 
		LargeTexture,
		/// @brief 
		Upload,
		/// @brief 
		CoherentUpload,
		/// @brief 
		ReadBack,
		Count
	};

	export using BufferPoolSizeMap = std::unordered_map<BufferPoolType, std::pair<std::size_t, std::size_t>>;

	export template <class DerivedVulkanRenderer> class BaseVulkanRenderer
		: public core::BaseRenderer<DerivedVulkanRenderer, VulkanRendererTraits> {
		friend class core::BaseRenderer<DerivedVulkanRenderer, VulkanRendererTraits>;
	protected:
		using ThreadCommandObjects = concurrency::ConcurrentHashMap<std::thread::id, VulkanCommandObject>;
		using InstanceCommandObjects = concurrency::ConcurrentHashMap<DerivedVulkanRenderer*, ThreadCommandObjects>;
		static inline InstanceCommandObjects s_command_objects;

		core::LoggerPtr m_logger;
		VulkanCore m_core;
		SwapChainBundle m_swap_chain_bundle;
		vk::UniqueRenderPass m_render_pass;
		std::vector<vk::Image> m_images;
		std::shared_ptr<concurrency::ISubscription> m_command_ready_sub;
		std::shared_ptr<concurrency::ISubscription> m_current_frame_sub;
		std::unordered_map<BufferPoolType, VulkanBufferPool> m_buffer_pools;

		std::vector<FrameContext> m_frames;

		std::uint32_t m_current_frame_index = 0;
		std::uint32_t m_previous_frame_index = 0u;

		std::atomic_flag m_allow_submission;

		bool m_is_minimized;
		bool m_is_window_alive;

		static inline vk::MemoryPropertyFlags BufferPoolTypeToMemoryPropertyFlags(BufferPoolType type) noexcept {
			switch (type) {
			case BufferPoolType::LargeTexture:
			case BufferPoolType::MediumTexture:
			case BufferPoolType::SmallTexture:
				return vk::MemoryPropertyFlagBits::eDeviceLocal;

			case BufferPoolType::LargeBuffer:
			case BufferPoolType::MediumBuffer:
			case BufferPoolType::SmallBuffer:
				return vk::MemoryPropertyFlagBits::eDeviceLocal;

			case BufferPoolType::Upload:
				return vk::MemoryPropertyFlagBits::eHostVisible;

			case BufferPoolType::CoherentUpload:
				return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

			case BufferPoolType::ReadBack:
				return vk::MemoryPropertyFlagBits::eHostVisible;

			default:
				return vk::MemoryPropertyFlagBits::eDeviceLocal;
			}
		}

		static inline vk::BufferUsageFlags BufferPoolTypeToBufferUsageFlags(BufferPoolType type) noexcept {
			switch (type) {
			case BufferPoolType::LargeTexture:
			case BufferPoolType::MediumTexture:
			case BufferPoolType::SmallTexture:
				return vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

			case BufferPoolType::LargeBuffer:
			case BufferPoolType::MediumBuffer:
			case BufferPoolType::SmallBuffer:
				return vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer |
					vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

			case BufferPoolType::Upload:
			case BufferPoolType::CoherentUpload:
				return vk::BufferUsageFlagBits::eTransferSrc;

			case BufferPoolType::ReadBack:
				return vk::BufferUsageFlagBits::eTransferDst;

			default:
				return vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
			}
		}

		void CreateFrames() {

			std::size_t image_count = m_images.size();
			m_frames.reserve(image_count);

			for (std::size_t i = 0; i < image_count; ++i) {

				/*
				*	Create image view
				*/

				vk::ImageViewCreateInfo image_view_info(
					{},
					m_images[i],
					vk::ImageViewType::e2D,
					m_swap_chain_bundle.format,
					{}, // default component 
					vk::ImageSubresourceRange(
						vk::ImageAspectFlagBits::eColor,
						0, 1, // baseMipLevel, levelCount
						0, 1  // baseArrayLayer, layerCount
					)
				);

				vk::UniqueImageView back_buffer_view = m_core.logical_device->createImageViewUnique(
					image_view_info,
					m_core.allocator
				);


				/*
				*	Create frame buffer
				*/

				std::array attachments = { *back_buffer_view };

				vk::FramebufferCreateInfo frame_buffer_info(
					vk::FramebufferCreateFlags(),
					*m_render_pass,
					static_cast<std::uint32_t>(attachments.size()), attachments.data(),
					m_swap_chain_bundle.extent.width,
					m_swap_chain_bundle.extent.height,
					1 // layers
				);

				vk::UniqueFramebuffer frame_buffer = m_core.logical_device->createFramebufferUnique(
					frame_buffer_info, 
					m_core.allocator
				);

				/*
				*	Create sync objects
				*/

				vk::SemaphoreCreateInfo semaphore_info;
				vk::UniqueSemaphore next_frame_available_semaphore = m_core.logical_device->createSemaphoreUnique(
					semaphore_info, m_core.allocator
				);
				vk::UniqueSemaphore frame_available_semaphore = m_core.logical_device->createSemaphoreUnique(
					semaphore_info, m_core.allocator
				);
				vk::UniqueSemaphore render_finished_semaphore = m_core.logical_device->createSemaphoreUnique(
					semaphore_info, m_core.allocator
				);

				vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
				vk::UniqueFence in_flight_fence = m_core.logical_device->createFenceUnique(
					fence_info, m_core.allocator
				);

				FrameContext frame;
				frame.back_buffer_view = std::move(back_buffer_view);
				frame.frame_buffer = std::move(frame_buffer);
				frame.frame_available_semaphore = std::move(frame_available_semaphore);
				frame.render_finished_semaphore = std::move(render_finished_semaphore);
				frame.in_flight_fence = std::move(in_flight_fence);

				m_frames.emplace_back(std::move(frame));


			}
			

		}

		void WaitForLastSubmitted() {
			FrameContext& frame = m_frames[m_previous_frame_index];
			(void)m_core.logical_device->waitForFences(
				1, &(*frame.in_flight_fence),
				vk::True,
				std::numeric_limits<std::uint64_t>::max()
			);
		}


		void RecreateSwapChain() {

			WaitForLastSubmitted();

			m_core.logical_device->waitIdle();

			m_frames.clear();
			m_images.clear();
			m_swap_chain_bundle.swap_chain.reset();
			m_render_pass.reset();

			auto [width, height] = static_cast<DerivedVulkanRenderer*>(this)->GetTargetWindowWidthAndHeight();

			m_swap_chain_bundle = CreateSwapChainBundle(m_core, width, height);
			m_render_pass = CreateRenderPass(m_core, m_swap_chain_bundle);
			m_images = m_core.logical_device->getSwapchainImagesKHR(*(m_swap_chain_bundle.swap_chain));
			CreateFrames();
			m_current_frame_index = GetNextFrameIndex();

		}

		std::uint32_t GetNextFrameIndex() {

			auto& previous_frame = m_frames[m_previous_frame_index];

			/*
			*	Wait for last commands submission done
			*/
			(void)m_core.logical_device->waitForFences(
				1, &(*previous_frame.in_flight_fence),
				vk::True, std::numeric_limits<std::uint64_t>::max()
			);

			/*
			*	Get next frame
			*/

			auto [_, next_frame_index] = m_core.logical_device->acquireNextImageKHR(
				*m_swap_chain_bundle.swap_chain,
				std::numeric_limits<uint64_t>::max(),
				*previous_frame.frame_available_semaphore,
				nullptr
			);

			return next_frame_index;

		}

		bool BeginFrameImpl() try {

			if (m_is_minimized || !m_is_window_alive) {
				return false;
			}


			auto& current_frame = m_frames[m_current_frame_index];

			/*
			*	wait for frame to be ready
			*/
			(void)m_core.logical_device->waitForFences(
				1, &(*current_frame.in_flight_fence),
				vk::True, std::numeric_limits<std::uint64_t>::max()
			);

			/*
			*	reset fence
			*/
			(void)m_core.logical_device->resetFences(1, &(*current_frame.in_flight_fence));


			/*
			*	notify the awaiting rendering thread
			*/

			m_allow_submission.test_and_set(std::memory_order::release);
			m_allow_submission.notify_all();

			return true;

		}
		catch (vk::OutOfDateKHRError const& ex) {
			if (!m_is_minimized && m_is_window_alive) {
				m_logger->Log(
					core::LogLevel::Info,
					ex.what()
				);
				RecreateSwapChain();
			}
			return false;
		}
		catch (vk::SystemError const& ex) {
			m_logger->Log(
				core::LogLevel::Error,
				ex.what()
			);
			return false;
		}

		void EndFrameImpl() try {

			FrameContext& current_frame = m_frames[m_current_frame_index];
			FrameContext& previous_frame = m_frames[m_previous_frame_index];

			m_allow_submission.clear(std::memory_order::release);
			std::vector<vk::CommandBuffer> ready_commands;


			while (auto ready_command = current_frame.ready_commands.TryPopFront()) {
				ready_commands.emplace_back(std::move(*ready_command));
			}
			

			/*
			*	Present
			*/

			vk::SubmitInfo submit_info{};
			std::array wait_semaphores = { *previous_frame.frame_available_semaphore };
			std::array wait_stages = { vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput) };
			submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
			submit_info.pWaitSemaphores = wait_semaphores.data();
			submit_info.pWaitDstStageMask = wait_stages.data();

			submit_info.commandBufferCount = static_cast<std::uint32_t>(ready_commands.size());
			submit_info.pCommandBuffers = ready_commands.data();

			std::array signal_semaphores = { *current_frame.render_finished_semaphore };
			submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
			submit_info.pSignalSemaphores = signal_semaphores.data();

			m_core.graphics_queue.submit(submit_info, *current_frame.in_flight_fence);

			vk::PresentInfoKHR present_info = {};
			present_info.waitSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
			present_info.pWaitSemaphores = signal_semaphores.data();

			std::array swap_chains = { *m_swap_chain_bundle.swap_chain };
			present_info.pSwapchains = swap_chains.data();
			present_info.swapchainCount = static_cast<std::uint32_t>(swap_chains.size());
			present_info.pImageIndices = &m_current_frame_index;

			(void)m_core.present_queue.presentKHR(present_info);

			m_previous_frame_index = m_current_frame_index;
			m_current_frame_index = GetNextFrameIndex();


		}
		catch (vk::OutOfDateKHRError const& ex) {
			if (!m_is_minimized && m_is_window_alive) {
				m_logger->Log(
					core::LogLevel::Info,
					ex.what()
				);
				RecreateSwapChain();
			}
		}
		catch (vk::SystemError const& ex) {
			m_logger->Log(
				core::LogLevel::Error,
				ex.what()
			);
		}

		VulkanCommandObject& GetCommandObjectImpl() {

			auto thread_command_objects = s_command_objects.find(static_cast<DerivedVulkanRenderer*>(this));
			if (!thread_command_objects) {
				throw std::runtime_error("no renderer registration");
			}

			auto [command_object_ptr, success] = thread_command_objects->try_emplace(
				std::this_thread::get_id(),
				&(static_cast<DerivedVulkanRenderer*>(this))->m_message_bus, 
				m_frames.size(),
				*m_core.logical_device, 
				*m_core.graphics_queue_family, 
				m_core.allocator
			);

			static thread_local util::Defer gc(
				[]() {
					auto modifier = s_command_objects.LockedModifier();
					for (auto& [instance, thread_command_objects] : modifier) {
						thread_command_objects.erase(std::this_thread::get_id());
					}
				}
			);

			return *command_object_ptr;

		}

		VulkanBufferPool* PickBufferPool(core::BufferDescriptor const& desc) {

			VulkanBufferPool* buffer_pool;

			std::array buffer_pools = {
				&m_buffer_pools[BufferPoolType::SmallBuffer],
				&m_buffer_pools[BufferPoolType::MediumBuffer],
				&m_buffer_pools[BufferPoolType::LargeBuffer],
				&m_buffer_pools[BufferPoolType::Upload],
				&m_buffer_pools[BufferPoolType::ReadBack],
			};


			switch (desc.buffer_type) {
			case core::BufferType::Upload:
				buffer_pool = buffer_pools[3];
				break;

			case core::BufferType::Staging:
				buffer_pool = buffer_pools[4];
				break;

			default:

				for (std::size_t i = 0; i < 3u; ++i) {
					auto buffer_pool_ = buffer_pools[i];
					buffer_pool = desc.size < buffer_pool_->GetBlockSize() ?
						buffer_pool_ :
						nullptr;
				}

			}

			return buffer_pool;

		}

		UniqueVulkanBuffer CreateBufferImpl(core::BufferDescriptor const& desc) {

			VulkanBufferPool* buffer_pool = PickBufferPool(desc);

			if (!buffer_pool) {
				throw std::out_of_range("No suitable pool");
			}

			std::optional<VulkanBufferAllocation> allocation = buffer_pool->Allocate(desc.size, desc.alignment);

			return UniqueVulkanBuffer(std::move(*allocation), VulkanBufferCollector(buffer_pool));

		}

		VulkanFrameBuffer OutputTargetOfCurrentFrameImpl() {
			auto [width, height] = static_cast<DerivedVulkanRenderer*>(this)->GetTargetWindowWidthAndHeight();
			return VulkanFrameBuffer{ 
				*m_render_pass, 
				*m_frames[m_current_frame_index].frame_buffer, 
				m_images[m_current_frame_index],
				{vk::Offset2D{}, vk::Extent2D{ width, height }} 
			};
		}

		void OnCommandReady(VulkanCommandBufferReady command_ready) {
			while (!m_allow_submission.test(std::memory_order::acquire)) {
				m_allow_submission.wait(false, std::memory_order::relaxed);
			}
			auto& current_frame = m_frames[m_current_frame_index];
			current_frame.ready_commands.emplace_back(*command_ready.buffer);
		}

		void OnFrameIndexRequest(VulkanFrameIndexRequest request) {
			while (!m_allow_submission.test(std::memory_order::acquire)) {
				m_allow_submission.wait(false, std::memory_order::relaxed);
			}
			request.current_frame_index = m_current_frame_index;
		}

	public:
		using Base = core::BaseRenderer<DerivedVulkanRenderer, VulkanRendererTraits>;

		BaseVulkanRenderer(
			core::LoggerPtr const& logger,
			VulkanCore&& core,
			SwapChainBundle&& swap_chain_bundle,
			vk::UniqueRenderPass&& render_pass,
			BufferPoolSizeMap const& pool_size_map
		) : Base(logger),
			m_core(std::move(core)),
			m_swap_chain_bundle(std::move(swap_chain_bundle)),
			m_render_pass(std::move(render_pass)),
			m_images(m_core.logical_device->getSwapchainImagesKHR(*(m_swap_chain_bundle.swap_chain))),
			m_command_ready_sub(
				static_cast<DerivedVulkanRenderer*>(this)->m_message_bus.Subscribe<VulkanCommandBufferReady>(
					[this](VulkanCommandBufferReady command_ready) {
						OnCommandReady(command_ready);
					}
				)
			),
			m_current_frame_sub(
				static_cast<DerivedVulkanRenderer*>(this)->m_message_bus.Subscribe<VulkanFrameIndexRequest>(
					[this](VulkanFrameIndexRequest request) {
						OnFrameIndexRequest(request);
					}
				)
			) {

			for (auto& [buffer_pool_type, pair] : pool_size_map) {
				auto& [buffer_size, min_allocation] = pair;
				m_buffer_pools.try_emplace(
					buffer_pool_type,
					m_core.physical_device,
					*m_core.logical_device,
					BufferPoolTypeToBufferUsageFlags(buffer_pool_type),
					BufferPoolTypeToMemoryPropertyFlags(buffer_pool_type),
					buffer_size,
					min_allocation
				);
			}

			(void)s_command_objects.try_emplace(static_cast<DerivedVulkanRenderer*>(this), ThreadCommandObjects());
			CreateFrames();
			m_current_frame_index = GetNextFrameIndex();

		}

		~BaseVulkanRenderer() noexcept {
			WaitForLastSubmitted();
			m_core.logical_device->waitIdle();
			(void)s_command_objects.erase(static_cast<DerivedVulkanRenderer*>(this));
		}

		
	};

	export template <class DerivedVulkanRendererBuilder, class DerivedVulkanRenderer> class BaseVulkanRendererBuilder
		: public core::BaseRendererBuilder<BaseVulkanRendererBuilder<DerivedVulkanRendererBuilder, DerivedVulkanRenderer>, DerivedVulkanRenderer> {
	private:
		static inline BufferPoolSizeMap CreateDefaultMap() {
			BufferPoolSizeMap map;
			map.try_emplace(
				BufferPoolType::SmallBuffer,
				4 * 1024 * 1024,	// 4MB pool size
				64 * 1024			// 64KB minimum allocation
			);
			map.try_emplace(
				BufferPoolType::MediumBuffer,
				16 * 1024 * 1024,	// 16MB pool size
				256 * 1024			// 256KB minimum allocation
			);
			map.try_emplace(
				BufferPoolType::LargeBuffer,
				64 * 1024 * 1024,	// 64MB pool size
				1 * 1024 * 1024		// 1M minimum allocation
			);
			map.try_emplace(
				BufferPoolType::SmallTexture,
				4 * 1024 * 1024,	// 16MB pool size
				64 * 1024			// 64KB minimum allocation
			);
			map.try_emplace(
				BufferPoolType::MediumTexture,
				64 * 1024 * 1024,	// 64MB pool size
				1 * 1024 * 1024		// 1M minimum allocation
			);
			map.try_emplace(
				BufferPoolType::LargeTexture,
				256 * 1024 * 1024,	// 256MB pool size
				4 * 1024 * 1024		// 4M minimum allocation
			);
			map.try_emplace(
				BufferPoolType::MediumTexture,
				64 * 1024 * 1024,	// 64MB pool size
				1 * 1024 * 1024		// 1M minimum allocation
			);
			map.try_emplace(
				BufferPoolType::Upload,
				16 * 1024 * 1024,	// 16MB pool size
				64 * 1024			// 64KB minimum allocation
			);
			map.try_emplace(
				BufferPoolType::CoherentUpload,
				16 * 1024 * 1024,	// 16MB pool size
				64 * 1024			// 64KB minimum allocation
			);
			map.try_emplace(
				BufferPoolType::ReadBack,
				16 * 1024 * 1024,	// 16MB pool size
				64 * 1024			// 64KB minimum allocation
			);

			return map;

		}

	protected:
		friend class core::BaseRendererBuilder<BaseVulkanRendererBuilder<DerivedVulkanRendererBuilder, DerivedVulkanRenderer>, DerivedVulkanRenderer>;

		static inline std::array const s_validation_layers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<char const*> m_instance_extensions = {
			vk::EXTDebugUtilsExtensionName,
			vk::KHRPortabilityEnumerationExtensionName,
			vk::KHRSurfaceExtensionName,
		};

		std::vector<char const*> m_device_extensions = {
			vk::KHRSwapchainExtensionName,
			/*vk::EXTMapMemoryPlacedExtensionName,*/
		};

		BufferPoolSizeMap m_buffer_pool_size_map = CreateDefaultMap();

		vk::AllocationCallbacks const* m_allocator = nullptr;
		std::string m_application_name;
		std::string m_engine_name;

		static std::pair<std::optional<std::uint32_t>, std::optional<std::uint32_t>> FindQueueFamilies(
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
				m_application_name.data(),
				vk::makeVersion(1, 0, 0), // application version
				m_engine_name.data(),
				vk::makeVersion(1, 0, 0), // engine version
				vk::makeVersion(1, 0, 0) // Vulkan API version
			);

			char const* const* instance_extensions = m_instance_extensions.data();

			vk::InstanceCreateInfo instance_info(
				vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
				&app_info,
				0, nullptr, // enabled layers
				static_cast<std::uint32_t>(m_instance_extensions.size()), instance_extensions
			);

			auto available_layers = vk::enumerateInstanceLayerProperties();

#ifndef NDEBUG
			bool validation_layers_available
				= std::any_of(
					s_validation_layers.begin(), s_validation_layers.end(),
					[&available_layers](char const* layer_name) {
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

#endif // !NDEBUG

			return vk::createInstanceUnique(instance_info, m_allocator);

		}

		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic> BindLogger(vk::UniqueInstance const& instance) {

			if (!this->m_logger) {
				return vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::detail::DispatchLoaderDynamic>(nullptr);
			}

			vk::DebugUtilsMessengerCreateInfoEXT create_info(
				vk::DebugUtilsMessengerCreateFlagsEXT(),
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
				LoggerCallback,
				this->m_logger.Get()
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

#ifndef NDEBUG
			create_info.enabledLayerCount = static_cast<std::uint32_t>(s_validation_layers.size());
			create_info.ppEnabledLayerNames = s_validation_layers.data();
#endif // !NDEBUG


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

			vk::UniqueSurfaceKHR surface = static_cast<DerivedVulkanRendererBuilder*>(this)->CreateSurface(instance);
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

		std::shared_ptr<DerivedVulkanRenderer> BuildImpl() {
			return static_cast<DerivedVulkanRendererBuilder*>(this)->BuildImpl();
		}

		void BuildImpl(std::optional<DerivedVulkanRenderer>& buffer) {
			static_cast<DerivedVulkanRendererBuilder*>(this)->BuildImpl(buffer);
		}

		void BuildImpl(std::span<std::byte> buffer) {
			static_cast<DerivedVulkanRendererBuilder*>(this)->BuildImpl(buffer);
		}

	public:
		using Base = core::BaseRendererBuilder<BaseVulkanRendererBuilder<DerivedVulkanRendererBuilder, DerivedVulkanRenderer>, DerivedVulkanRenderer>;

		template <class ApplicationName>
			requires std::constructible_from<std::string, ApplicationName>
		DerivedVulkanRendererBuilder& SetApplicationName(ApplicationName&& application_name) noexcept {
			m_application_name = std::forward<ApplicationName>(application_name);
			return static_cast<DerivedVulkanRendererBuilder&>(*this);
		}

		template <class EngineName>
			requires std::constructible_from<std::string, EngineName>
		DerivedVulkanRendererBuilder& SetEngineName(EngineName&& engine_name) noexcept {
			m_engine_name = std::forward<EngineName>(engine_name);
			return static_cast<DerivedVulkanRendererBuilder&>(*this);
		}

		DerivedVulkanRendererBuilder& AddInstanceExtension(char const* extension) noexcept {
			m_instance_extensions.push_back(extension);
			return static_cast<DerivedVulkanRendererBuilder&>(*this);
		}

		DerivedVulkanRendererBuilder& AddDeviceExtension(char const* extension) noexcept {
			m_device_extensions.push_back(extension);
			return static_cast<DerivedVulkanRendererBuilder&>(*this);
		}

		DerivedVulkanRendererBuilder& SetAllocator(vk::AllocationCallbacks const* allocator) noexcept {
			m_allocator = allocator;
			return static_cast<DerivedVulkanRendererBuilder&>(*this);
		}

		DerivedVulkanRendererBuilder& SetHeapPoolSizeMap(BufferPoolSizeMap const& map) noexcept {
			m_buffer_pool_size_map = map;
			return *this;
		}

	};


}
