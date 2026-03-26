/* d3d12_logical_device.impl.cpp - Module implementation for D3D12 logical device */

// This file provides the implementation of D3D12LogicalDevice.
module;

#include <version>

#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <string_view>
#include <format>
#include <print>
#endif // !defined(__cpp_lib_modules)

#include "platform.hpp"
#include "declare_pool.hpp"

#if defined(_WIN32)
#include <D3D12MemAlloc.h>
#if defined(PIX_ENABLED)
#include "pix3.h"
#endif // defined(PIX_ENABLED)
#endif // defined(_WIN32)

module fyuu_rhi:d3d12_logical_device_impl;

#if defined(_WIN32)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

import :d3d12_logical_device;
import plastic.registrable;
import :types;
import :d3d12_throw;
import :d3d12_physical_device;
import :d3d12_types;
import :d3d12_descriptor_allocator;

// ----------------------------------------------------------------------------
// Anonymous namespace: helper functions for converting DRED enums to strings.
// ----------------------------------------------------------------------------
namespace {

	/**
	 * @brief Converts a D3D12_DRED_ALLOCATION_TYPE to a human-readable string.
	 */
	constexpr std::string_view D3D12_DRED_ALLOCATION_TYPE_to_string(D3D12_DRED_ALLOCATION_TYPE type) noexcept {
		switch (type) {
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:           return "COMMAND_QUEUE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:       return "COMMAND_ALLOCATOR";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:          return "PIPELINE_STATE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:            return "COMMAND_LIST";
		case D3D12_DRED_ALLOCATION_TYPE_FENCE:                   return "FENCE";
		case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:         return "DESCRIPTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_HEAP:                    return "HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:              return "QUERY_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:       return "COMMAND_SIGNATURE";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:        return "PIPELINE_LIBRARY";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:           return "VIDEO_DECODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:         return "VIDEO_PROCESSOR";
		case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:                return "RESOURCE";
		case D3D12_DRED_ALLOCATION_TYPE_PASS:                    return "PASS";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:           return "CRYPTOSESSION";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:     return "CRYPTOSESSIONPOLICY";
		case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION: return "PROTECTEDRESOURCESESSION";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:      return "VIDEO_DECODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:            return "COMMAND_POOL";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:        return "COMMAND_RECORDER";
		case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:            return "STATE_OBJECT";
		case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:             return "METACOMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:         return "SCHEDULINGGROUP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:  return "VIDEO_MOTION_ESTIMATOR";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP: return "VIDEO_MOTION_VECTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND: return "VIDEO_EXTENSION_COMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER:           return "VIDEO_ENCODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP:      return "VIDEO_ENCODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_INVALID:                 return "INVALID";
		default:                                                  return "Unknown D3D12_DRED_ALLOCATION_TYPE";
		}
	}

	/**
	 * @brief Converts a D3D12_AUTO_BREADCRUMB_OP to a human-readable string.
	 */
	constexpr std::string_view D3D12AutoBreadcrumbOpToString(D3D12_AUTO_BREADCRUMB_OP op) noexcept {
		switch (op) {
		case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:                          return "SETMARKER";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:                         return "BEGINEVENT";
		case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:                           return "ENDEVENT";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:                      return "DRAWINSTANCED";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:               return "DRAWINDEXEDINSTANCED";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:                    return "EXECUTEINDIRECT";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:                           return "DISPATCH";
		case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:                   return "COPYBUFFERREGION";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:                  return "COPYTEXTUREREGION";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:                       return "COPYRESOURCE";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:                          return "COPYTILES";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:                 return "RESOLVESUBRESOURCE";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:              return "CLEARRENDERTARGETVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:           return "CLEARUNORDEREDACCESSVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:              return "CLEARDEPTHSTENCILVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:                    return "RESOURCEBARRIER";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:                      return "EXECUTEBUNDLE";
		case D3D12_AUTO_BREADCRUMB_OP_PRESENT:                            return "PRESENT";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:                   return "RESOLVEQUERYDATA";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:                    return "BEGINSUBMISSION";
		case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:                      return "ENDSUBMISSION";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:                        return "DECODEFRAME";
		case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:                      return "PROCESSFRAMES";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:               return "ATOMICCOPYBUFFERUINT";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:             return "ATOMICCOPYBUFFERUINT64";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:           return "RESOLVESUBRESOURCEREGION";
		case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:               return "WRITEBUFFERIMMEDIATE";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:                       return "DECODEFRAME1";
		case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:        return "SETPROTECTEDRESOURCESESSION";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:                       return "DECODEFRAME2";
		case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:                     return "PROCESSFRAMES1";
		case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE: return "BUILDRAYTRACINGACCELERATIONSTRUCTURE";
		case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO: return "EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE: return "COPYRAYTRACINGACCELERATIONSTRUCTURE";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:                       return "DISPATCHRAYS";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:              return "INITIALIZEMETACOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:                 return "EXECUTEMETACOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:                     return "ESTIMATEMOTION";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:            return "RESOLVEMOTIONVECTORHEAP";
		case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:                  return "SETPIPELINESTATE1";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:         return "INITIALIZEEXTENSIONCOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:            return "EXECUTEEXTENSIONCOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:                       return "DISPATCHMESH";
		case D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME:                        return "ENCODEFRAME";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA:       return "RESOLVEENCODEROUTPUTMETADATA";
		default:                                                           return "Unknown D3D12_AUTO_BREADCRUMB_OP";
		}
	}

