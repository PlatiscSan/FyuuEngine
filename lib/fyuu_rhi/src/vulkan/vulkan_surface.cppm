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

export module fyuu_rhi:vulkan_surface;
import vulkan_hpp;
import :swap_chain;
import :vulkan_declaration;
import plastic.any_pointer;

namespace fyuu_rhi::vulkan {

	export class VulkanSurface 
		: public plastic::utility::EnableSharedFromThis<VulkanSurface>,
		public ISurface {
		friend class ISurface;

	private:
#if defined(_WIN32)
		HWND m_window_handle;
#elif defined(__linux__)
		struct Wayland {
			wl_display* display;
			wl_surface* surface;
		};

		struct Xlib {
			Display* display;
			Window window;
		};

		std::variant<std::monostate, Wayland, Xlib> m_handle;
#elif defined(__ANDROID__)
		ANativeWindow* m_window_handle;
#endif // defined(_WIN32)
		vk::UniqueSurfaceKHR m_impl;

	public:
#if defined(_WIN32)

		VulkanSurface(
			vk::Instance const& instance, 
			HWND window_handle
		);

		VulkanSurface(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			HWND window_handle
		);

		VulkanSurface(
			VulkanPhysicalDevice const& physical_device,
			HWND window_handle
		);

#elif defined(__linux__)

		VulkanSurface(
			vk::Instance const& instance,
			wl_display* display,
			wl_surface* surface
		);

		VulkanSurface(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			wl_display* display,
			wl_surface* surface
		);

		VulkanSurface(
			VulkanPhysicalDevice const& physical_device,
			wl_display* display,
			wl_surface* surface
		);

		VulkanSurface(
			vk::Instance const& instance,
			Display* display,
			Window window
		);

		VulkanSurface(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			Display* display,
			Window window
		);

		VulkanSurface(
			VulkanPhysicalDevice const& physical_device,
			Display* display,
			Window window
		);

#elif defined(__ANDROID__)

		VulkanSurface(
			vk::Instance const& instance, 
			ANativeWindow* window_handle
		);

		VulkanSurface(
			plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
			ANativeWindow* window_handle
		);

		VulkanSurface(
			VulkanPhysicalDevice const& physical_device,
			ANativeWindow* window_handle
		);

#endif // defined(_WIN32)

		std::pair<std::uint32_t, std::uint32_t> GetWidthAndHeightImpl() const;

		vk::SurfaceKHR GetNative() const noexcept;

	};

}