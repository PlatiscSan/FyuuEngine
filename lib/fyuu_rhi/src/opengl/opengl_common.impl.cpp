/* opengl_common.impl.cppm */
/**
 * @file opengl_common.impl.cppm
 * @brief Implementation of the OpenGL common module.
 *
 * Contains the definitions of OpenGLStateMachine::PlatformGLContext
 * constructors/destructor and OpenGLStateMachine member functions.
 */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <system_error>
#include <stdexcept>
#include <utility>
#include <numeric>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <variant>
#include <concepts>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
module fyuu_rhi:opengl_common_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_common;
import :types;
import :common;
import plastic.other;

namespace fyuu_rhi::opengl {
	
	std::size_t OpenGLStateMachine::KeyHash::operator()(FramebufferKey const& key) const {
		std::size_t h = 0;
		for (GLuint id : key.color_tex_ids)
			h ^= std::hash<GLuint>{}(id) + 0x9e3779b9 + (h << 6) + (h >> 2);
		h ^= std::hash<GLuint>{}(key.depth_tex_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}

#if defined(_WIN32)
	/**
	 * @brief Constructs a Windows OpenGL context.
	 * @param handle Platform handle containing the HWND.
	 * @throws std::invalid_argument If the handle does not contain a valid HWND.
	 * @throws std::system_error If GetDC, ChoosePixelFormat, SetPixelFormat, or wglCreateContext fails.
	 */
	OpenGLStateMachine::PlatformGLContext::PlatformGLContext(PlatformHandle const& handle)
		: device_context(
			[&handle]() -> HDC {
				HWND hwnd = handle.impl;
				if (!hwnd) {
					throw std::invalid_argument("OpenGL on Windows requires a valid window handle");
				}
				HDC device_context = GetDC(hwnd);
				if (!device_context) {
					DWORD ec = GetLastError();
					throw std::system_error(ec, std::system_category(), "Failed to get device context");
				}
				return device_context;
			}()),
		rendering_context(
			[this]() -> HGLRC {
				PIXELFORMATDESCRIPTOR pfd = {
					sizeof(PIXELFORMATDESCRIPTOR), 1,
					PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
					PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 24, 8, 0,
					PFD_MAIN_PLANE, 0, 0, 0, 0
				};
				int pixel_format = ChoosePixelFormat(device_context, &pfd);
				if (!pixel_format) {
					DWORD ec = GetLastError();
					throw std::system_error(ec, std::system_category(), "Failed to choose pixel format");
				}

				if (!SetPixelFormat(device_context, pixel_format, &pfd)) {
					DWORD ec = GetLastError();
					throw std::system_error(ec, std::system_category(), "Failed to set pixel format");
				}

				HGLRC rendering_context = wglCreateContext(device_context);
				if (!rendering_context) {
					DWORD ec = GetLastError();
					throw std::system_error(ec, std::system_category(), "Failed to create rendering context");
				}

				return rendering_context;

			}()) {

	}
#elif defined(__linux__)
	/**
	 * @brief Constructs a Linux OpenGL context (either GLX for X11 or EGL for Wayland).
	 * @param handle Platform handle containing either Xlib or Wayland data.
	 * @throws std::runtime_error If the platform is unsupported or any step fails.
	 */
	OpenGLStateMachine::PlatformGLContext::PlatformGLContext(PlatformHandle const& handle)
		: handle(
			[&handle]() -> std::variant<std::monostate, GLX, EGL> {
				return std::visit(
					[](auto&& arg) -> std::variant<std::monostate, GLX, EGL> {
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, Wayland>) {
							Wayland wl = arg;
							EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)wl.display);
							if (display == EGL_NO_DISPLAY)
								throw std::runtime_error("eglGetDisplay failed");
							EGLint major, minor;
							if (!eglInitialize(display, &major, &minor))
								throw std::runtime_error("eglInitialize failed");
							EGLConfig config;
							EGLint config_attribs[] = {
								EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
								EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
								EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
								EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
								EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
								EGL_NONE
							};
							EGLint num_configs;
							if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs) || num_configs == 0) {
								eglTerminate(display);
								throw std::runtime_error("eglChooseConfig failed");
							}
							EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)wl.surface, nullptr);
							if (surface == EGL_NO_SURFACE) {
								eglTerminate(display);
								throw std::runtime_error("eglCreateWindowSurface failed");
							}
							EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, nullptr);
							if (context == EGL_NO_CONTEXT) {
								eglDestroySurface(display, surface);
								eglTerminate(display);
								throw std::runtime_error("eglCreateContext failed");
							}
							return EGL{ display, surface, surface, context };
						}
						else if constexpr (std::is_same_v<T, Xlib>) {
							Xlib xlib = arg;
							int visual_attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
							XVisualInfo* vi = glXChooseVisual(xlib.display, DefaultScreen(xlib.display), visual_attribs);
							if (!vi)
								throw std::runtime_error("glXChooseVisual failed");
							GLXContext ctx = glXCreateContext(xlib.display, vi, nullptr, GL_TRUE);
							XFree(vi);
							if (!ctx)
								throw std::runtime_error("glXCreateContext failed");
							return GLX{ xlib.display, xlib.window, ctx };
						}
						else {
							throw std::runtime_error("Unsupported Linux platform handle");
						}
					},
					handle.impl
				);
			}()) {

	}