    UINT GetFormatSize(DXGI_FORMAT format) noexcept {
        switch (format) {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 16u;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 12u;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return 8u;
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
            return 8u;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 8u;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return 4u;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return 4u;
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
            return 4u;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            return 4u;
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return 4u;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
            return 2u;
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
            return 2u;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
            return 1u;
        default:
            return 0u;
        }
    }

} // anonymous namespace

namespace fyuu_rhi::d3d12 {

	// ----------------------------------------------------------------------------
	// DeviceRemovedCallback: static callback invoked when the device‑removed event fires.
	// ----------------------------------------------------------------------------
	void D3D12LogicalDevice::DeviceRemovedCallback(PVOID context, BOOLEAN) {
		D3D12LogicalDevice* device = static_cast<D3D12LogicalDevice*>(context);
		ID3D12Device2* d3d12_device = device->m_impl.Get();

		HRESULT reason = d3d12_device->GetDeviceRemovedReason();
		if (reason == S_OK) {
			return; // Proper shutdown, no device removal actually occurred.
		}

#if defined(PIX_ENABLED) && defined(TDR_PIX_CAPTURE)
		// If we were capturing with PIX, end the capture on TDR.
		PIXEndCapture(FALSE);
#endif

#if !defined(_NDEBUG)
		// In debug builds, log extensive DRED information.
		auto [callback, user_data] = device->m_log_callback;
		if (!callback) {
			return;
		}

		// Log the HRESULT reason as a string.
		{
			_com_error error(reason, nullptr);
#if defined(_UNICODE)
			// Convert wide error message to UTF-8 using a PMR pool allocator.
			DECLARE_POOL(message, char, 256ull)
			std::pmr::string message = common::UTF16ToUTF8(error.ErrorMessage(), message_alloc);
			callback(LogSeverity::Fatal, message, user_data);
#else
			callback(LogSeverity::Fatal, error.ErrorMessage(), user_data);
#endif
		}

		// Query DRED (Device Removed Extended Data) interfaces.
		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> dred;
		ThrowIfFailed(d3d12_device->QueryInterface(IID_PPV_ARGS(&dred)));

		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dred_auto_breadcrumbs_output;
		D3D12_DRED_PAGE_FAULT_OUTPUT dred_page_fault_output;
		ThrowIfFailed(dred->GetAutoBreadcrumbsOutput(&dred_auto_breadcrumbs_output));
		ThrowIfFailed(dred->GetPageFaultAllocationOutput(&dred_page_fault_output));

		// Log auto‑breadcrumb data (command history).
		{
			auto node = dred_auto_breadcrumbs_output.pHeadAutoBreadcrumbNode;
			while (node) {
				callback(LogSeverity::Fatal,
					std::format("DRED Breadcrumb Data:\n\tCommand List:\t{}\n\tCommand Queue:\t{}\n",
						node->pCommandListDebugNameA == nullptr ? "[Unnamed CommandList]" : node->pCommandListDebugNameA,
						node->pCommandQueueDebugNameA == nullptr ? "[Unnamed CommandQueue]" : node->pCommandQueueDebugNameA),
					user_data);

				for (UINT32 i = 0; i < node->BreadcrumbCount; i++) {
					auto& command = node->pCommandHistory[i];
					auto str = D3D12AutoBreadcrumbOpToString(command);
					const char* failStr = "";
					if (i == *node->pLastBreadcrumbValue) {
						failStr = "\t\t <- failed";
					}
					callback(LogSeverity::Fatal, std::format("\t\t{}{}\n", str, failStr), user_data);
				}
				node = node->pNext;
			}
		}

		// Log page fault information.
		callback(LogSeverity::Fatal,
			std::format("DRED Page Fault Output:\n\tVirtual Address: {:X}", dred_page_fault_output.PageFaultVA),
			user_data);

		// Log allocations that existed at the time of the fault.
		{
			auto node = dred_page_fault_output.pHeadExistingAllocationNode;
			while (node) {
				callback(LogSeverity::Fatal,
					std::format("\tDRED Page Fault Existing Allocation Node:\n\t\tObject Name: {}\n\t\tAllocation Type: {}\n",
						node->ObjectNameA ? node->ObjectNameA : "[Unnamed Object]",
						D3D12_DRED_ALLOCATION_TYPE_to_string(node->AllocationType)),
					user_data);
				node = node->pNext;
			}
		}

		// Log recently freed allocations.
		{
			auto node = dred_page_fault_output.pHeadRecentFreedAllocationNode;
			while (node) {
				callback(LogSeverity::Fatal,
					std::format("\tDRED Page Fault Recent Freed Allocation Node:\n\t\tObject Name: {}\n\t\tAllocation Type: {}\n",
						node->ObjectNameA ? node->ObjectNameA : "[Unnamed Object]",
						D3D12_DRED_ALLOCATION_TYPE_to_string(node->AllocationType)),
					user_data);
				node = node->pNext;
			}
		}
#endif // !defined(_NDEBUG)

		// Final fatal message.
		callback(LogSeverity::Fatal, "Device removal triggered!", user_data);
	}

