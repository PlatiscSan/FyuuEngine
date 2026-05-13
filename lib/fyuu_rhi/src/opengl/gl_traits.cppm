/* the opengl pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"
#if defined(_WIN32)
#include "glad/glad_wgl.h"
#else
#if defined(__linux__)
#include "glad/glad_glx.h"
#endif // defined(__linux__)
#include "glad/glad_egl.h"
#endif // defined(_WIN32)
#endif //!defined(__APPLE__)
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
#include <memory>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"
#if defined(_WIN32)
#include "glad/glad_wgl.h"
#else
#if defined(__linux__)
#include "glad/glad_glx.h"
#endif // defined(__linux__)
#include "glad/glad_egl.h"
#endif // defined(_WIN32)
#endif //!defined(__APPLE__)
export module fyuu_rhi:opengl_traits;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :resource_types;
import :command_types;

namespace fyuu_rhi::opengl {
	
	export struct Backend {

		struct Dummy {};

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

		using PhysicalDevice = Dummy;
		using Surface = Dummy;
		using LogicalDevice = Dummy;
		using Promise = std::shared_ptr<std::remove_pointer_t<GLsync>>;
		using Future = std::shared_ptr<std::remove_pointer_t<GLsync>>;
		using Resource = std::shared_ptr<GLuint>;

#if defined(_WIN32)
		static Instance CreateInstance(HWND window_handle);
#elif defined(__linux__)
		static Instance CreateInstance(Display* x11_dpy, Window x11_window);
		static Instance CreateInstance(wl_display* display, wl_surface* surface, std::uint32_t width, std::uint32_t height);
#elif defined(__ANDROID__)
		static Instance CreateInstance(ANativeWindow* window);
#endif // defined(_WIN32)

		template <class... Args>
		static Dummy CreateSurface(Args&&... args) noexcept {
			return {};
		}

		template <class... Args>
		static Dummy EnumeratePhysicalDevices(Args&&... args) noexcept {
			return {};
		}

		static void ShareContextOnThisThread(Instance const& instance);

		static void DestroyInstance(Instance const& instance) noexcept;

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(Dummy);

		template <class... Args>
		static Dummy CreateLogicalDevice(Args&&... args) noexcept {
			return {};
		}

		template <class... Args>
		static std::shared_ptr<std::remove_pointer_t<GLsync>> CreatePromise(Args&&... args) noexcept {
			return nullptr;
		}

		static std::shared_ptr<std::remove_pointer_t<GLsync>> GetFuture(std::shared_ptr<std::remove_pointer_t<GLsync>>& promise) noexcept;

		static std::shared_ptr<GLuint> CreateBuffer(Dummy, std::size_t size_in_bytes, ResourceFlags const& flags);

		static std::shared_ptr<GLuint> CreateTexture(Dummy, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

	};
}
#endif // !defined(__APPLE__)