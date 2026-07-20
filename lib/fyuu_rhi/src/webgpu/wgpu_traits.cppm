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
#include <vector>
#include <variant>
#include <format>
#include <span>
#include <cstdint>
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
export module fyuu_rhi:webgpu_traits;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :resource_types;
import :sampler_types;
import :pipeline_types;
import :native_pipeline_binding;

namespace fyuu_rhi::webgpu {
	struct Backend {
		using Instance = wgpu::Instance;
		using PhysicalDevice = wgpu::Adapter;
		using Surface = wgpu::Surface;
		using LogicalDevice = wgpu::Device;

		struct Resource {
			std::variant<std::monostate, wgpu::Buffer, wgpu::Texture> impl;
		};

		struct View {
			struct BufferView {
				wgpu::Buffer buf;
				std::size_t offset;
				std::size_t range;
			};
			std::variant<std::monostate, wgpu::TextureView, BufferView> impl;
		};

		using Sampler = wgpu::Sampler;

		struct Pipeline {
			std::vector<wgpu::BindGroupLayout> bind_group_layouts;
			std::vector<PipelineBindingMetadata> bindings;
			wgpu::RenderPipeline state;
		};

		using PipelineResourceGroup = NativePipelineResourceGroup<Backend>;

		static wgpu::Instance CreateInstance(
			std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver
#if defined(__ANDROID__)
			, android_app* android_app
#endif // defined(__ANDROID__)
		);

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

		static Resource CreateBuffer(wgpu::Device const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Resource CreateTexture(wgpu::Device const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static View CreateTextureView(wgpu::Device const& ld, Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

		static View CreateBufferView(wgpu::Device const& ld, Resource const& buf, std::size_t offset, std::size_t range, ResourceFlags const& flags);

		static wgpu::Sampler CreateSampler(wgpu::Device const& ld, SamplerDescriptor const& desc);

		static Pipeline CreateGraphicsPipeline(wgpu::Device const& ld, GraphicsPipelineDescriptor const& descriptor);

		static PipelineResourceGroup CreatePipelineResourceGroup(
			wgpu::Device const& ld,
			Pipeline const& pipeline,
			std::uint32_t space,
			std::span<NativePipelineResourceBinding<Backend> const> bindings
		);

	};
}
