/* webgpu_logical_device.impl.cpp - Module implementation for WebGPU logical device */

module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <string_view>
#include <semaphore>
#include <format>
#endif // !defined(__cpp_lib_modules)

#include "webgpu.hpp"

module fyuu_rhi:webgpu_logical_device_impl;

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :webgpu_logical_device;
import :types;
import :webgpu_common;
import :webgpu_physical_device;

namespace fyuu_rhi::webgpu {

	// ----------------------------------------------------------------------------
	// UncapturedErrorCallback: forwards WebGPU errors to the application log.
	// ----------------------------------------------------------------------------
	void WebGPULogicalDevice::UncapturedErrorCallback(
		wgpu::Device const& device,
		wgpu::ErrorType type,
		wgpu::StringView message,
		LogCallback* log_callback
	) {
		auto&& [Func, user_data] = *log_callback;
		if (!Func) return;

		// Map WebGPU error type to RHI log severity.
		LogSeverity severity = LogSeverity::Error;
		switch (type) {
		case wgpu::ErrorType::Validation:
		case wgpu::ErrorType::OutOfMemory:
			severity = LogSeverity::Fatal;   // These are serious errors.
			break;
		case wgpu::ErrorType::Internal:
		case wgpu::ErrorType::Unknown:
		default:
			severity = LogSeverity::Error;   // General errors.
			break;
		}

		// Format and send the message.
		Func(
			severity,
			std::format("WebGPU uncaptured error: {}", message.data),
			user_data
		);
	}

	// ----------------------------------------------------------------------------
	// DeviceLostCallback: called when the WebGPU device is lost.
	// ----------------------------------------------------------------------------
	void WebGPULogicalDevice::DeviceLostCallback(
		wgpu::Device const& device,
		wgpu::DeviceLostReason reason,
		wgpu::StringView message,
		LogCallback* log_callback
	) {
		auto&& [Func, user_data] = *log_callback;
		if (!Func) return;

		// Convert reason enum to string.
		std::string reason_str;
		switch (reason) {
		case wgpu::DeviceLostReason::Destroyed:		 reason_str = "Destroyed"; break;
		case wgpu::DeviceLostReason::CallbackCancelled: reason_str = "CallbackCancelled"; break;
		case wgpu::DeviceLostReason::FailedCreation:	reason_str = "FailedCreation"; break;
		default:										 reason_str = "Unknown"; break;
		}

		// Log as fatal (device loss is critical).
		Func(
			LogSeverity::Fatal,
			std::format("WebGPU device lost ({}): {}", reason_str, message.data),
			user_data
		);
	}

	// ----------------------------------------------------------------------------
	// Constructor: requests a WebGPU device from the adapter.
	// ----------------------------------------------------------------------------
	WebGPULogicalDevice::WebGPULogicalDevice(WebGPUPhysicalDevice const& physical_device)
		: PolymorphicLogicalDeviceBase(this),
		WebGPUCommon(this),
		m_log_callback(physical_device.GetLogCallback()),
		m_impl(
			[this, &physical_device]() {
				wgpu::Adapter adapter = physical_device.GetNative();

				// Set up device descriptor with error and lost callbacks.
				wgpu::DeviceDescriptor device_desc = {};
				device_desc.label = "Main WebGPU Device";
				device_desc.requiredFeatureCount = 0;	  // No required features.
				device_desc.requiredLimits = nullptr;	  // No custom limits.
				device_desc.defaultQueue.label = "Default Queue";

				// Attach callbacks, passing the address of m_log_callback as user data.
				device_desc.SetUncapturedErrorCallback(UncapturedErrorCallback, &m_log_callback);
				device_desc.SetDeviceLostCallback(
					wgpu::CallbackMode::AllowProcessEvents,  // Callback may be called during wgpu::Device::ProcessEvents.
					DeviceLostCallback,
					&m_log_callback
				);

				// Context for synchronous waiting.
				struct RequestCtx {
					wgpu::Device device = nullptr;
					std::binary_semaphore semaphore{ 0 };
				} ctx;

				// Request the device asynchronously.
				adapter.RequestDevice(
					&device_desc,
					wgpu::CallbackMode::WaitAnyOnly,   // We will wait manually.
					[&ctx](wgpu::RequestDeviceStatus status,
						wgpu::Device device,
						wgpu::StringView message) {
							if (status == wgpu::RequestDeviceStatus::Success) {
								ctx.device = device;
							}
							// Signal completion regardless of success/failure.
							ctx.semaphore.release();
					});

				// Wait for the callback to complete.
				ctx.semaphore.acquire();

				if (!ctx.device) {
					throw std::runtime_error("Failed to obtain WebGPU device");
				}

				return ctx.device;
			}()) {
		// Constructor body is empty.
	}