	void D3D12LogicalDevice::D3D12MessageCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID ID, LPCSTR description, void* context) {
	
		std::size_t id = reinterpret_cast<std::size_t>(context);
		D3D12LogicalDevice* device = plastic::utility::QueryObject<D3D12LogicalDevice>(id);
		if (!device) {
			return;
		}
		
		auto [callback, user_data] = device->m_log_callback;
		if (!callback) {
			return;
		}

		// Map severity
		LogSeverity log_severity;
		switch (severity) {
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			log_severity = LogSeverity::Fatal;
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
			log_severity = LogSeverity::Error;
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			log_severity = LogSeverity::Warning;
			break;
		case D3D12_MESSAGE_SEVERITY_INFO:
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
		default:
			log_severity = LogSeverity::Info;
			break;
		}

		// Format message with category and ID
		std::string formatted_message = std::format("[D3D12] Category: {}, ID: {} - {}", static_cast<std::size_t>(category), static_cast<std::size_t>(ID), description);
		callback(log_severity, formatted_message, user_data);

	}

	// ----------------------------------------------------------------------------
	// Constructor: creates the D3D12 device and all associated resources.
	// ----------------------------------------------------------------------------
	D3D12LogicalDevice::D3D12LogicalDevice(D3D12PhysicalDevice const& physical_device)
		: PolymorphicLogicalDeviceBase(this),
		D3D12Common(this),
		m_log_callback(physical_device.GetLogCallback()),
		// Create the D3D12 device (feature level 12.0).
		m_impl(
			[&physical_device]() {
				Microsoft::WRL::ComPtr<ID3D12Device2> device;
				ThrowIfFailed(
					D3D12CreateDevice(
						physical_device.GetRawNative(),
						D3D_FEATURE_LEVEL_12_0,
						IID_PPV_ARGS(&device)
					)
				);

				return device;
			}()),
		m_callback_cookie(
			[this](){
#if defined(PIX_ENABLED) && defined(TDR_PIX_CAPTURE)
				// If PIX capture is enabled and we are set to capture on TDR, load the library and begin capture.
				auto lib = PIXLoadLatestWinPixGpuCapturerLibrary();
				Assert(lib != nullptr, "Failed to load PIX library");
				PIXCaptureParameters params{
					.GpuCaptureParameters = {
						.FileName = L"TDRCap.pix",
					}
				};
				ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_GPU, &params));
