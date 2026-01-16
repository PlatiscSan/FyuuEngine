module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <wrl.h>
#include <comdef.h>
#include "pix3.h"
#include "declare_pool.h"
#endif // WIN32

module fyuu_rhi:d3d12_logical_device;
import :d3d12_physical_device;
import :d3d12_command_object;
import :boost_locale;

namespace fyuu_rhi::d3d12 {
#ifdef _WIN32

	static Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(
		plastic::utility::AnyPointer<IDXGIAdapter4> const& adapter
	) {

		Microsoft::WRL::ComPtr<ID3D12Device2> device;
		ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

		// Enable debug messages in debug mode.
#if !defined(_NDEBUG)
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue;
		if (SUCCEEDED(device.As(&info_queue))) {

			if (!PIXIsAttachedForGpuCapture()) {        // these can screw with PIX captures
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			}
			// Suppress whole categories of messages
			// uncomment to provide some
			//D3D12_MESSAGE_CATEGORY categories[] = {};

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY severities[] = {
				D3D12_MESSAGE_SEVERITY_INFO     // these don't indicate misuse of the API so we ignore them
			};

			// Suppress individual messages by their ID
			D3D12_MESSAGE_ID denied_ids[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message. Happens when clearing using a color other than the optimized clear color. If arbitrary clear colors are not desired, dont' suppress this message
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D12_INFO_QUEUE_FILTER new_filter = {};
			//new_filter.DenyList.NumCategories = _countof(Categories);
			//new_filter.DenyList.pCategoryList = Categories;
			new_filter.DenyList.NumSeverities = _countof(severities);
			new_filter.DenyList.pSeverityList = severities;
			new_filter.DenyList.NumIDs = _countof(denied_ids);
			new_filter.DenyList.pIDList = denied_ids;

			ThrowIfFailed(info_queue->PushStorageFilter(&new_filter));
		}
#endif

		return device;

	}

	static EventHandle CreateDeviceRemovedEvent() {
		if (HANDLE handle = CreateEvent(nullptr, false, false, nullptr)) {
			return std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>(handle, CloseHandle);
		}
		throw std::runtime_error("Failed to create device removed event");
	}

	static constexpr std::string_view D3D12_DRED_ALLOCATION_TYPE_to_string(D3D12_DRED_ALLOCATION_TYPE type) {
		switch (type) {
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:
			return "COMMAND_QUEUE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:
			return "COMMAND_ALLOCATOR";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:
			return "PIPELINE_STATE";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:
			return "COMMAND_LIST";
		case D3D12_DRED_ALLOCATION_TYPE_FENCE:
			return "FENCE";
		case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:
			return "DESCRIPTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_HEAP:
			return "HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:
			return "QUERY_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:
			return "COMMAND_SIGNATURE";
		case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:
			return "PIPELINE_LIBRARY";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:
			return "VIDEO_DECODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:
			return "VIDEO_PROCESSOR";
		case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:
			return "RESOURCE";
		case D3D12_DRED_ALLOCATION_TYPE_PASS:
			return "PASS";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:
			return "CRYPTOSESSION";
		case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:
			return "CRYPTOSESSIONPOLICY";
		case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION:
			return "PROTECTEDRESOURCESESSION";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:
			return "VIDEO_DECODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:
			return "COMMAND_POOL";
		case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:
			return "COMMAND_RECORDER";
		case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:
			return "STATE_OBJECT";
		case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:
			return "METACOMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:
			return "SCHEDULINGGROUP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:
			return "VIDEO_MOTION_ESTIMATOR";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP:
			return "VIDEO_MOTION_VECTOR_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND:
			return "VIDEO_EXTENSION_COMMAND";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER:
			return "VIDEO_ENCODER";
		case D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP:
			return "VIDEO_ENCODER_HEAP";
		case D3D12_DRED_ALLOCATION_TYPE_INVALID:
			return "INVALID";
		default:
			return "Unknown D3D12_DRED_ALLOCATION_TYPE";
		}
	}

	static constexpr std::string_view D3D12AutoBreadcrumbOpToString(D3D12_AUTO_BREADCRUMB_OP op) {
		switch (op) {
		case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
			return "SETMARKER";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
			return "BEGINEVENT";
		case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
			return "ENDEVENT";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
			return "DRAWINSTANCED";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
			return "DRAWINDEXEDINSTANCED";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
			return "EXECUTEINDIRECT";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
			return "DISPATCH";
		case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
			return "COPYBUFFERREGION";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
			return "COPYTEXTUREREGION";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
			return "COPYRESOURCE";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
			return "COPYTILES";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
			return "RESOLVESUBRESOURCE";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
			return "CLEARRENDERTARGETVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
			return "CLEARUNORDEREDACCESSVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
			return "CLEARDEPTHSTENCILVIEW";
		case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
			return "RESOURCEBARRIER";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
			return "EXECUTEBUNDLE";
		case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
			return "PRESENT";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
			return "RESOLVEQUERYDATA";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
			return "BEGINSUBMISSION";
		case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
			return "ENDSUBMISSION";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
			return "DECODEFRAME";
		case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
			return "PROCESSFRAMES";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
			return "ATOMICCOPYBUFFERUINT";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
			return "ATOMICCOPYBUFFERUINT64";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
			return "RESOLVESUBRESOURCEREGION";
		case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
			return "WRITEBUFFERIMMEDIATE";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
			return "DECODEFRAME1";
		case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
			return "SETPROTECTEDRESOURCESESSION";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
			return "DECODEFRAME2";
		case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
			return "PROCESSFRAMES1";
		case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
			return "BUILDRAYTRACINGACCELERATIONSTRUCTURE";
		case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
			return "EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
			return "COPYRAYTRACINGACCELERATIONSTRUCTURE";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
			return "DISPATCHRAYS";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
			return "INITIALIZEMETACOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
			return "EXECUTEMETACOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
			return "ESTIMATEMOTION";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
			return "RESOLVEMOTIONVECTORHEAP";
		case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
			return "SETPIPELINESTATE1";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
			return "INITIALIZEEXTENSIONCOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
			return "EXECUTEEXTENSIONCOMMAND";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
			return "DISPATCHMESH";
		case D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME:
			return "ENCODEFRAME";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA:
			return "RESOLVEENCODEROUTPUTMETADATA";
		default:
			return "Unknown D3D12_AUTO_BREADCRUMB_OP";
		}
	}

	static void DeviceRemovedCallback(PVOID context, BOOLEAN) {

		auto logical_device = static_cast<D3D12LogicalDevice*>(context);
		ID3D12Device2* d3d12_device = logical_device->GetRawNative();

		HRESULT reason = logical_device->GetRawNative()->GetDeviceRemovedReason();
		if (reason == S_OK) {
			return; // proper shutdown, no need to go further
		}

#if PIX_ENABLED && TDR_PIX_CAPTURE
		PIXEndCapture(FALSE);
#endif
#if !defined(_NDEBUG)

		LogCallback callback = logical_device->GetLogCallback();

		if (!callback) {
			return;
		}

		{
			_com_error error(reason, nullptr);
#if defined(_UNICODE)
			DECLARE_PMR_POOL(char, 256ull)

			std::pmr::string message = common::UTF16ToUTF8(error.ErrorMessage(), char_alloc);

			callback(LogSeverity::Fatal, message);
#else
			callback(LogSeverity::Fatal, error.ErrorMessage());
#endif // defined(_UNICODE)
		}

		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> dred;
		ThrowIfFailed(d3d12_device->QueryInterface(IID_PPV_ARGS(&dred)));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dred_auto_breadcrumbs_output;
		D3D12_DRED_PAGE_FAULT_OUTPUT dred_page_fault_output;
		ThrowIfFailed(dred->GetAutoBreadcrumbsOutput(&dred_auto_breadcrumbs_output));
		ThrowIfFailed(dred->GetPageFaultAllocationOutput(&dred_page_fault_output));

		// Custom processing of DRED data can be done here.
		// Log information to console...
		{
			auto node = dred_auto_breadcrumbs_output.pHeadAutoBreadcrumbNode;
			while (node) {
				callback(LogSeverity::Fatal, std::format("DRED Breadcrumb Data:\n\tCommand List:\t{}\n\tCommand Queue:\t{}\n", node->pCommandListDebugNameA == nullptr ? "[Unnamed CommandList]" : node->pCommandListDebugNameA, node->pCommandQueueDebugNameA == nullptr ? "[Unnamed CommandQueue]" : node->pCommandQueueDebugNameA));
				for (UINT32 i = 0; i < node->BreadcrumbCount; i++) {
					auto& command = node->pCommandHistory[i];
					auto str = D3D12AutoBreadcrumbOpToString(command);
					const char* failStr = "";
					if (i == *node->pLastBreadcrumbValue) {
						failStr = "\t\t <- failed";
					}
					callback(LogSeverity::Fatal, std::format("\t\t{}{}\n", str, failStr));
				}
				node = node->pNext;
			}
		}

		callback(LogSeverity::Fatal, std::format("DRED Page Fault Output:\n\tVirtual Address: {:X}", dred_page_fault_output.PageFaultVA));
		{
			auto node = dred_page_fault_output.pHeadExistingAllocationNode;
			while (node) {
				callback(LogSeverity::Fatal, std::format("\tDRED Page Fault Existing Allocation Node:\n\t\tObject Name: {}\n\t\tAllocation Type: {}\n", node->ObjectNameA ? node->ObjectNameA : "[Unnamed Object]", D3D12_DRED_ALLOCATION_TYPE_to_string(node->AllocationType)));
				node = node->pNext;
			}
		}
		{
			auto node = dred_page_fault_output.pHeadRecentFreedAllocationNode;
			while (node) {
				callback(LogSeverity::Fatal, std::format("\tDRED Page Fault Recent Freed Allocation Node:\n\t\tObject Name: {}\n\t\tAllocation Type: {}\n", node->ObjectNameA ? node->ObjectNameA : "[Unnamed Object]", D3D12_DRED_ALLOCATION_TYPE_to_string(node->AllocationType)));
				node = node->pNext;
			}
		}
#endif

		callback(LogSeverity::Fatal, "Device removal triggered!");

	}

	static Microsoft::WRL::ComPtr<ID3D12Fence> CreateDeviceRemovedFence(
		Microsoft::WRL::ComPtr<ID3D12Device2> const& device,
		HANDLE device_removed_event
	) {

		Microsoft::WRL::ComPtr<ID3D12Fence> device_removed_fence;
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&device_removed_fence)));
		device_removed_fence->SetEventOnCompletion(UINT64_MAX, device_removed_event);

		return device_removed_fence;

	}

	D3D12LogicalDevice::DeviceRemovedHandle D3D12LogicalDevice::RegisterDeviceRemovedHandler(
		D3D12LogicalDevice* logical_device
	) {
		HANDLE wait_handle;
		RegisterWaitForSingleObject(
			&wait_handle,
			logical_device->m_device_removed_event.get(),
			DeviceRemovedCallback,
			logical_device,			//	Pass the device as context
			INFINITE,				//	No timeout
			0						//	No flags
		);

		return D3D12LogicalDevice::DeviceRemovedHandle(wait_handle, UnregisterWait);

	}

	static Microsoft::WRL::ComPtr<D3D12MA::Allocator> CreateVideoMemoryAllocator(
		plastic::utility::AnyPointer<IDXGIAdapter4> const& physical_device,
		Microsoft::WRL::ComPtr<ID3D12Device2> const& logical_device

	) {

		Microsoft::WRL::ComPtr<D3D12MA::Allocator> alloc;

		D3D12MA::ALLOCATOR_DESC allocator_desc = {};
		allocator_desc.pDevice = logical_device.Get();
		allocator_desc.pAdapter = physical_device.Get();

		D3D12MA::CreateAllocator(&allocator_desc, &alloc);

		return alloc;

	}

	D3D12LogicalDevice::D3D12LogicalDevice(
		plastic::utility::AnyPointer<IDXGIAdapter4> const& adapter,
		LogCallback callback
	) : m_log_callback(callback),
		m_impl(CreateDevice(adapter)),
		m_device_removed_event(CreateDeviceRemovedEvent()),
		m_device_removed_fence(
			CreateDeviceRemovedFence(m_impl, m_device_removed_event.get())
		),
		m_device_removed_handler(D3D12LogicalDevice::RegisterDeviceRemovedHandler(this)),
		m_video_memory_alloc(CreateVideoMemoryAllocator(adapter, m_impl)) {
	}

	D3D12LogicalDevice::D3D12LogicalDevice(
		plastic::utility::AnyPointer<D3D12PhysicalDevice> const& physical_device
	) : D3D12LogicalDevice(physical_device->GetNative(), physical_device->GetLogCallback()) {

	}

	D3D12LogicalDevice::D3D12LogicalDevice(D3D12PhysicalDevice const& physical_device)
		: m_log_callback(physical_device.GetLogCallback()),
		m_impl(CreateDevice(physical_device.GetNative())),
		m_device_removed_event(CreateDeviceRemovedEvent()),
		m_device_removed_fence(
			CreateDeviceRemovedFence(m_impl, m_device_removed_event.get())
		),
		m_device_removed_handler(D3D12LogicalDevice::RegisterDeviceRemovedHandler(this)),
		m_video_memory_alloc(CreateVideoMemoryAllocator(physical_device.GetNative(), m_impl)) {
	}

	D3D12LogicalDevice::D3D12LogicalDevice(D3D12LogicalDevice&& other) noexcept
		: Registrable(std::move(other)),
		m_impl(std::move(other.m_impl)),
		m_device_removed_event(std::move(other.m_device_removed_event)),
		m_device_removed_fence(std::move(other.m_device_removed_fence)),
		m_device_removed_handler(std::move(other.m_device_removed_handler)) {
	}

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

#endif // _WIN32
}