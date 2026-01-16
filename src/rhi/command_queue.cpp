#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(CommandQueue)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateCommandQueue(
	FyuuCommandQueue* queue,
	FyuuLogicalDevice* logical_device,
	FyuuCommandObjectType queue_type,
	FyuuQueuePriority priority
) NO_EXCEPT try {

	if (!queue || !logical_device) {
		return FyuuErrorCode_InvalidPointer;
	}

	if (!queue->impl) {
		queue->impl = new FyuuCommandQueue::Implementation();
	}

	auto logical_device_impl = reinterpret_cast<fyuu_rhi::LogicalDevice*>(logical_device->impl);

	return std::visit(
		[&rhi_obj = queue->impl->rhi_obj, queue_type, priority](auto&& logical_device) {
			using LogicalDevice = std::decay_t<decltype(logical_device)>;
			if constexpr (!std::same_as<std::monostate, LogicalDevice>) {

				static constexpr std::size_t index
					= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(logical_device_impl)>, LogicalDevice>;

				rhi_obj.emplace<index>(
					logical_device, 
					static_cast<fyuu_rhi::CommandObjectType>(queue_type),
					static_cast<fyuu_rhi::QueuePriority>(priority)
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
CATCH_COMMON_EXCEPTION(queue)

DECLARE_FYUU_DESTROY(CommandQueue);
