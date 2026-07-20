/* the opengl pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"

#if defined(_WIN32)
#include "glad/glad_wgl.h"

#elif defined(__linux__)
#include "glad/glad_glx.h"
#include "glad/glad_egl.h"

#elif defined(__ANDROID__)
#include "glad/glad_egl.h"
#include <android/native_window.h>
#include <android/android_native_app_glue.h>

#endif // defined(_WIN32)
#endif // !defined(__APPLE__)
export module fyuu_rhi:;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi::opengl {

}
#endif // !defined(__APPLE__)

*/

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <variant>
#include <optional>
#include <vector>
#include <span>
#include <cstdint>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"

#if defined(_WIN32)
#include "glad/glad_wgl.h"

#elif defined(__linux__)
#include "glad/glad_glx.h"
#include "glad/glad_egl.h"

#elif defined(__ANDROID__)
#include "glad/glad_egl.h"
#include <android/native_window.h>
#include <android/android_native_app_glue.h>

#endif // defined(_WIN32)
#endif // !defined(__APPLE__)
export module fyuu_rhi:opengl_traits;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :resource_types;
import :sampler_types;
import :pipeline_types;
import :native_pipeline_binding;

namespace fyuu_rhi::opengl {
	
	export struct Backend {

		struct Instance {
#if defined(_WIN32)
			HDC dc;
			HGLRC rc;
#else
#if defined(__linux__)
			struct GLX {
				Display* dpy;
				GLXDrawable drawable;
				GLXContext ctx;
				GLXFBConfig fbconfig;   // added for shared context creation
			};
#endif // defined(__linux__)
			struct EGL {
				EGLDisplay display;
				EGLSurface draw;
				EGLSurface read;
				EGLContext context;
				EGLConfig config;       // added for shared context creation
				void* native_window;   	// wl_egl_window* for Wayland, ANativeWindow* for Android
			};
			std::variant<
				std::monostate
#if defined(__linux__)
				, GLX
#endif // defined(__linux__)
				, EGL
			> gl_handle;
#endif // defined(_WIN32)
		};

		using PhysicalDevice = Instance const*;
		using LogicalDevice = Instance const*;

		struct GLResource {
			GLuint impl;
			enum class Type : std::uint8_t {
				Buffer,
				Texture
			} type;
			~GLResource() noexcept {
				switch (type) {
				case fyuu_rhi::opengl::Backend::GLResource::Type::Buffer:
					glDeleteBuffers(1u, &impl);
					break;
				case fyuu_rhi::opengl::Backend::GLResource::Type::Texture:
					glDeleteTextures(1u, &impl);
					break;
				default:
					break;
				}
			}
		};

		using Resource = std::shared_ptr<GLResource>;

		struct GLTextureView {
			GLuint impl;
			~GLTextureView() noexcept {
				glDeleteTextures(1u, &impl);
			}
		};

		struct GLBufferView {
			struct Range {
				std::size_t offset;
				std::size_t size;
			};

			GLuint impl;
			std::optional<Range> range; // fallback for no GLAD_GL_ARB_texture_buffer_range
			~GLBufferView() noexcept {
				glDeleteTextures(1u, &impl);
			}
		};

		struct View {
			std::variant<std::monostate, std::shared_ptr<GLTextureView>, std::shared_ptr<GLBufferView>> impl;
		};

		struct GLSampler {
			GLuint impl;
			~GLSampler() noexcept {
				if (impl) {
					glDeleteSamplers(1u, &impl);
				}
			}
		};

		using Sampler = std::shared_ptr<GLSampler>;

		struct GLPipeline {
			GLuint impl;
			std::vector<VertexBufferLayout> vertex_buffers;
			std::vector<VertexAttribute> vertex_attributes;
			PrimitiveState primitive;
			RasterizationState rasterization;
			MultisampleState multisample;
			std::optional<DepthStencilState> depth_stencil;
			std::vector<ColorTargetState> color_targets;
			std::vector<PipelineBindingMetadata> bindings;

			~GLPipeline() noexcept {
				if (impl) {
					glDeleteProgram(impl);
				}
			}
		};

		using Pipeline = std::shared_ptr<GLPipeline>;

		using PipelineResourceGroup = NativePipelineResourceGroup<Backend>;

#if defined(_WIN32)
		static Instance CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver, HWND window_handle);
#elif defined(__linux__)
		static Instance CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver, Display* x11_dpy, Window x11_window);
		static Instance CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver, wl_display* display, wl_surface* surface, std::uint32_t width, std::uint32_t height);
#elif defined(__ANDROID__)
		static Instance CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver, android_app* app);
#endif // defined(_WIN32)

		static PhysicalDevice EnumeratePhysicalDevices(Instance const& instance) noexcept;

		static void ShareContextOnThisThread(Instance const& instance);

		static void DestroyInstance(Instance const& instance) noexcept;

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(PhysicalDevice const& phys_dev);

		static LogicalDevice CreateLogicalDevice(PhysicalDevice const& phys_dev) noexcept;

		static std::shared_ptr<GLResource> CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static std::shared_ptr<GLResource> CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static View CreateTextureView(LogicalDevice const& ld, std::shared_ptr<GLResource> const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

		static View CreateBufferView(LogicalDevice const& ld, std::shared_ptr<GLResource> const& res, std::size_t offset, std::size_t range, ResourceFlags const& flags);

		static Sampler CreateSampler(LogicalDevice const& ld, SamplerDescriptor const& descriptor);

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
