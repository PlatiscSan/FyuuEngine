/* d3d12_physical_device.cppm - Module interface file for D3D12 physical device */

// Module declaration: this file is part of the fyuu_rhi module partition :d3d12_physical_device.
// It defines the interface of the D3D12PhysicalDevice class.
module;

#include <version>
#if !defined(__cpp_lib_modules)
#include <mutex>
#endif // !defined(__cpp_lib_modules)

#include "platform.hpp"
export module fyuu_rhi:d3d12_physical_device;

#if defined(_WIN32)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

// Import other partitions of the fyuu_rhi module: types and common utilities.
import :types;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	/**
	 * @class D3D12PhysicalDevice
	 * @brief Represents a physical GPU device (adapter) in the D3D12 backend.
	 *
	 * This class encapsulates an IDXGIAdapter4 object and provides methods to query
	 * adapter properties. It also manages the DXGI factory used to enumerate adapters.
	 *
	 * It inherits from PolymorphicPhysicalDeviceBase (to support polymorphic behavior
	 * in the RHI) and RHICommon<D3D12PhysicalDevice> (to provide common RHI facilities).
	 */
	export class D3D12PhysicalDevice
		: public PolymorphicPhysicalDeviceBase,
		public D3D12Common<D3D12PhysicalDevice> {
	private:
		// COM pointers to the DXGI factory and the chosen adapter.
		Microsoft::WRL::ComPtr<IDXGIFactory5> m_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_impl;
		// Logging callback provided by the application.
		LogCallback m_log_callback;

		/**
		 * @brief Enables the D3D12 debug layer and additional debugging features.
		 *
		 * This function must be called before creating any D3D12 device.
		 * It enables:
		 * - The basic debug layer.
		 * - GPU‑based validation (if GPU_BASED_VALIDATION is defined).
		 * - Device Removed Extended Data (DRED) for auto‑breadcrumbs and page fault reporting.
		 */
		static void EnableDebugLayer();

	public:
		/**
		 * @brief Constructs a D3D12PhysicalDevice.
		 *
		 * @param init_options Initialization options, including logging callback and whether
		 *                     to fall back to software (WARP) adapter.
		 * @throws std::runtime_error if no suitable physical device is found.
		 */
		D3D12PhysicalDevice(InitOptions const& init_options);

		/// Returns the raw IDXGIFactory5 pointer used by this object.
		IDXGIFactory5* GetRawFactory() const noexcept;

		/// Returns the raw IDXGIAdapter4 pointer representing the chosen GPU.
		IDXGIAdapter4* GetRawNative() const noexcept;

		/// Returns the logging callback.
		LogCallback const& GetLogCallback() const noexcept;

		/// Queries the vendor ID (e.g., NVIDIA = 0x10DE).
		std::uint32_t GetVendorID() const;
		
		/// Queries the device ID (specific model identifier).
		std::uint32_t GetID() const;

		/// Queries a human‑readable description of the adapter (e.g., "NVIDIA GeForce RTX 3080").
		std::string GetDescription() const;

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)