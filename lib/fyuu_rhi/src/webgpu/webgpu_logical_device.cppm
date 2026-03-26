/* webgpu_logical_device.cppm - Module interface for WebGPU logical device */

// Module declaration: this file defines the WebGPULogicalDevice class as part of the
// fyuu_rhi module partition :webgpu_logical_device.
module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <string_view>
#endif // !defined(__cpp_lib_modules)

#include "webgpu.hpp"

// Export this module partition.
export module fyuu_rhi:webgpu_logical_device;

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :types;
import :webgpu_common;
import :webgpu_physical_device;

namespace fyuu_rhi::webgpu {

	/**
	 * @class WebGPULogicalDevice
	 * @brief Represents a logical WebGPU device (wgpu::Device) with associated resources.
	 *
	 * This class encapsulates a wgpu::Device object and provides factory methods for
	 * creating various WebGPU resources (buffers, textures, pipelines, etc.). It also
	 * sets up error and device‑lost callbacks to forward messages to the application's
	 * logging system.
	 */
	export class WebGPULogicalDevice
		: public PolymorphicLogicalDeviceBase,
		public WebGPUCommon<WebGPULogicalDevice> {
	private:
		// Logging callback provided by the application.
		LogCallback m_log_callback;

		// The actual WebGPU device.
		wgpu::Device m_impl;

		/**
		 * @brief Static callback for uncaptured errors.
		 *
		 * Called by the WebGPU implementation when an error occurs that is not captured
		 * by a specific error scope. Forwards the error message to the logging callback.
		 *
		 * @param device        The device that generated the error.
		 * @param type          The type of error (Validation, OutOfMemory, etc.).
		 * @param message       Descriptive error message.
		 * @param log_callback  Pointer to the application's LogCallback.
		 */
		static void UncapturedErrorCallback(
			wgpu::Device const& device,
			wgpu::ErrorType type,
			wgpu::StringView message,
			LogCallback* log_callback
		);

		/**
		 * @brief Static callback for device lost events.
		 *
		 * Called when the device is lost (e.g., due to a driver crash, or intentionally destroyed).
		 * Forwards the reason and message to the logging callback.
		 *
		 * @param device        The lost device.
		 * @param reason        Reason for the loss (Destroyed, FailedCreation, etc.).
		 * @param message       Additional information.
		 * @param log_callback  Pointer to the application's LogCallback.
		 */
		static void DeviceLostCallback(
			wgpu::Device const& device,
			wgpu::DeviceLostReason reason,
			wgpu::StringView message,
			LogCallback* log_callback
		);

	public:
		/**
		 * @brief Constructs a WebGPULogicalDevice from a given physical device.
		 *
		 * Initialization steps:
		 * 1. Store the logging callback from the physical device.
		 * 2. Create a device descriptor with error and device‑lost callbacks.
		 * 3. Request the device asynchronously from the adapter and wait for completion.
		 * 4. Store the obtained device.
		 *
		 * @param physical_device The physical device (adapter) to use.
		 * @throws std::runtime_error if device creation fails.
		 */
		WebGPULogicalDevice(WebGPUPhysicalDevice const& physical_device);

		/// @return The native WebGPU device handle.
		wgpu::Device GetNative() const noexcept;

		/**
		 * @brief Sets a debug label on any WebGPU object that supports SetLabel.
		 *
		 * @tparam T Type of the WebGPU object (e.g., wgpu::Buffer, wgpu::Texture).
		 * @param object The object to name.
		 * @param name   The debug name (UTF‑8 string view).
		 */
		template <class T>
		void SetDebugName(T& object, std::string_view name) const noexcept {
			object.SetLabel(wgpu::StringView{ name.data(), name.size() });
		}

		// ------------------------------------------------------------------------
		// Resource creation methods (all throw std::runtime_error on failure).
		// Each method optionally assigns a debug name if provided.
		// ------------------------------------------------------------------------

		/// Creates a buffer with the given descriptor.
		wgpu::Buffer CreateBuffer(wgpu::BufferDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a texture with the given descriptor.
		wgpu::Texture CreateTexture(wgpu::TextureDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a sampler with the given descriptor.
		wgpu::Sampler CreateSampler(wgpu::SamplerDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a bind group layout with the given descriptor.
		wgpu::BindGroupLayout CreateBindGroupLayout(wgpu::BindGroupLayoutDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a pipeline layout with the given descriptor.
		wgpu::PipelineLayout CreatePipelineLayout(wgpu::PipelineLayoutDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a shader module with the given descriptor.
		wgpu::ShaderModule CreateShaderModule(wgpu::ShaderModuleDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a render pipeline with the given descriptor.
		wgpu::RenderPipeline CreateRenderPipeline(wgpu::RenderPipelineDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a compute pipeline with the given descriptor.
		wgpu::ComputePipeline CreateComputePipeline(wgpu::ComputePipelineDescriptor const& desc, std::string_view debug_name = {}) const;

		/// Creates a command encoder with the given descriptor.
		wgpu::CommandEncoder CreateCommandEncoder(wgpu::CommandEncoderDescriptor const& desc, std::string_view debug_name = {}) const;

		wgpu::Queue GetQueue() const noexcept;
	};

} // namespace fyuu_rhi::webgpu