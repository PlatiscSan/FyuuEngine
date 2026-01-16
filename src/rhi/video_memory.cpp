#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(VideoMemory);

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuAllocateVideoMemory(
	FyuuVideoMemory* video_memory,
	FyuuLogicalDevice* logical_device,
	size_t size_in_bytes,
	FyuuVideoMemoryUsage usage,
	FyuuVideoMemoryType type
) NO_EXCEPT try {

	if (!video_memory || !logical_device) {
		return FyuuErrorCode::FyuuErrorCode_InvalidPointer;
	}

	if (size_in_bytes == 0u) {
		return FyuuErrorCode::FyuuErrorCode_InvalidParameter;
	}

	if (!video_memory->impl) {
		video_memory->impl = new FyuuVideoMemory::Implementation();
	}

	auto logical_device_impl = reinterpret_cast<fyuu_rhi::LogicalDevice*>(logical_device->impl);

	return std::visit(
		[&rhi_obj = video_memory->impl->rhi_obj, size_in_bytes, usage, type]
		(auto&& logical_device) {
			using LogicalDevice = std::decay_t<decltype(logical_device)>;
			if constexpr (!std::same_as<std::monostate, LogicalDevice>) {

				static constexpr std::size_t index
					= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(logical_device_impl)>, LogicalDevice>;

				rhi_obj.emplace<index>(
					logical_device,
					size_in_bytes, 
					static_cast<fyuu_rhi::VideoMemoryUsage>(usage),
					static_cast<fyuu_rhi::VideoMemoryType>(type)
				);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}
		},
		*logical_device_impl
	);


}
CATCH_COMMON_EXCEPTION(video_memory)

EXTERN_C DLL_API void DLL_CALL FyuuFreeVideoMemory(FyuuVideoMemory* obj) NO_EXCEPT {
	delete obj->impl;
	obj->impl = nullptr;
}