#endif
#if !defined(_NDEBUG)
				// In debug builds, configure the info queue (debug layer) to break on errors and filter spam.
				Microsoft::WRL::ComPtr<ID3D12InfoQueue1> info_queue;
				if (SUCCEEDED(m_impl.As(&info_queue))) {
#if defined(PIX_ENABLED) && defined(TDR_PIX_CAPTURE)
					// If PIX capture is active, disabling breaks can interfere with captures.
					if (!PIXIsAttachedForGpuCapture()) {
#endif
						info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
						info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
						info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#if defined(PIX_ENABLED) && defined(TDR_PIX_CAPTURE)
					}
#endif
					DWORD callback_cookie = 0;
					ThrowIfFailed(
						info_queue->RegisterMessageCallback(
							D3D12MessageCallback,
							D3D12_MESSAGE_CALLBACK_FLAG_NONE,
							reinterpret_cast<void*>(*GetID()),
							&callback_cookie 
						)
					);

					// Suppress specific message IDs and severities that are too noisy.
					D3D12_MESSAGE_SEVERITY severities[] = {
						D3D12_MESSAGE_SEVERITY_INFO     // Ignore informational messages.
					};

					D3D12_MESSAGE_ID denied_ids[] = {
						D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // Clear with non‑optimized color.
						D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                       // Occurs during graphics debugging.
						D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                     // Occurs during graphics debugging.
					};

					D3D12_INFO_QUEUE_FILTER new_filter = {};
					new_filter.DenyList.NumSeverities = _countof(severities);
					new_filter.DenyList.pSeverityList = severities;
					new_filter.DenyList.NumIDs = _countof(denied_ids);
					new_filter.DenyList.pIDList = denied_ids;

					ThrowIfFailed(info_queue->PushStorageFilter(&new_filter));

					return callback_cookie;

				}
				return DWORD(0);
