/* opengl_common.cppm */
/**
 * @file opengl_common.cppm
 * @brief OpenGL RHI common module interface.
 *
 * Provides the OpenGLStateMachine base class for managing an OpenGL context
 * and the OpenGLCommon template mixin for RHI implementations.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <variant>
#include <span>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_common;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :common;

namespace fyuu_rhi::opengl {

	/**
	 * @brief Manages an OpenGL context and its platform‑specific handles.
	 *
	 * OpenGLStateMachine encapsulates the lifetime of a native OpenGL context
	 * and provides RAII cleanup. It also tracks the currently active context
	 * per thread to avoid redundant MakeCurrent calls.
	 */
	export class OpenGLStateMachine {
	private:
		struct FramebufferKey {
			std::vector<GLuint> color_tex_ids;
			GLuint depth_tex_id;
		
			std::strong_ordering operator<=>(FramebufferKey const& other) const noexcept = default;
			
		};

		struct KeyHash {
			std::size_t operator()(FramebufferKey const& key) const;
		};

	protected:
		/**
		 * @brief Platform‑specific OpenGL context handles.
		 *
		 * This structure holds the native handles needed for an OpenGL context
		 * on the current platform. It is movable but not copyable.
		 */
		struct PlatformGLContext {
#if defined(_WIN32)
			HDC device_context;          ///< Windows GDI device context.
			HGLRC rendering_context;      ///< Windows OpenGL rendering context.
#elif defined(__linux__)
			/**
			 * @brief GLX context data (X11).
			 */
			struct GLX {
				Display* dpy;            ///< X11 display connection.
				GLXDrawable drawable;     ///< GLX drawable (usually a window).
				GLXContext ctx;           ///< GLX rendering context.
			};
			/**
			 * @brief EGL context data (Wayland or fallback).
			 */
			struct EGL {
				EGLDisplay display;       ///< EGL display connection.
				EGLSurface draw;          ///< EGL draw surface.
				EGLSurface read;           ///< EGL read surface (same as draw for simplicity).
				EGLContext context;        ///< EGL rendering context.
			};

			std::variant<std::monostate, GLX, EGL> handle; ///< Active context variant.
#elif defined(__ANDROID__)
			EGLDisplay display;           ///< EGL display connection.
			EGLSurface draw;              ///< EGL draw surface.
			EGLSurface read;               ///< EGL read surface.
			EGLContext context;            ///< EGL rendering context.
#endif // defined(_WIN32)
			std::unordered_map<FramebufferKey, GLuint, KeyHash> fbo_cache;

			/**
			 * @brief Constructs a platform context from an abstract platform handle.
			 * @param handle Platform handle containing window/system information.
			 * @throws std::runtime_error If context creation fails.
			 * @throws std::system_error On Windows if GDI calls fail.
			 */
			PlatformGLContext(PlatformHandle const& handle);

			PlatformGLContext(PlatformGLContext const&) = delete;
			PlatformGLContext(PlatformGLContext&& other) noexcept = default;

			/**
			 * @brief Destroys the OpenGL context and releases all platform resources.
			 */
			~PlatformGLContext() noexcept;
		};

		/**
		 * @brief Makes this context current on the calling thread.
		 * @throws std::runtime_error If the context is lost.
		 * @throws std::system_error On Windows if wglMakeCurrent fails.
		 */
		void MakeCurrent() const;

	private:
		static thread_local inline std::shared_ptr<PlatformGLContext> s_stl_context; ///< Currently active context per thread.
		std::shared_ptr<PlatformGLContext> m_context; ///< Shared context handle.

		/**
		 * @brief Initializes OpenGL function pointers using glad.
		 * @throws std::runtime_error If gladLoadGL() fails.
		 */
		void InitializeGL();

	public:
		/**
		 * @brief Constructs an OpenGLStateMachine from a platform handle.
		 * @param platform_handle Abstract handle describing the target window/surface.
		 */
		OpenGLStateMachine(PlatformHandle const& platform_handle);

		/**
		 * @brief Copy‑constructs from a derived object (shares the context).
		 * @tparam Derived Any class derived from OpenGLStateMachine.
		 * @param derived Reference to the derived object.
		 */
		template <std::derived_from<OpenGLStateMachine> Derived>
		OpenGLStateMachine(Derived const& derived) noexcept
			: m_context(derived.m_context) {

		}

		/**
		 * @brief Copy‑constructs from a derived object pointer (shares the context).
		 * @tparam Derived Any class derived from OpenGLStateMachine.
		 * @param derived Pointer to the derived object.
		 */
		template <std::derived_from<OpenGLStateMachine> Derived>
		OpenGLStateMachine(Derived const* derived) noexcept
			: m_context(derived->m_context) {

		}

		OpenGLStateMachine(OpenGLStateMachine&& other) noexcept = default;

		/**
		 * @brief Returns the shared context handle.
		 * @return Shared pointer to the platform context.
		 */
		std::shared_ptr<PlatformGLContext> GetContext() const noexcept;

		GLuint GetFramebuffer(std::span<GLuint const> color_tex_ids, GLuint depth_tex_id);

		void InvalidateTexture(GLuint tex_id);

	};

	/**
	 * @brief CRTP mixin for OpenGL RHI implementations.
	 *
	 * OpenGLCommon combines RHICommon (for derived‑to‑base casts) with
	 * OpenGLStateMachine. It provides the API identification and several
	 * convenient constructors that forward the context from various sources.
	 *
	 * @tparam Derived The actual RHI class (e.g., OpenGLDevice, OpenGLCommandList).
	 */
	export template <class Derived> class OpenGLCommon
		: public ::fyuu_rhi::RHICommon<Derived>,
		public OpenGLStateMachine {
	private:
		using Base = ::fyuu_rhi::RHICommon<Derived>;

	public:
		/**
		 * @brief Constructs OpenGLCommon from a derived pointer, assuming the context
		 *        is already managed elsewhere (e.g., the derived object owns it).
		 * @param derived Pointer to the derived object.
		 */
		OpenGLCommon(Derived* derived) noexcept
			: Base(derived),
			OpenGLStateMachine(derived) {

		}

		/**
		 * @brief Constructs OpenGLCommon from a platform handle, creating a new context.
		 * @param derived Pointer to the derived object.
		 * @param platform_handle Platform handle describing the target window/surface.
		 */
		OpenGLCommon(Derived* derived, PlatformHandle const& platform_handle)
			: Base(derived),
			OpenGLStateMachine(platform_handle) {

		}

		/**
		 * @brief Constructs OpenGLCommon from another context source (by reference).
		 * @tparam ContextSource A type derived from OpenGLStateMachine.
		 * @param self Pointer to the derived object.
		 * @param context_source Reference to an object that provides an OpenGL context.
		 */
		template <std::derived_from<OpenGLStateMachine> ContextSource>
		OpenGLCommon(Derived* self, ContextSource const& context_source)
			: Base(self),
			OpenGLStateMachine(context_source) {

		}

		/**
		 * @brief Constructs OpenGLCommon from another context source (by pointer).
		 * @tparam ContextSource A type derived from OpenGLStateMachine.
		 * @param self Pointer to the derived object.
		 * @param context_source Pointer to an object that provides an OpenGL context.
		 */
		template <std::derived_from<OpenGLStateMachine> ContextSource>
		OpenGLCommon(Derived* self, ContextSource const* context_source)
			: Base(self),
			OpenGLStateMachine(context_source) {

		}

		OpenGLCommon(OpenGLCommon&& other) noexcept = default;

		/**
		 * @brief Returns the API type identifier.
		 * @return API::OpenGL
		 */
		static constexpr API GetAPI() noexcept {
			return API::OpenGL;
		}

		/**
		 * @brief Returns the API name as a string.
		 * @return "OpenGL"
		 */
		static constexpr std::string_view GetAPIString() noexcept {
			return "OpenGL";
		}

	};

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)