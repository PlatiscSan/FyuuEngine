/* d3d12_logical_device.cppm - Module interface for D3D12 logical device */

// Module declaration: this file defines the D3D12LogicalDevice class as part of the
// fyuu_rhi module partition :d3d12_logical_device.
module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <memory> 
#include <string_view> 
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "declare_pool.hpp"
#if defined(_WIN32)
#include <D3D12MemAlloc.h>
#if defined(PIX_ENABLED)
#include "pix3.h"
#endif // defined(PIX_ENABLED)
#endif // defined(_WIN32)

// Export this module partition.
export module fyuu_rhi:d3d12_logical_device;

#if defined(_WIN32)

#if defined(__cpp_lib_modules)
import std;
#endif

import :types; 
import :d3d12_physical_device; 
import :d3d12_types; 
import :d3d12_descriptor_allocator;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	/**
	 * @brief Type alias for a unique_ptr that automatically unregisters a wait handle.
	 *
	 * Uses UnregisterWait as the deleter.
	 */
	export using DeviceRemovedHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&UnregisterWait)>;

	/**
	 * @class D3D12LogicalDevice
	 * @brief Represents a logical D3D12 device (ID3D12Device2) with associated resources.
	 *
	 * This class encapsulates:
	 * - The actual D3D12 device (ID3D12Device2).
	 * - A device‑removed event and callback for handling GPU crashes.
	 * - A D3D12MA allocator for GPU memory management.
	 * - Descriptor allocators for various view types (RTV, DSV, SRV/CBV/UAV, samplers).
	 *
	 */
	export class D3D12LogicalDevice :
		public PolymorphicLogicalDeviceBase,
		public D3D12Common<D3D12LogicalDevice> {
	private:
		// Logging callback provided by the application.
		LogCallback m_log_callback;

		// The actual D3D12 device (version 2 for extended features).
		Microsoft::WRL::ComPtr<ID3D12Device2> m_impl;

		DWORD m_callback_cookie;

		// Event and fence used to detect device removal.
		EventHandle m_device_removed_event;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_device_removed_fence;

		// Handle for the registered device‑removed callback (auto‑unregistered).
		DeviceRemovedHandle m_device_removed_handle;

		// D3D12MA allocator for GPU memory (buffers, textures).
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_video_memory_alloc;

		// Descriptor allocators for different heap types.
		std::unique_ptr<D3D12UniversalViewAllocator> m_universal_view_allocator;   // CBV/SRV/UAV
		std::unique_ptr<D3D12RTVAllocator> m_render_target_view_allocator;         // RTV
		std::unique_ptr<D3D12DSVAllocator> m_depth_stencil_view_allocator;         // DSV
		std::unique_ptr<D3D12SamplerAllocator> m_sampler_allocator;                // Samplers

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_multidraw, m_multidraw_indexed, m_dispatch_indirect;

		/**
		 * @brief Static callback invoked when the device‑removed event is signaled.
		 *
		 * This function queries DRED (Device Removed Extended Data) to log detailed
		 * information about the cause of the device removal (GPU crash, TDR, etc.).
		 *
		 * @param context Pointer to the D3D12LogicalDevice instance.
		 * @param BOOLEAN Unused (required by RegisterWaitForSingleObject).
		 */
		static void DeviceRemovedCallback(PVOID context, BOOLEAN);

		static void D3D12MessageCallback(
			D3D12_MESSAGE_CATEGORY category,
			D3D12_MESSAGE_SEVERITY severity,
			D3D12_MESSAGE_ID ID,
			LPCSTR description,
			void* context
		);

	public:
		/**
		 * @brief Constructs a D3D12LogicalDevice from a given physical device.
		 *
		 * Initialization steps:
		 * 1. Create the ID3D12Device2 using D3D12CreateDevice.
		 * 2. Enable debug layer filters in debug builds.
		 * 3. Create a fence and event for device‑removed detection.
		 * 4. Register a wait on that event (DeviceRemovedCallback).
		 * 5. Create the D3D12MA allocator.
		 * 6. Create all descriptor allocators.
		 *
		 * @param physical_device The physical device (adapter) to use.
		 */
		D3D12LogicalDevice(D3D12PhysicalDevice const& physical_device);

		~D3D12LogicalDevice() noexcept;

		/// @return The native ID3D12Device2 as a ComPtr.
		Microsoft::WRL::ComPtr<ID3D12Device2> GetNative() const noexcept;

		/// @return The raw pointer to ID3D12Device2.
		ID3D12Device2* GetRawNative() const noexcept;

		/// @return The D3D12MA allocator as a ComPtr.
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> GetVideoMemoryAllocator() const noexcept;

		/// @return The raw pointer to the D3D12MA allocator.
		D3D12MA::Allocator* GetRawVideoMemoryAllocator() const noexcept;

		/// @return The logging callback.
		LogCallback GetLogCallback() const noexcept;

		/// @return Pointer to the universal (CBV/SRV/UAV) descriptor allocator.
		D3D12UniversalViewAllocator* GetUniversalViewAllocator() const noexcept;

		/// @return Pointer to the render target view descriptor allocator.
		D3D12RTVAllocator* GetRenderTargetViewAllocator() const noexcept;

		/// @return Pointer to the depth stencil view descriptor allocator.
		D3D12DSVAllocator* GetDepthStencilViewAllocator() const noexcept;

		/// @return Pointer to the sampler descriptor allocator.
		D3D12SamplerAllocator* GetSamplerAllocator() const noexcept;

		/**
		 * @brief Creates a new command list of the specified type.
		 *
		 * Also creates an associated command allocator and stores it as private data
		 * in the command list for later retrieval.
		 *
		 * @param type       The type of command list (DIRECT, BUNDLE, COMPUTE, COPY).
		 * @param debug_name Optional debug name (used in debug builds).
		 * @return The created command list (ID3D12GraphicsCommandList4).
		 */
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(
			D3D12_COMMAND_LIST_TYPE type,
			std::wstring_view debug_name = {}) const;

		/**
		 * @brief Creates a fence with optional debug name.
		 *
		 * @param debug_name Optional debug name.
		 * @return The created fence (ID3D12Fence1).
		 */
		Microsoft::WRL::ComPtr<ID3D12Fence1> CreateFence(std::wstring_view debug_name = {}) const;

		/**
		 * @brief Creates a committed resource (buffer or texture).
		 *
		 * @param heap_properties        Heap properties (type, CPU page properties, etc.).
		 * @param heap_flags             Additional heap flags.
		 * @param desc                   Resource description (dimensions, format, flags).
		 * @param initial_resource_state Initial resource state (e.g., D3D12_RESOURCE_STATE_COMMON).
		 * @param optimized_clear_value  Optional clear value for performance optimization.
		 * @param debug_name             Optional debug name.
		 * @return The created resource (ID3D12Resource2).
		 */
		Microsoft::WRL::ComPtr<ID3D12Resource2> CreateCommittedResource(
			D3D12_HEAP_PROPERTIES const* heap_properties,
			D3D12_HEAP_FLAGS heap_flags,
			D3D12_RESOURCE_DESC const* desc,
			D3D12_RESOURCE_STATES initial_resource_state,
			D3D12_CLEAR_VALUE const* optimized_clear_value,
			std::wstring_view debug_name = {}
		) const;
		
		/**
		 * @brief Creates a command queue of the specified type and priority.
		 * 
		 * @param type      The type of command list this queue will execute 
		 *                  (DIRECT, COMPUTE, COPY, etc.).
		 * @param priority  The priority level for the queue 
		 *                  (e.g., D3D12_COMMAND_QUEUE_PRIORITY_NORMAL).
		 * @param debug_name Optional debug name to associate with the queue 
		 *                  (used in debug builds for identification).
		 * @return The created command queue as a ComPtr<ID3D12CommandQueue>.
		 */
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
			D3D12_COMMAND_LIST_TYPE type,
			D3D12_COMMAND_QUEUE_PRIORITY priority,
			std::wstring_view debug_name = {}
		) const;

		void CreateSRV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_SHADER_RESOURCE_VIEW_DESC const& srv_desc) const noexcept;

		void CreateUAV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uav_desc) const noexcept;

		void CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_CONSTANT_BUFFER_VIEW_DESC const& cbv_desc) const noexcept;

		void CreateRTV(ID3D12Resource* resource,D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_RENDER_TARGET_VIEW_DESC const& rtv_desc) const noexcept;

		void CreateDSV(ID3D12Resource* resource,D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_DEPTH_STENCIL_VIEW_DESC const& dsv_desc) const noexcept;

		void CreateSampler(D3D12_SAMPLER_DESC const& sampler_desc, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) const noexcept;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3DBlob* signature_blob) const;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& desc) const; 

		Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePipeline(D3D12_COMPUTE_PIPELINE_STATE_DESC const& desc) const; 

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> GetMultiDrawSignature() const noexcept;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> GetMultiDrawIndexedSignature() const noexcept;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> GetDispatchIndirectSignature() const noexcept;

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)