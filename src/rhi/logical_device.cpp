#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(LogicalDevice)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateLogicalDevice(
	FyuuLogicalDevice* logical_device,
	FyuuPhysicalDevice* physical_device
) NO_EXCEPT try {

	if (!logical_device || !physical_device) {
		return FyuuErrorCode_InvalidPointer;
	}

	if (!logical_device->impl) {
		logical_device->impl = new FyuuLogicalDevice::Implementation();
	}

	auto physical_device_impl = reinterpret_cast<fyuu_rhi::PhysicalDevice*>(physical_device->impl);

	return std::visit(
		[&rhi_obj = logical_device->impl->rhi_obj](auto&& physical_device) {
			using PhysicalDevice = std::decay_t<decltype(physical_device)>;
			if constexpr (!std::same_as<std::monostate, PhysicalDevice>) {

				static constexpr std::size_t index
					= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(physical_device_impl)>, PhysicalDevice>;

				rhi_obj.emplace<index>(physical_device);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}
		},
		*physical_device_impl
	);

}
CATCH_COMMON_EXCEPTION(logical_device)

DECLARE_FYUU_DESTROY(LogicalDevice)