    // ----------------------------------------------------------------------------
	// GetNative: returns the underlying wgpu::Device.
	// ----------------------------------------------------------------------------
	wgpu::Device WebGPULogicalDevice::GetNative() const noexcept {
		return m_impl;
	}

	// ----------------------------------------------------------------------------
	// Resource creation methods: each calls the corresponding device method,
	// checks for success, and optionally sets a debug name.
	// ----------------------------------------------------------------------------

	wgpu::Buffer WebGPULogicalDevice::CreateBuffer(
		wgpu::BufferDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::Buffer buffer = m_impl.CreateBuffer(&desc);
		if (!buffer) {
			throw std::runtime_error("Failed to create WebGPU buffer");
		}
		if (!debug_name.empty()) {
			SetDebugName(buffer, debug_name);
		}
		return buffer;
	}

	wgpu::Texture WebGPULogicalDevice::CreateTexture(
		wgpu::TextureDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::Texture texture = m_impl.CreateTexture(&desc);
		if (!texture) {
			throw std::runtime_error("Failed to create WebGPU texture");
		}
		if (!debug_name.empty()) {
			SetDebugName(texture, debug_name);
		}
		return texture;
	}

	wgpu::Sampler WebGPULogicalDevice::CreateSampler(
		wgpu::SamplerDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::Sampler sampler = m_impl.CreateSampler(&desc);
		if (!sampler) {
			throw std::runtime_error("Failed to create WebGPU sampler");
		}
		if (!debug_name.empty()) {
			SetDebugName(sampler, debug_name);
		}
		return sampler;
	}

	wgpu::BindGroupLayout WebGPULogicalDevice::CreateBindGroupLayout(
		wgpu::BindGroupLayoutDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::BindGroupLayout layout = m_impl.CreateBindGroupLayout(&desc);
		if (!layout) {
			throw std::runtime_error("Failed to create WebGPU bind group layout");
		}
		if (!debug_name.empty()) {
			SetDebugName(layout, debug_name);
		}
		return layout;
	}

	wgpu::PipelineLayout WebGPULogicalDevice::CreatePipelineLayout(
		wgpu::PipelineLayoutDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::PipelineLayout layout = m_impl.CreatePipelineLayout(&desc);
		if (!layout) {
			throw std::runtime_error("Failed to create WebGPU pipeline layout");
		}
		if (!debug_name.empty()) {
			SetDebugName(layout, debug_name);
		}
		return layout;
	}

	wgpu::ShaderModule WebGPULogicalDevice::CreateShaderModule(
		wgpu::ShaderModuleDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::ShaderModule module_ = m_impl.CreateShaderModule(&desc);
		if (!module_) {
			throw std::runtime_error("Failed to create WebGPU shader module");
		}
		if (!debug_name.empty()) {
			SetDebugName(module_, debug_name);
		}
		return module_;
	}

	wgpu::RenderPipeline WebGPULogicalDevice::CreateRenderPipeline(
		wgpu::RenderPipelineDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::RenderPipeline pipeline = m_impl.CreateRenderPipeline(&desc);
		if (!pipeline) {
			throw std::runtime_error("Failed to create WebGPU render pipeline");
		}
		if (!debug_name.empty()) {
			SetDebugName(pipeline, debug_name);
		}
		return pipeline;
	}

	wgpu::ComputePipeline WebGPULogicalDevice::CreateComputePipeline(
		wgpu::ComputePipelineDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::ComputePipeline pipeline = m_impl.CreateComputePipeline(&desc);
		if (!pipeline) {
			throw std::runtime_error("Failed to create WebGPU compute pipeline");
		}
		if (!debug_name.empty()) {
			SetDebugName(pipeline, debug_name);
		}
		return pipeline;
	}

	wgpu::CommandEncoder WebGPULogicalDevice::CreateCommandEncoder(
		wgpu::CommandEncoderDescriptor const& desc,
		std::string_view debug_name
	) const {
		wgpu::CommandEncoder encoder = m_impl.CreateCommandEncoder(&desc);
		if (!encoder) {
			throw std::runtime_error("Failed to create WebGPU command encoder");
		}
		if (!debug_name.empty()) {
			SetDebugName(encoder, debug_name);
		}
		return encoder;
	}

	wgpu::Queue WebGPULogicalDevice::GetQueue() const noexcept {
		return m_impl.GetQueue();
	}

} // namespace fyuu_rhi::webgpu