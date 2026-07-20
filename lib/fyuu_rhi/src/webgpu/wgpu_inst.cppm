module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <vector>
#include <format>
#include <source_location>
#endif // !defined(__cpp_lib_modules)
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
#include <webgpu/webgpu_cpp.h>
module fyuu_rhi:webgpu_instance;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_traits;
import :log;
import :cache_system;

namespace fyuu_rhi::webgpu {

	wgpu::Instance Backend::CreateInstance(
		std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver
#if defined(__ANDROID__)
		, android_app* android_app
#endif // defined(__ANDROID__)
	) {
		cache::Initialize(
			app_name, app_ver, engine_name, engine_ver
#if defined(__ANDROID__)
			, android_app
#endif // defined(__ANDROID__)
		);
		wgpu::InstanceDescriptor desc = {};
		return wgpu::CreateInstance(&desc);
	}

	wgpu::Adapter Backend::EnumeratePhysicalDevices(wgpu::Instance const& instance) {

		std::vector<wgpu::Adapter> adapters;

		wgpu::RequestAdapterOptions options{
			nullptr,
			wgpu::FeatureLevel::Compatibility,
			wgpu::PowerPreference::HighPerformance,
			false,
			wgpu::BackendType::Undefined,
			nullptr
		};

		wgpu::Adapter adapter;
		auto SetAdapter = [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, char const* message) {
			if (status != wgpu::RequestAdapterStatus::Success) {
				log::Fatal(std::format("Calling RequestAdapter(), WebGPU reported: {}", message), std::source_location::current());
				return;
			}
			adapter = std::move(adapter_);
		};

		auto future = instance.RequestAdapter(
			&options,
			wgpu::CallbackMode::AllowProcessEvents,
			SetAdapter
		);

		wgpu::WaitStatus status = instance.WaitAny(future, (std::numeric_limits<std::uint64_t>::max)());

		if (status != wgpu::WaitStatus::Success) {
			throw std::runtime_error(std::format("Calling WaitAny(), but instance waiting failed"));
		}

		return adapter;

	}

#if defined(_WIN32)
	wgpu::Surface Backend::CreateSurface(wgpu::Instance const& instance, HWND window_handle) {
		wgpu::SurfaceSourceWindowsHWND win32_desc = {};
		win32_desc.hinstance = GetModuleHandle(nullptr);
		win32_desc.hwnd = window_handle;
		wgpu::SurfaceDescriptor surface_desc = {};
		surface_desc.nextInChain = &win32_desc;
		return instance.CreateSurface(&surface_desc);
	}
#elif defined(__linux__)
	wgpu::Surface Backend::CreateSurface(wgpu::Instance const& instance, Display* x11_dpy, Window x11_window) {
		wgpu::SurfaceDescriptorFromXlibWindow xlib_desc = {};
		xlib_desc.display = x11_dpy;
		xlib_desc.window = x11_window;

		wgpu::SurfaceDescriptor surface_desc = {};
		surface_desc.nextInChain = &xlib_desc;

		return instance.CreateSurface(surface_desc);
	}
	wgpu::Surface Backend::CreateSurface(wgpu::Instance const& instance, wl_display* display, wl_surface* surface) {
		wgpu::SurfaceDescriptorFromWaylandSurface wayland_desc = {};
		wayland_desc.display = display;
		wayland_desc.surface = surface;

		wgpu::SurfaceDescriptor surface_desc = {};
		surface_desc.nextInChain = &wayland_desc;

		return instance.CreateSurface(surface_desc);
	}
#elif defined(__ANDROID__)
	wgpu::Surface Backend::CreateSurface(wgpu::Instance const& instance, ANativeWindow* window) {
		wgpu::SurfaceDescriptorFromAndroidNativeWindow android_desc = {};
		android_desc.window = window;

		wgpu::SurfaceDescriptor surface_desc = {};
		surface_desc.nextInChain = &android_desc;

		return instance.CreateSurface(surface_desc);
	}
#endif // defined(_WIN32)

}
