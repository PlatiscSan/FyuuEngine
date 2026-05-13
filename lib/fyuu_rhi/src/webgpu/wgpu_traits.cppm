/* the webgpu pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#include <webgpu/webgpu_cpp.h>
export module fyuu_rhi:;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi::webgpu {

}

*/

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string>
#include <variant>
#include <format>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#endif // defined(_WIN32)
#include <webgpu/webgpu_cpp.h>
export module fyuu_rhi:webgpu_traits;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :command_types;
import :resource_types;

namespace fyuu_rhi::webgpu {
	struct Backend {
		using Instance = wgpu::Instance;
		using PhysicalDevice = wgpu::Adapter;
		using Surface = wgpu::Surface;
		using LogicalDevice = wgpu::Device;
		using Promise = wgpu::Future;
		using Future = wgpu::Future;

		struct Resource {
			std::variant<std::monostate, wgpu::Buffer, wgpu::Texture> impl;
		};

		static wgpu::Instance CreateInstance();

		static wgpu::Adapter EnumeratePhysicalDevices(wgpu::Instance const& instance);

#if defined(_WIN32)
		static wgpu::Surface CreateSurface(wgpu::Instance const& instance, HWND window_handle);
#elif defined(__linux__)
		static wgpu::Surface CreateSurface(wgpu::Instance const& instance, Display* x11_dpy, Window x11_window);
		static wgpu::Surface CreateSurface(wgpu::Instance const& instance, wl_display* display, wl_surface* surface);
#elif defined(__ANDROID__)
		static wgpu::Surface CreateSurface(wgpu::Instance const& instance, ANativeWindow* window);
#endif // defined(_WIN32)

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(wgpu::Adapter const& phys_dev);

		static wgpu::Device CreateLogicalDevice(wgpu::Adapter const& adapter);

		template <class... Args>
		static wgpu::Future CreatePromise(Args&&... args) noexcept {
			return {};
		}

		static wgpu::Future GetFuture(wgpu::Future& promise) noexcept;

		static Resource CreateBuffer(wgpu::Device const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Resource CreateTexture(wgpu::Device const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);


	};
}