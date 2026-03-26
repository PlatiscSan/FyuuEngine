/* opengl_logical_device.impl.cpp - Module implementation for OpenGL logical device */

// This file provides the implementation of OpenGLLogicalDevice.
module;

#include "platform.hpp"
#include "glad.hpp"

// Define the module partition this file belongs to.
module fyuu_rhi:opengl_logical_device_impl;

// OpenGL is not supported on Apple.
#if !defined(__APPLE__)

// Import required module partitions.
import :opengl_logical_device;        // The class declaration.
import :types;
import :opengl_common;
import :opengl_physical_device;

namespace fyuu_rhi::opengl {

	// ----------------------------------------------------------------------------
	// Constructor: simply stores the context from the physical device.
	// ----------------------------------------------------------------------------
	OpenGLLogicalDevice::OpenGLLogicalDevice(OpenGLPhysicalDevice const& physical_device)
		: PolymorphicLogicalDeviceBase(this),
		OpenGLCommon(this, physical_device) {
		// No additional initialization needed.
	}

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)