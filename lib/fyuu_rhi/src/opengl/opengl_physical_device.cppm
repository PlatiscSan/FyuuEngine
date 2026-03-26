/* opengl_physical_device.cppm */

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_physical_device;

#if !defined(__APPLE__)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :types;
import :opengl_common; 

namespace fyuu_rhi::opengl {

	/**
	 * @class OpenGLPhysicalDevice
	 * @brief Represents a physical OpenGL device (essentially the OpenGL context).
	 *
	 * This class encapsulates an OpenGL context and provides methods to query
	 * vendor/device information using glGetString. It also manages the debug callback.
	 */
	export class OpenGLPhysicalDevice
		: public PolymorphicPhysicalDeviceBase,
		public OpenGLCommon<OpenGLPhysicalDevice> {
	private:
		// Logging callback provided by the application.
		LogCallback m_log_callback;

		/**
		 * @brief Maps an OpenGL vendor string to a standard PCI vendor ID.
		 *
		 * OpenGL returns vendor names like "NVIDIA Corporation", "AMD", etc.
		 * This function converts common substrings to numeric IDs (e.g., 0x10DE for NVIDIA).
		 *
		 * @param vendor The vendor string from glGetString(GL_VENDOR).
		 * @return The vendor ID, or 0 if unknown.
		 */
		static std::uint32_t MapVendorStringToID(std::string_view vendor) noexcept;

		/**
		 * @brief OpenGL debug callback function (KHR_debug/ARB_debug_output).
		 *
		 * This function is called by the OpenGL implementation whenever a debug message
		 * is generated. It forwards the message to the application's LogCallback.
		 *
		 * @param source      The source of the message (API, window system, etc.).
		 * @param type        The type of message (error, deprecated, performance, etc.).
		 * @param id          The message ID.
		 * @param severity    Severity (high, medium, low, notification).
		 * @param length      Length of the message string.
		 * @param message     The message text.
		 * @param user_param  User pointer (set to the OpenGLPhysicalDevice instance).
		 */
		static void GLAPIENTRY DebugCallback(
			GLenum source, 
			GLenum type, 
			GLuint id, 
			GLenum severity,
			GLsizei length,
			GLchar const* message,
			void const* user_param
		);

	public:
		/**
		 * @brief Constructs an OpenGLPhysicalDevice.
		 *
		 * Initialization is platform-specific:
		 * - Windows: uses wglCreateContext on the given window handle.
		 * - Linux: uses GLX for X11, EGL for Wayland.
		 * - Android: uses EGL to create an OpenGL ES context.
		 *
		 * After context creation, Glad is used to load OpenGL function pointers.
		 * The debug callback is registered if the necessary extensions are available.
		 *
		 * @param init_options Initialization options, including window handle and log callback.
		 * @throws std::runtime_error or std::system_error if context creation or function loading fails.
		 */
		OpenGLPhysicalDevice(InitOptions const& init_options);

		~OpenGLPhysicalDevice() noexcept = default;

		/// @return Vendor ID (PCI vendor ID) mapped from the GL_VENDOR string.
		std::uint32_t GetVendorID() const noexcept;

		/// @return Device ID (always 0 for OpenGL, as GL_RENDERER is not a numeric ID).
		std::uint32_t GetID() const noexcept;

		/// @return Human-readable description from GL_RENDERER (e.g., "GeForce RTX 3080").
		std::string GetDescription() const noexcept;

	};

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)