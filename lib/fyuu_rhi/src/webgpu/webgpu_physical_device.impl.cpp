/* webgpu_physical_device.impl.cpp - Module implementation for WebGPU physical device */

module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <string>
#include <chrono>
#include <semaphore>
#endif // !defined(__cpp_lib_modules)

#include "webgpu.hpp"

module fyuu_rhi:webgpu_physical_device_impl;

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :webgpu_physical_device;
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	// ----------------------------------------------------------------------------
	// Constructor: creates WebGPU instance and requests an adapter.
	// ----------------------------------------------------------------------------
	WebGPUPhysicalDevice::WebGPUPhysicalDevice(InitOptions const& init_options)
		: PolymorphicPhysicalDeviceBase(this),
		WebGPUCommon(this),
		m_log_callback(init_options.log_callback),
		// Step 1: Create the WebGPU instance.
		m_instance(
			[]() {
				wgpu::InstanceDescriptor instance_desc = {};
				// Create a default instance (no extra features requested).
				wgpu::Instance instance = wgpu::CreateInstance(&instance_desc);

				if (!instance) {
					throw std::runtime_error("Failed to create WebGPU instance");
				}

				return instance;
			}()),
		// Step 2: Request an adapter asynchronously and wait for the result.
		m_impl(
			[this]() {
				using namespace std::chrono_literals;

				// Context struct to pass data between the callback and the waiting thread.
				struct RequestAdapterContext {
					wgpu::Adapter adapter = nullptr;   // Will hold the obtained adapter.
					std::binary_semaphore semaphore{ 0 }; // Semaphore to signal completion.
				} ctx;

				// Configure adapter request: prefer high-performance GPU.
				wgpu::RequestAdapterOptions adapter_opts = {};
				adapter_opts.powerPreference = wgpu::PowerPreference::HighPerformance;

				// Issue the asynchronous adapter request.
				// The callback will be invoked when the request completes.
				m_instance.RequestAdapter(
					&adapter_opts,
					wgpu::CallbackMode::WaitAnyOnly,   // Indicates we will wait manually.
					[&ctx](
						wgpu::RequestAdapterStatus status,
						wgpu::Adapter adapter,
						wgpu::StringView message
						) {
							if (status == wgpu::RequestAdapterStatus::Success) {
								ctx.adapter = adapter;   // Store the adapter on success.
							}
							// Signal that the callback has finished.
							ctx.semaphore.release();
					}
				);

				// Wait until the callback signals completion.
				(void)ctx.semaphore.try_acquire_for(5s);

				// If no adapter was obtained, throw an error.
				if (!ctx.adapter) {
					throw std::runtime_error("Failed to get WebGPU adapter");
				}

				return ctx.adapter;
			}()) {
		// Constructor body is empty; all initialization was done in the initializer list.
	}

    // ----------------------------------------------------------------------------
	// GetVendorID: retrieves the vendor ID from the adapter info.
	// ----------------------------------------------------------------------------
	std::uint32_t WebGPUPhysicalDevice::GetVendorID() const noexcept {
		wgpu::AdapterInfo info;
		// wgpu::Adapter::GetInfo fills the provided structure with adapter properties.
		if (m_impl.GetInfo(&info) == wgpu::Status::Success) {
			return info.vendorID;   // Vendor ID (e.g., 0x10DE for NVIDIA).
		}
		return 0;   // Return 0 if info retrieval fails.
	}

	// ----------------------------------------------------------------------------
	// GetID: retrieves the device ID from the adapter info.
	// ----------------------------------------------------------------------------
	std::uint32_t WebGPUPhysicalDevice::GetID() const noexcept {
		wgpu::AdapterInfo info;
		if (m_impl.GetInfo(&info) == wgpu::Status::Success) {
			return info.deviceID;   // Device ID (specific GPU model).
		}
		return 0;
	}

	// ----------------------------------------------------------------------------
	// GetDescription: retrieves the human-readable adapter description.
	// ----------------------------------------------------------------------------
	std::string WebGPUPhysicalDevice::GetDescription() const noexcept {
		wgpu::AdapterInfo info;
		if (m_impl.GetInfo(&info) == wgpu::Status::Success) {
			// info.description is a wgpu::StringView, which has .data and .length.
			// Construct a std::string from that view.
			return std::string(info.description.data, info.description.length);
		}
		return {};
	}

	// ----------------------------------------------------------------------------
	// GetInstance: returns the WebGPU instance.
	// ----------------------------------------------------------------------------
	wgpu::Instance WebGPUPhysicalDevice::GetInstance() const noexcept {
		return m_instance;
	}

	// ----------------------------------------------------------------------------
	// GetNative: returns the WebGPU adapter.
	// ----------------------------------------------------------------------------
	wgpu::Adapter WebGPUPhysicalDevice::GetNative() const noexcept {
		return m_impl;
	}

	// ----------------------------------------------------------------------------
	// GetLogCallback: returns the stored logging callback.
	// ----------------------------------------------------------------------------
	LogCallback const& WebGPUPhysicalDevice::GetLogCallback() const noexcept {
		return m_log_callback;
	}

    wgpu::Surface WebGPUPhysicalDevice::CreateSurface(wgpu::SurfaceDescriptor& surface_desc) const {
        return m_instance.CreateSurface(&surface_desc);
    }

} // namespace fyuu_rhi::webgpu