#elif defined(__ANDROID__)
	/**
	 * @brief Constructs an Android EGL context.
	 * @param handle Platform handle containing an ANativeWindow*.
	 * @throws std::runtime_error If any EGL call fails.
	 */
	OpenGLStateMachine::PlatformGLContext::PlatformGLContext(PlatformHandle const& handle)
		: display(
			[handle]() -> EGLDisplay {
				EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
				if (dpy == EGL_NO_DISPLAY)
					throw std::runtime_error("eglGetDisplay failed");
				EGLint major, minor;
				if (!eglInitialize(dpy, &major, &minor))
					throw std::runtime_error("eglInitialize failed");
				return dpy;
			}()),
		draw(
			[this, &handle]() -> EGLSurface {
				EGLConfig config;
				EGLint config_attribs[] = {
					EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
					EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
					EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
					EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
					EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
					EGL_NONE
				};
				EGLint num_configs;
				if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs) || num_configs == 0) {
					eglTerminate(display);
					throw std::runtime_error("eglChooseConfig failed");
				}
				ANativeWindow* window = handle.impl;
				EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
				if (surface == EGL_NO_SURFACE) {
					eglTerminate(display);
					throw std::runtime_error("eglCreateWindowSurface failed");
				}
				return surface;
			}()),
		read(draw),
		context(
			[this]() -> EGLContext {
				EGLConfig config;
				EGLint config_attribs[] = {
					EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
					EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
					EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
					EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
					EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
					EGL_NONE
				};
				EGLint num_configs;
				if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs) || num_configs == 0) {
					eglDestroySurface(display, draw);
					eglTerminate(display);
					throw std::runtime_error("eglChooseConfig failed");
				}
				EGLContext ctx = eglCreateContext(display, config, EGL_NO_CONTEXT, nullptr);
				if (ctx == EGL_NO_CONTEXT) {
					eglDestroySurface(display, draw);
					eglTerminate(display);
					throw std::runtime_error("eglCreateContext failed");
				}
				return ctx;
			}()) {

	}
#endif // defined(_WIN32)

	/**
	 * @brief Destroys the OpenGL context and releases all associated platform resources.
	 *
	 * Calls the appropriate platform‑specific destruction functions.
	 */
	OpenGLStateMachine::PlatformGLContext::~PlatformGLContext() noexcept {
#if defined(_WIN32)
		if (rendering_context) {
			wglDeleteContext(rendering_context);
		}
#elif defined(__linux__)
		std::visit(
			[](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, GLX>) {
					glXDestroyContext(arg.dpy, arg.ctx);
				}
				else if constexpr (std::is_same_v<T, EGL>) {
					eglDestroyContext(arg.display, arg.context);
					eglDestroySurface(arg.display, arg.draw);
					eglTerminate(arg.display);
				}
			},
			handle
		);
#elif defined(__ANDROID__)
		if (context != EGL_NO_CONTEXT) {
			eglDestroyContext(display, context);
		}
		if (draw != EGL_NO_SURFACE) {
			eglDestroySurface(display, draw);
		}
		if (display != EGL_NO_DISPLAY) {
			eglTerminate(display);
		}
