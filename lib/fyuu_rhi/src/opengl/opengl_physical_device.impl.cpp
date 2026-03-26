/* opengl_physical_device.impl.cpp - Module implementation for OpenGL physical device */

// This file provides the implementation of OpenGLPhysicalDevice.
module;

#include <version>


#if !defined(__cpp_lib_modules)
#include <stdexcept> 
#include <string_view>
#include <sstream>
#endif // !defined(__cpp_lib_modules)

#include "platform.hpp"
#include "glad.hpp"

module fyuu_rhi:opengl_physical_device_impl;


#if !defined(__APPLE__)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :opengl_physical_device; 
import :types;
import :opengl_common; 
import plastic.other;

namespace fyuu_rhi::opengl {

	// ----------------------------------------------------------------------------
	// MapVendorStringToID: converts vendor name to PCI vendor ID.
	// ----------------------------------------------------------------------------
	std::uint32_t OpenGLPhysicalDevice::MapVendorStringToID(std::string_view vendor) noexcept {
		// NVIDIA
		if (vendor.find("NVIDIA") != std::string_view::npos)
			return 0x10DE;
		// AMD (or ATI legacy)
		if (vendor.find("AMD") != std::string_view::npos || vendor.find("ATI") != std::string_view::npos)
			return 0x1002;
		// Intel
		if (vendor.find("Intel") != std::string_view::npos)
			return 0x8086;
		// ARM
		if (vendor.find("ARM") != std::string_view::npos)
			return 0x13B5;
		// Qualcomm
		if (vendor.find("Qualcomm") != std::string_view::npos)
			return 0x5143;
		return 0;
	}

	// ----------------------------------------------------------------------------
	// DebugCallback: OpenGL debug message handler.
	// ----------------------------------------------------------------------------
	void GLAPIENTRY OpenGLPhysicalDevice::DebugCallback(
		GLenum source, GLenum type, GLuint id, GLenum severity,
		GLsizei length, GLchar const* message, void const* user_param
	) {
		// Retrieve the OpenGLPhysicalDevice instance (passed as user_param).
		auto* self = static_cast<OpenGLPhysicalDevice const*>(user_param);
		auto [Func, user_data] = self->m_log_callback;

		// If no callback function is set, do nothing.
		if (!Func) return;

		// Map OpenGL severity to RHI log severity.
		LogSeverity logSeverity;
		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:          logSeverity = LogSeverity::Error; break;
		case GL_DEBUG_SEVERITY_MEDIUM:        logSeverity = LogSeverity::Warning; break;
		default:                               logSeverity = LogSeverity::Info; break;
		}

		// Build a detailed message string (similar to Vulkan debug output).
		std::ostringstream msg;
		msg << "OpenGL ";

		// Source
		switch (source) {
		case GL_DEBUG_SOURCE_API:             msg << "[API] "; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   msg << "[Window System] "; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: msg << "[Shader Compiler] "; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     msg << "[Third Party] "; break;
		case GL_DEBUG_SOURCE_APPLICATION:     msg << "[Application] "; break;
		case GL_DEBUG_SOURCE_OTHER:           msg << "[Other] "; break;
		default:                               msg << "[Unknown Source] "; break;
		}

		// Type
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:               msg << "[Error] "; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msg << "[Deprecated] "; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  msg << "[Undefined] "; break;
		case GL_DEBUG_TYPE_PORTABILITY:         msg << "[Portability] "; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         msg << "[Performance] "; break;
		case GL_DEBUG_TYPE_MARKER:              msg << "[Marker] "; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          msg << "[Push Group] "; break;
		case GL_DEBUG_TYPE_POP_GROUP:           msg << "[Pop Group] "; break;
		case GL_DEBUG_TYPE_OTHER:               msg << "[Other] "; break;
		default:                                 msg << "[Unknown Type] "; break;
		}

		// Severity as string
		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:          msg << "[High] "; break;
		case GL_DEBUG_SEVERITY_MEDIUM:        msg << "[Medium] "; break;
		case GL_DEBUG_SEVERITY_LOW:            msg << "[Low] "; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:  msg << "[Notification] "; break;
		default:                               msg << "[Unknown Severity] "; break;
		}

		// Message ID
		msg << "ID:" << id << " ";

		// Actual message text
		if (message && length > 0) {
			msg << std::string_view(message, length);
		}

		// Forward to the application's logging function.
		Func(logSeverity, msg.str(), user_data);
	}

	OpenGLPhysicalDevice::OpenGLPhysicalDevice(InitOptions const& init_options)
		: PolymorphicPhysicalDeviceBase(this),
		OpenGLCommon(this, init_options.platform_handle) {
		MakeCurrent();
		if (GLAD_GL_KHR_debug || GLAD_GL_ARB_debug_output) {
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(DebugCallback, this);
			// Enable all messages (no filtering by default).
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}
		else {
			// If debug output is not available, emit a warning.
			if (m_log_callback.Func) {
				m_log_callback.Func(
					LogSeverity::Warning,
					"OpenGL debug output not available",
					m_log_callback.user_data
				);
			}
		}
	}

	// ----------------------------------------------------------------------------
	// GetVendorID: returns PCI vendor ID by querying GL_VENDOR and mapping.
	// ----------------------------------------------------------------------------
	std::uint32_t OpenGLPhysicalDevice::GetVendorID() const noexcept {
		MakeCurrent();
		char const* vendor = reinterpret_cast<char const*>(glGetString(GL_VENDOR));
		if (!vendor)
			return 0;
		return MapVendorStringToID(vendor);
	}

	// ----------------------------------------------------------------------------
	// GetID: OpenGL does not have a numeric device ID, so return 0.
	// ----------------------------------------------------------------------------
	std::uint32_t OpenGLPhysicalDevice::GetID() const noexcept {
		return 0;
	}

	// ----------------------------------------------------------------------------
	// GetDescription: returns the GL_RENDERER string.
	// ----------------------------------------------------------------------------
	std::string OpenGLPhysicalDevice::GetDescription() const noexcept {
		MakeCurrent();
		char const* renderer = reinterpret_cast<char const*>(glGetString(GL_RENDERER));
		return renderer ? std::string(renderer) : std::string();
	}


} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)