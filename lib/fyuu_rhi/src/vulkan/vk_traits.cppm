/* the vulkan pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)

#endif //!defined(__APPLE__)
export module fyuu_rhi:;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi::vulkan {

}
#endif // !defined(__APPLE__)

*/

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <memory>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <string_view>
#include <optional>
#include <variant>
#include <format>
#include <ranges>
#include <span>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#include <android/android_native_app_glue.h>
#endif // defined(_WIN32)
#include <vma/vk_mem_alloc.h>
#include <boost/scope/unique_resource.hpp>


#endif //!defined(__APPLE__)
export module fyuu_rhi:vulkan_traits;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :core_types;
import :vulkan_queue_allocator;
import :resource_types;
import :sampler_types;
import :scheduler_types;
import :pipeline_types;
import :native_pipeline_binding;

namespace fyuu_rhi::vulkan {

	using namespace fyuu_rhi::pipeline;
	using namespace fyuu_rhi::execution;

	export struct Backend {

		struct Instance {
			vk::detail::DispatchLoaderDynamic const& dispatcher;
			std::vector<std::string> enabled_extensions;
			vk::SharedInstance impl;
			vk::SharedDebugUtilsMessengerEXT debug_messenger;
		};
		
		struct PhysicalDevice {
			Instance const& instance;
			vk::SharedPhysicalDevice impl;
		};

		using Surface = vk::SharedSurfaceKHR;

		struct VMAAllocator {
			VmaAllocator impl;
			~VMAAllocator() noexcept {
				if (impl) {
					vmaDestroyAllocator(impl);
				}
			}
		};

		struct LogicalDevice {
			PhysicalDevice phys_dev;
			QueueAllocator queue_alloc;
			std::vector<std::string> enabled_extensions;
			vk::SharedDevice impl;
			std::shared_ptr<vk::detail::DispatchLoaderDynamic> dispatcher;
			std::shared_ptr<VMAAllocator> mem_alloc;
		};

		struct VulkanScheduler {
			std::shared_ptr<ManagedQueue> allocation;
			vk::SharedQueue queue;
			SchedulerFlags flags;
		};

		using Scheduler = std::shared_ptr<VulkanScheduler>;

		struct Resource {
			struct Buffer {
				std::shared_ptr<VMAAllocator> mem_alloc;
				VkBufferCreateInfo buf_info;
				VkBuffer vk_handle;
				VmaAllocation alloc;
				VmaAllocationInfo alloc_info;
				~Buffer() noexcept {
					if (mem_alloc && vk_handle && alloc) {
						vmaDestroyBuffer(mem_alloc->impl, vk_handle, alloc);
					}
				}
			};
			struct Texture {
				std::shared_ptr<VMAAllocator> mem_alloc;
				VkImageCreateInfo buf_info;
				VkImage vk_handle;
				VmaAllocation alloc;
				VmaAllocationInfo alloc_info;
				vk::ImageLayout last_layout;
				vk::ImageLayout curr_layout;
				~Texture() noexcept {
					if (mem_alloc && vk_handle && alloc) {
						vmaDestroyImage(mem_alloc->impl, vk_handle, alloc);
					}
				}
			};
			std::variant<std::monostate, std::shared_ptr<Buffer>, std::shared_ptr<Texture>> impl;
		};

		struct View {
			struct Buffer {
				vk::BufferViewCreateInfo info;
				vk::SharedBufferView impl;
			};
			struct Texture {
				vk::ImageViewCreateInfo info;
				vk::SharedImageView impl;
			};
			std::variant<std::monostate, Buffer, Texture> impl;
		};

		using Sampler = vk::SharedSampler;

		struct Pipeline {
			std::vector<vk::SharedDescriptorSetLayout> descriptor_set_layouts;
			std::vector<PipelineBindingMetadata> bindings;
			vk::SharedPipelineLayout layout;
			vk::SharedRenderPass compatible_render_pass;
			vk::SharedPipeline state;
		};

		using PipelineResourceGroup = NativePipelineResourceGroup<Backend>;

		static Instance CreateInstance(
			std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver
#if defined(__ANDROID__)
			, android_app* android_app
#endif // defined(__ANDROID__)
		);

		static std::vector<PhysicalDevice> EnumeratePhysicalDevices(Instance const& instance);

#if defined(_WIN32)
		static vk::SharedSurfaceKHR CreateSurface(Instance const& instance, HWND window_handle);
#elif defined(__linux__)
		static vk::SharedSurfaceKHR CreateSurface(Instance const& instance, Display* x11_dpy, Window x11_window);
		static vk::SharedSurfaceKHR CreateSurface(Instance const& instance, wl_display* display, wl_surface* surface);
#elif defined(__ANDROID__)
		static vk::SharedSurfaceKHR CreateSurface(Instance const& instance, ANativeWindow* window);
#endif // defined(_WIN32)

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(PhysicalDevice const& phys_dev);

		static LogicalDevice CreateLogicalDevice(PhysicalDevice const& phys_dev);

		static Scheduler CreateScheduler(LogicalDevice& ld, SchedulerDescriptor const& descriptor);

		static Resource CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Resource CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static View CreateTextureView(LogicalDevice const& ld, Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

		static View CreateBufferView(LogicalDevice const& ld, Resource const& buf, std::size_t offset, std::size_t range, ResourceFlags const& flags);

		static vk::SharedSampler CreateSampler(LogicalDevice const& ld, SamplerDescriptor const& descriptor);

		static Pipeline CreateGraphicsPipeline(LogicalDevice const& ld, GraphicsPipelineDescriptor const& descriptor);

		static PipelineResourceGroup CreatePipelineResourceGroup(
			LogicalDevice const& ld,
			Pipeline const& pipeline,
			std::uint32_t space,
			std::span<NativePipelineResourceBinding<Backend> const> bindings
		);

	};
}
#endif // !defined(__APPLE__)