#endif
	}

	/**
	 * @brief Makes the OpenGL context current on the calling thread.
	 *
	 * If the context is already current on this thread (i.e., s_stl_context equals m_context),
	 * the function returns immediately. Otherwise, it calls the appropriate platform function
	 * to bind the context and updates the thread‑local current context pointer.
	 *
	 * @throws std::runtime_error If m_context is null (context lost).
	 * @throws std::system_error On Windows if wglMakeCurrent fails.
	 */
	void OpenGLStateMachine::MakeCurrent() const {

		if (!m_context) {
			throw std::runtime_error("context lost");
		}

		if (s_stl_context == m_context) {
			return;
		}

#if defined(_WIN32)
		if (!wglMakeCurrent(m_context->device_context, m_context->rendering_context)) {
			DWORD ec = GetLastError();
			throw std::system_error(ec, std::system_category(), "Failed to make current");
		}
#elif defined(__linux__)
		std::visit(
			[this](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, GLX>) {
					glXMakeCurrent(arg.dpy, arg.drawable, arg.ctx);
				}
				else if constexpr (std::is_same_v<T, EGL>) {
					eglMakeCurrent(arg.display, arg.draw, arg.read, arg.context);
				}
				else {

				}
			},
			m_context->handle
		);
#elif defined(__ANDROID__)
		eglMakeCurrent(m_context->display, m_context->draw, m_context->read, m_context->context);
#endif
		s_stl_context = m_context;
	}

	/**
	 * @brief Loads OpenGL function pointers using glad.
	 *
	 * Makes the context current and calls gladLoadGL(). If loading fails,
	 * the context is released and an exception is thrown.
	 *
	 * @throws std::runtime_error If gladLoadGL() returns false.
	 */
	void OpenGLStateMachine::InitializeGL() {
		plastic::utility::InitializeGlobalInstance(
			[this]() {
				MakeCurrent();
				if (!gladLoadGL()) {
					wglMakeCurrent(nullptr, nullptr);
					throw std::runtime_error("Failed to load GL functions");
				}
			}
		);
	}

	/**
	 * @brief Constructs an OpenGLStateMachine, creating a new context from the given platform handle.
	 * @param platform_handle Abstract handle describing the target window/surface.
	 */
	OpenGLStateMachine::OpenGLStateMachine(PlatformHandle const& platform_handle)
		: m_context(std::make_shared<PlatformGLContext>(platform_handle)) {
		InitializeGL();
	}

	/**
	 * @brief Returns the shared pointer to the platform context.
	 * @return Shared pointer to the context handle.
	 */
	std::shared_ptr<OpenGLStateMachine::PlatformGLContext> OpenGLStateMachine::GetContext() const noexcept {
		return m_context;
	}

	GLuint OpenGLStateMachine::GetFramebuffer(std::span<GLuint const> color_tex_ids, GLuint depth_tex_id) {
		FramebufferKey key{ std::vector<GLuint>(color_tex_ids.begin(), color_tex_ids.end()), depth_tex_id };
		auto& fbo_cache = m_context->fbo_cache;
		auto it = fbo_cache.find(key);
		if (it != fbo_cache.end())
			return it->second;
	
		GLuint fbo;
		glCreateFramebuffers(1, &fbo);
	
		for (size_t i = 0; i < color_tex_ids.size(); ++i) {
			glNamedFramebufferTexture(
				fbo, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
				color_tex_ids[i], 0
			);
		}

		if (depth_tex_id) {
			glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth_tex_id, 0);
		}
	
		std::vector<GLenum> draw_buffers(color_tex_ids.size());
		std::iota(draw_buffers.begin(), draw_buffers.end(), GL_COLOR_ATTACHMENT0);
		glNamedFramebufferDrawBuffers(fbo, static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data());

		GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteFramebuffers(1, &fbo);
			throw std::runtime_error("Framebuffer incomplete");
		}
	
		fbo_cache[key] = fbo;
		return fbo;
	}

	void OpenGLStateMachine::InvalidateTexture(GLuint tex_id) {
		auto& fbo_cache = m_context->fbo_cache;
		auto it = fbo_cache.begin();
		while (it != fbo_cache.end()) {
			auto const& key = it->first;
			bool contains = key.depth_tex_id == tex_id || 
				std::find(key.color_tex_ids.begin(), key.color_tex_ids.end(), tex_id) != key.color_tex_ids.end();
			if (contains) {
				glDeleteFramebuffers(1, &it->second);
				it = fbo_cache.erase(it);
			} else {
				++it;
			}
		}
	}

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)