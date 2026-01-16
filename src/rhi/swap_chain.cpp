#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(SwapChain);

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateSwapChain(
	FyuuSwapChain* swap_chain,
	FyuuPhysicalDevice* physical_device,
	FyuuLogicalDevice* logical_device,
	FyuuCommandQueue* present_queue,
	FyuuSurface* surface,
	uint32_t buffer_size
) NO_EXCEPT try {

	if (!swap_chain || !physical_device || !logical_device || !present_queue || !surface) {
		return FyuuErrorCode_InvalidPointer;
	}

	if (!swap_chain->impl) {
		swap_chain->impl = new FyuuSwapChain::Implementation();
	}

	auto physical_device_impl 
		= reinterpret_cast<fyuu_rhi::PhysicalDevice*>(physical_device->impl);

	auto logical_device_impl
		= reinterpret_cast<fyuu_rhi::LogicalDevice*>(logical_device->impl);

	auto present_queue_impl
		= reinterpret_cast<fyuu_rhi::CommandQueue*>(present_queue->impl);

	auto surface_impl
		= reinterpret_cast<fyuu_rhi::Surface*>(surface->impl);

	return std::visit(
		[buffer_size, &rhi_obj = swap_chain->impl->rhi_obj]
		(auto&& physical_device, auto&& logical_device, auto&& present_queue, auto&& surface) {

			using PhysicalDevice = std::decay_t<decltype(physical_device)>;
			using LogicalDevice = std::decay_t<decltype(logical_device)>;
			using CommandQueue = std::decay_t<decltype(present_queue)>;
			using Surface = std::decay_t<decltype(surface)>;

			static constexpr std::size_t physical_device_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(physical_device_impl)>, PhysicalDevice>;

			static constexpr std::size_t logical_device_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(logical_device_impl)>, LogicalDevice>;

			static constexpr std::size_t present_queue_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(present_queue_impl)>, CommandQueue>;

			static constexpr std::size_t surface_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(surface_impl)>, Surface>;

			static constexpr bool is_same_api =
				physical_device_index == logical_device_index &&
				logical_device_index == surface_index &&
				surface_index == present_queue_index;

			static constexpr bool is_valid =
				!std::same_as<std::monostate, PhysicalDevice> &&
				!std::same_as<std::monostate, LogicalDevice> &&
				!std::same_as<std::monostate, Surface> &&
				!std::same_as<std::monostate, CommandQueue>&&
				is_same_api;

			if constexpr (is_valid) {
				rhi_obj.emplace<physical_device_index>(
					physical_device,
					logical_device,
					present_queue,
					surface,
					buffer_size
				);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}

		},
		*physical_device_impl,
		*logical_device_impl,
		*present_queue_impl,
		*surface_impl
	);

}
CATCH_COMMON_EXCEPTION(swap_chain)

DECLARE_FYUU_DESTROY(SwapChain)
