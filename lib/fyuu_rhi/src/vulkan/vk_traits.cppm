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
import :command_types;
import :resource_types;

namespace fyuu_rhi::vulkan {

	export struct Version {
		std::uint8_t variant;
		std::uint8_t major;
		std::uint8_t minor;
		std::uint8_t patch;
	};

	export struct Backend {

		struct Instance {
			vk::DispatchLoaderDynamic const& dispatcher;
			std::vector<std::string> enabled_extensions;
			vk::SharedInstance impl;
			vk::SharedDebugUtilsMessengerEXT debug_messenger;
		};
		
		struct PhysicalDevice {
			Instance const& instance;
			vk::SharedPhysicalDevice impl;
		};

		using Surface = vk::SharedSurfaceKHR;

		struct LogicalDevice {
			PhysicalDevice phys_dev;
			QueueAllocator queue_alloc;
			std::vector<std::string> enabled_extensions;
			vk::SharedDevice impl;
			std::shared_ptr<vk::DispatchLoaderDynamic> dispatcher;
			std::shared_ptr<std::remove_pointer_t<VmaAllocator>> mem_alloc;
		};

		using UniqueBinarySemaphore = boost::scope::unique_resource<vk::SharedSemaphore, void(*)(vk::SharedSemaphore&)>;
		using UniqueFence = boost::scope::unique_resource<vk::SharedFence, void(*)(vk::SharedFence&)>;

		struct BinarySynchronization {
			UniqueBinarySemaphore sem;
			UniqueFence fence;
		};

		struct Future {
			struct TimelineSemaphore {
				vk::SharedSemaphore impl;
				std::uint64_t val;
			};
			std::variant<std::monostate, std::shared_ptr<BinarySynchronization>, TimelineSemaphore> impl;
			std::shared_ptr<vk::DispatchLoaderDynamic> dispatcher;
		};

		struct Promise {
			struct TimelineSemaphore {
				vk::SharedSemaphore impl;
				TimelineCounter counter;
			};
			std::variant<std::monostate, std::shared_ptr<BinarySynchronization>, TimelineSemaphore> impl;
			std::shared_ptr<vk::DispatchLoaderDynamic> dispatcher;
			Future last_fut;
		};

		struct Resource {
			struct Buffer {
				VkBufferCreateInfo buf_info;
				VkBuffer vk_handle;
				VmaAllocation alloc;
				VmaAllocationInfo alloc_info;
			};
			struct Texture {
				VkImageCreateInfo buf_info;
				VkImage vk_handle;
				VmaAllocation alloc;
				VmaAllocationInfo alloc_info;
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

		static Instance CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver);

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

		static Promise CreatePromise(LogicalDevice const& ld);

		static Future GetFuture(Promise& promise) noexcept;
		
		static Resource CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Resource CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static View CreateTextureView(LogicalDevice const& ld, Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

	};
}
#endif // !defined(__APPLE__)