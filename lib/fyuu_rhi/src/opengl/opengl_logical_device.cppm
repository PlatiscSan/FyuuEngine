/* opengl_logical_device.cppm - Module interface for OpenGL logical device */

// Module declaration: this file defines the OpenGLLogicalDevice class as part of the
// fyuu_rhi module partition :opengl_logical_device.
module;

#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_logical_device;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :opengl_common; 
import :opengl_physical_device; 

namespace fyuu_rhi::opengl {

	/**
	 * @class OpenGLLogicalDevice
	 * @brief Represents a logical OpenGL device (essentially the OpenGL context).
	 *
	 * In OpenGL, the logical device is simply the context that was created during
	 * physical device initialization. This class holds a SharedContext and forwards
	 * it to other components. No additional device creation is needed because OpenGL
	 * contexts are already fully functional after physical device creation.
	 *
	 */
	export class OpenGLLogicalDevice
		: public PolymorphicLogicalDeviceBase,
		public OpenGLCommon<OpenGLLogicalDevice> {
	public:
		/**
		 * @brief Constructs an OpenGLLogicalDevice from a physical device.
		 *
		 * Simply stores the context obtained from the physical device.
		 *
		 * @param physical_device The physical device that already owns the context.
		 */
		OpenGLLogicalDevice(OpenGLPhysicalDevice const& physical_device);

	};

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)