#endif // !defined(_NDEBUG)
			}()),
		// Create an event for device removal notification.
		m_device_removed_event(CreateEventHandle()),
		// Create a fence and set it to signal the event when the fence reaches UINT64_MAX.
		m_device_removed_fence(
			[this]() {
				Microsoft::WRL::ComPtr<ID3D12Fence> device_removed_fence;
				ThrowIfFailed(m_impl->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&device_removed_fence)));
				device_removed_fence->SetEventOnCompletion(UINT64_MAX, m_device_removed_event.get());
				return device_removed_fence;
			}()),
		// Register a wait on that event. The callback will be called when the event fires.
		m_device_removed_handle(
			[this]() {
				HANDLE wait_handle;
				RegisterWaitForSingleObject(
					&wait_handle,
					m_device_removed_event.get(),
					DeviceRemovedCallback,
					this,                    // Pass instance as context.
					INFINITE,                 // No timeout.
					0                         // No flags.
				);
				return DeviceRemovedHandle(wait_handle, UnregisterWait);
			}()),
		// Create the D3D12MA allocator for GPU memory management.
		m_video_memory_alloc(
			[this, &physical_device]() {
				Microsoft::WRL::ComPtr<D3D12MA::Allocator> alloc;

				D3D12MA::ALLOCATOR_DESC allocator_desc = {};
				allocator_desc.pDevice = m_impl.Get();
				allocator_desc.pAdapter = physical_device.GetRawNative();

				D3D12MA::CreateAllocator(&allocator_desc, &alloc);
				return alloc;
			}()),

		// Create descriptor allocators for various view types.
		m_universal_view_allocator(std::make_unique<D3D12UniversalViewAllocator>(m_impl.Get())),
		m_render_target_view_allocator(std::make_unique<D3D12RTVAllocator>(m_impl.Get())),
		m_depth_stencil_view_allocator(std::make_unique<D3D12DSVAllocator>(m_impl.Get())),
		m_sampler_allocator(std::make_unique<D3D12SamplerAllocator>(m_impl.Get())),
		m_multidraw(
			[this]() {

				Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

				D3D12_INDIRECT_ARGUMENT_DESC argument_descs[] = {
					{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW }
				};
				D3D12_COMMAND_SIGNATURE_DESC signature{
					.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
					.NumArgumentDescs = std::size(argument_descs),
					.pArgumentDescs = argument_descs,
					.NodeMask = 0
				};
				m_impl->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));

				return cmd_sgn;
			}()),
		m_multidraw_indexed(
			[this]() {
				Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

				D3D12_INDIRECT_ARGUMENT_DESC argument_descs[] = {
					{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED }
				};
				D3D12_COMMAND_SIGNATURE_DESC signature{
					.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
					.NumArgumentDescs = std::size(argument_descs),
					.pArgumentDescs = argument_descs,
					.NodeMask = 0
				};
				m_impl->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));

				return cmd_sgn;
			}()),
		m_dispatch_indirect(
			[this]() {
				Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

				D3D12_INDIRECT_ARGUMENT_DESC argument_descs[] = {
					{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH }
				};
				D3D12_COMMAND_SIGNATURE_DESC signature{
					.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
					.NumArgumentDescs = std::size(argument_descs),
					.pArgumentDescs = argument_descs,
					.NodeMask = 0
				};
				m_impl->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));

				return cmd_sgn;
			}()) {
	}

	D3D12LogicalDevice::~D3D12LogicalDevice() noexcept {
#if !defined(_NDEBUG)
		Microsoft::WRL::ComPtr<ID3D12InfoQueue1> info_queue;
		if (SUCCEEDED(m_impl.As(&info_queue))) {
			info_queue->UnregisterMessageCallback(m_callback_cookie);
		}
#endif // !defined(_NDEBUG)
	}

	// ----------------------------------------------------------------------------
	// Simple getters.
	// ----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12Device2> D3D12LogicalDevice::GetNative() const noexcept {
		return m_impl;
	}

	ID3D12Device2* D3D12LogicalDevice::GetRawNative() const noexcept {
		return m_impl.Get();
	}

	Microsoft::WRL::ComPtr<D3D12MA::Allocator> D3D12LogicalDevice::GetVideoMemoryAllocator() const noexcept {
		return m_video_memory_alloc;
	}

	D3D12MA::Allocator* D3D12LogicalDevice::GetRawVideoMemoryAllocator() const noexcept {
		return m_video_memory_alloc.Get();
	}

	LogCallback D3D12LogicalDevice::GetLogCallback() const noexcept {
		return m_log_callback;
	}

	D3D12UniversalViewAllocator* D3D12LogicalDevice::GetUniversalViewAllocator() const noexcept {
		return m_universal_view_allocator.get();
	}

	D3D12RTVAllocator* D3D12LogicalDevice::GetRenderTargetViewAllocator() const noexcept {
		return m_render_target_view_allocator.get();
	}

	D3D12DSVAllocator* D3D12LogicalDevice::GetDepthStencilViewAllocator() const noexcept {
		return m_depth_stencil_view_allocator.get();
	}

	D3D12SamplerAllocator* D3D12LogicalDevice::GetSamplerAllocator() const noexcept {
		return m_sampler_allocator.get();
	}

	// ----------------------------------------------------------------------------
	// CreateCommandList: creates a command list and its associated allocator.
	// ----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> D3D12LogicalDevice::CreateCommandList(
		D3D12_COMMAND_LIST_TYPE type,
		std::wstring_view debug_name
	) const {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(m_impl->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> command_list;
		ThrowIfFailed(m_impl->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&command_list)));

		// Associate the command allocator with the command list so that it can be retrieved later.
		// This ensures the allocator stays alive as long as the command list is alive.
		ThrowIfFailed(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), allocator.Get()));

#if !defined(_NDEBUG)
		if (!debug_name.empty()) {
			allocator->SetName(debug_name.data());
			command_list->SetName(debug_name.data());
		}
