/* webgpu_physical_device.cppm - Module interface for WebGPU physical device */

module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <string>
#include <semaphore>
#endif // !defined(__cpp_lib_modules)


#include "webgpu.hpp"

export module fyuu_rhi:webgpu_physical_device;


#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

// Import external static polymorphism library and internal RHI types/utilities.
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	/**
	 * @class WebGPUPhysicalDevice
	 * @brief Represents a physical WebGPU adapter (GPU) in the RHI.
	 *
	 * This class encapsulates a wgpu::Adapter object and provides methods to query
	 * adapter properties (vendor ID, device ID, description). It also holds the
	 * wgpu::Instance that created the adapter.
	 *
	 * It inherits from PolymorphicPhysicalDeviceBase (for polymorphic behavior)
	 * and WebGPUCommon<WebGPUPhysicalDevice> (for common RHI infrastructure).
	 */
	export class WebGPUPhysicalDevice
		: public PolymorphicPhysicalDeviceBase,
		public WebGPUCommon<WebGPUPhysicalDevice> {

	private:
		// Logging callback provided by the application.
		LogCallback m_log_callback;

		// WebGPU instance (owns the adapter and the global WebGPU state).
		wgpu::Instance m_instance;

		// The selected WebGPU adapter (represents the physical GPU).
		wgpu::Adapter m_impl;

	public:
		/**
		 * @brief Constructs a WebGPUPhysicalDevice.
		 *
		 * Initialization steps:
		 * 1. Create a wgpu::Instance.
		 * 2. Request an adapter asynchronously using m_instance.RequestAdapter().
		 *    A binary semaphore is used to wait for the callback to complete.
		 * 3. Store the obtained adapter.
		 *
		 * @param init_options Initialization options (includes logging callback).
		 * @throws std::runtime_error if instance creation or adapter request fails.
		 */
		WebGPUPhysicalDevice(InitOptions const& init_options);

		/// @return Vendor ID (PCI vendor ID) from the adapter info.
		std::uint32_t GetVendorID() const noexcept;

		/// @return Device ID (PCI device ID) from the adapter info.
		std::uint32_t GetID() const noexcept;

		/// @return Human-readable description of the adapter (e.g., "NVIDIA GeForce RTX 3080").
		std::string GetDescription() const noexcept;

		/// @return The underlying WebGPU instance.
		wgpu::Instance GetInstance() const noexcept;

		/// @return The underlying WebGPU adapter.
		wgpu::Adapter GetNative() const noexcept;

		/// @return The logging callback.
		LogCallback const& GetLogCallback() const noexcept;

		wgpu::Surface CreateSurface(wgpu::SurfaceDescriptor& surface_desc) const;
	};

} // namespace fyuu_rhi::webgpu