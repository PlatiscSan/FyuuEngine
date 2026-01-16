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

module fyuu_rhi:vulkan_surface;
import :vulkan_physical_device;

namespace fyuu_rhi::vulkan {

#if defined(_WIN32)

	static vk::UniqueSurfaceKHR CreateSurface(
		vk::Instance const& instance,
		HWND window_handle
	) {
		vk::Win32SurfaceCreateInfoKHR create_info({}, GetModuleHandle(nullptr), window_handle, nullptr);
		return instance.createWin32SurfaceKHRUnique(create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);
	}

	VulkanSurface::VulkanSurface(
		vk::Instance const& instance,
		HWND window_handle
	) : m_window_handle(window_handle),
		m_impl(CreateSurface(instance, m_window_handle)) {

	}

	VulkanSurface::VulkanSurface(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		HWND window_handle
	) : VulkanSurface(physical_device->GetInstance(), window_handle) {

	}

	VulkanSurface::VulkanSurface(VulkanPhysicalDevice const& physical_device, HWND window_handle)
		: VulkanSurface(physical_device.GetInstance(), window_handle) {
	}

#elif defined(__linux__)

	static vk::UniqueSurfaceKHR CreateSurface(
		vk::Instance const& instance,
		wl_display* display,
		wl_surface* surface
	) {
		vk::WaylandSurfaceCreateInfoKHR create_info({}, display, surface);
		return instance.createWaylandSurfaceKHRUnique(create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);
	}

	static vk::UniqueSurfaceKHR CreateSurface(
		vk::Instance const& instance,
		Display* display,
		Window window
	) {
		vk::XlibSurfaceCreateInfoKHR create_info({}, display, window);
		return instance.createXlibSurfaceKHRUnique(create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);
	}

	VulkanSurface::VulkanSurface(
		vk::Instance const& instance,
		wl_display* display,
		wl_surface* surface
	) : m_handle(std::in_place_type<Wayland>, { display, surface }),
		m_impl(VulkanSurface::CreateSurfaceFromWayland(instance, display, surface)) {

	}

	VulkanSurface::VulkanSurface(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		wl_display* display,
		wl_surface* surface
	) : VulkanSurface(physical_device->GetInstance(), display, surface) {

	}

	VulkanSurface::VulkanSurface(
		VulkanPhysicalDevice const& physical_device, 
		wl_display* display,
		wl_surface* surface
	) : VulkanSurface(physical_device.GetInstance(), window_handle) {
	}

	VulkanSurface::VulkanSurface(
		vk::Instance const& instance,
		Display* display,
		Window window
	) : m_handle(std::in_place_type<Xlib>, { display, window }),
		m_impl(VulkanSurface::CreateSurfaceFromXlib(instance, display, window)) {

	}

	VulkanSurface::VulkanSurface(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		Display* display,
		Window window
	) : VulkanSurface(physical_device->GetInstance(), display, window) {

	}

	VulkanSurface::VulkanSurface(
		VulkanPhysicalDevice const& physical_device,
		Display* display,
		Window window
	) : VulkanSurface(physical_device.GetInstance(), display, window) {
	}

#elif defined(__ANDROID__)

	static vk::UniqueSurfaceKHR CreateSurface(
		vk::Instance const& instance,
		ANativeWindow* window_handle
	) {
		vk::AndroidSurfaceCreateInfoKHR create_info({}, window_handle);
		return instance.m_impl->createAndroidSurfaceKHRUnique(create_info, nullptr, vk::detail::defaultDispatchLoaderDynamic);
	}

	VulkanSurface::VulkanSurface(
		vk::Instance const& instance,
		ANativeWindow* window_handle
	) : m_window_handle(window_handle),
		m_impl(CreateSurface(instance, m_window_handle)) {

	}

	VulkanSurface::VulkanSurface(
		plastic::utility::AnyPointer<VulkanPhysicalDevice> const& physical_device,
		ANativeWindow* window_handle
	) : VulkanSurface(physical_device->GetInstance(), window_handle) {

	}

	VulkanSurface::VulkanSurface(
		VulkanPhysicalDevice const& physical_device,
		ANativeWindow* window_handle
	) : VulkanSurface(physical_device.GetInstance(), window_handle) {

	}

#endif // defined(_WIN32)

	std::pair<std::uint32_t, std::uint32_t> VulkanSurface::GetWidthAndHeightImpl() const {
#if defined(_WIN32)
		RECT client_rect{};
		GetClientRect(m_window_handle, &client_rect);

		return { client_rect.right - client_rect.left, client_rect.bottom - client_rect.top };
#elif defined(__linux__)

#elif defined(__ANDROID__)

#endif // defined(_WIN32)
	}

	vk::SurfaceKHR VulkanSurface::GetNative() const noexcept {
		return *m_impl;
	}

}