#endif

		// Command lists are created in a recording state; we close them by default.
		ThrowIfFailed(command_list->Close());

		return command_list;
	}

	// ----------------------------------------------------------------------------
	// CreateFence: creates a fence.
	// ----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12Fence1> D3D12LogicalDevice::CreateFence(std::wstring_view debug_name) const {
		Microsoft::WRL::ComPtr<ID3D12Fence1> fence;
		ThrowIfFailed(m_impl->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		if (!debug_name.empty()) {
			fence->SetName(debug_name.data());
		}
		return fence;
	}

	// ----------------------------------------------------------------------------
	// CreateCommittedResource: creates a committed resource (buffer/texture).
	// ----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12Resource2> D3D12LogicalDevice::CreateCommittedResource(
		D3D12_HEAP_PROPERTIES const* heap_properties,
		D3D12_HEAP_FLAGS heap_flags,
		D3D12_RESOURCE_DESC const* desc,
		D3D12_RESOURCE_STATES initial_resource_state,
		D3D12_CLEAR_VALUE const* optimized_clear_value,
		std::wstring_view debug_name
	) const {
		Microsoft::WRL::ComPtr<ID3D12Resource2> resource;
		ThrowIfFailed(m_impl->CreateCommittedResource(
			heap_properties, heap_flags, desc,
			initial_resource_state, optimized_clear_value,
			IID_PPV_ARGS(&resource)));

		if (!debug_name.empty()) {
			resource->SetName(debug_name.data());
		}
		return resource;
	}

	// ----------------------------------------------------------------------------
	// CreateCommandQueue: creates command queue.
	// ----------------------------------------------------------------------------
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> D3D12LogicalDevice::CreateCommandQueue(
		D3D12_COMMAND_LIST_TYPE type,
		D3D12_COMMAND_QUEUE_PRIORITY priority,
		std::wstring_view debug_name
	) const {

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;

		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Type = type;
		queue_desc.Priority = priority;
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.NodeMask = 0;

		ThrowIfFailed(m_impl->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

	#if !defined(_NDEBUG)
		if (!debug_name.empty()) {
			command_queue->SetName(debug_name.data());
		}
	#endif

		return command_queue;		

	}

	void D3D12LogicalDevice::CreateSRV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_SHADER_RESOURCE_VIEW_DESC const& srv_desc) const noexcept {
		m_impl->CreateShaderResourceView(resource, &srv_desc, cpu_handle);
	}

	void D3D12LogicalDevice::CreateUAV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uav_desc) const noexcept {
		m_impl->CreateUnorderedAccessView(resource, nullptr, &uav_desc, cpu_handle);
	}

	void D3D12LogicalDevice::CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_CONSTANT_BUFFER_VIEW_DESC const& cbv_desc) const noexcept {
		m_impl->CreateConstantBufferView(&cbv_desc, cpu_handle);
	}

	void D3D12LogicalDevice::CreateRTV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_RENDER_TARGET_VIEW_DESC const& rtv_desc) const noexcept {
		m_impl->CreateRenderTargetView(resource, &rtv_desc, cpu_handle);
	}

	void D3D12LogicalDevice::CreateDSV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_DEPTH_STENCIL_VIEW_DESC const& dsv_desc) const noexcept {
		m_impl->CreateDepthStencilView(resource, &dsv_desc, cpu_handle);
	}

	void D3D12LogicalDevice::CreateSampler(D3D12_SAMPLER_DESC const &sampler_desc, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle) const noexcept
	{
		m_impl->CreateSampler(&sampler_desc, cpu_handle);
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12LogicalDevice::CreateRootSignature(ID3DBlob* signature_blob) const {
		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
		ThrowIfFailed(
			m_impl->CreateRootSignature(
				0u,
				signature_blob->GetBufferPointer(),
				signature_blob->GetBufferSize(),
				IID_PPV_ARGS(&root_signature)
			)
		);
		return root_signature;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12LogicalDevice::CreatePipeline(D3D12_GRAPHICS_PIPELINE_STATE_DESC const& desc) const {
		
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(
			m_impl->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso))
		);

		return pso;

	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12LogicalDevice::CreatePipeline(D3D12_COMPUTE_PIPELINE_STATE_DESC const& desc) const {
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(
			m_impl->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso))
		);

		return pso;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandSignature> D3D12LogicalDevice::GetMultiDrawSignature() const noexcept {
		return m_multidraw;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandSignature> D3D12LogicalDevice::GetMultiDrawIndexedSignature() const noexcept {
		return m_multidraw_indexed;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandSignature> D3D12LogicalDevice::GetDispatchIndirectSignature() const noexcept {
		return m_dispatch_indirect;
	}

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)