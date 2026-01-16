#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(PhysicalDevice);

static fyuu_rhi::RHIInitOptions LoadFromFyuuInitOptions(FyuuInitOptions const* init_option) {

	fyuu_rhi::RHIInitOptions rhi_init_options;

	rhi_init_options.app_name = init_option->app_name;
	rhi_init_options.engine_name = "Fyuu Engine";

	rhi_init_options.app_version.variant = init_option->app_version.variant;
	rhi_init_options.app_version.major = init_option->app_version.major;
	rhi_init_options.app_version.minor = init_option->app_version.minor;
	rhi_init_options.app_version.patch = init_option->app_version.patch;

	rhi_init_options.engine_version.variant = 0;
	rhi_init_options.engine_version.major = 1;
	rhi_init_options.engine_version.minor = 0;
	rhi_init_options.engine_version.patch = 0;

	rhi_init_options.log_callback = [](fyuu_rhi::LogSeverity, std::string message) {
		std::cout << message << std::endl;
		};

	rhi_init_options.software_fallback = init_option->software_fallback;

	return rhi_init_options;

}

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreatePhysicalDevice(
	FyuuPhysicalDevice* physical_device, 
	FyuuInitOptions const* init_option, 
	FyuuAPI api
) NO_EXCEPT try {

	if (!physical_device) {
		return FyuuErrorCode_InvalidPointer;
	}

	if (!physical_device->impl) {
		physical_device->impl = new FyuuPhysicalDevice::Implementation();
	}

	fyuu_rhi::RHIInitOptions rhi_init = LoadFromFyuuInitOptions(init_option);

	if (api == FyuuAPI_PlatformDefault) {
#if defined(_WIN32)
		api = FyuuAPI_DirectX12;
#elif defined(__linux__) || defined(__ANDROID__)
		api = FyuuAPI_Vulkan;
#endif
	}

	switch (api) {
	case FyuuAPI_Vulkan:
		physical_device->impl->rhi_obj.emplace<fyuu_rhi::vulkan::VulkanPhysicalDevice>(rhi_init);
		return FyuuErrorCode::FyuuErrorCode_Success;

#if defined(_WIN32)	
	case FyuuAPI_DirectX12:
		physical_device->impl->rhi_obj.emplace<fyuu_rhi::d3d12::D3D12PhysicalDevice>(rhi_init);
		return FyuuErrorCode::FyuuErrorCode_Success;

#endif // defined(_WIN32)

	default:
		return FyuuErrorCode::FyuuErrorCode_Unsupported;
	}

}
CATCH_COMMON_EXCEPTION(physical_device)


DECLARE_FYUU_DESTROY(PhysicalDevice)