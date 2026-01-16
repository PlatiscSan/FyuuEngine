#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(Resource);

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateResource(
	FyuuResource* resource,
	FyuuVideoMemory* video_memory,
	size_t width,
	size_t height,
	size_t depth,
	FyuuResourceType type
) NO_EXCEPT try {

	if (!resource || !video_memory) {
		return FyuuErrorCode::FyuuErrorCode_InvalidPointer;
	}

	if (width == 0u || height == 0u || depth == 0u) {
		return FyuuErrorCode::FyuuErrorCode_InvalidParameter;
	}

	if (!resource->impl) {
		resource->impl = new FyuuResource::Implementation();
	}

	auto video_memory_impl =
		reinterpret_cast<fyuu_rhi::VideoMemory*>(video_memory->impl);

	return std::visit(
		[&rhi_obj = resource->impl->rhi_obj, width, height, depth, type]
		(auto&& video_memory) {
			using VideoMemory = std::decay_t<decltype(video_memory)>;
			if constexpr (!std::same_as<std::monostate, VideoMemory>) {

				static constexpr std::size_t index
					= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(video_memory_impl)>, VideoMemory>;

				rhi_obj.emplace<index>(
					video_memory,
					width,
					height,
					depth,
					static_cast<fyuu_rhi::ResourceType>(type)
				);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}
		},
		* video_memory_impl
	);

}
CATCH_COMMON_EXCEPTION(resource)

DECLARE_FYUU_DESTROY(Resource)


EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuSetBufferData(
	FyuuResource* resource,
	FyuuLogicalDevice* logical_device,
	FyuuCommandQueue* copy_queue,
	void const* data,
	size_t size_in_bytes,
	size_t offset
) NO_EXCEPT try {

	if (!resource || !logical_device || !copy_queue || !data) {
		return FyuuErrorCode::FyuuErrorCode_InvalidPointer;
	}

	if (size_in_bytes == 0u) {
		return FyuuErrorCode::FyuuErrorCode_InvalidParameter;
	}

	auto logical_device_impl
		= reinterpret_cast<fyuu_rhi::LogicalDevice*>(logical_device->impl);

	auto copy_queue_impl
		= reinterpret_cast<fyuu_rhi::CommandQueue*>(copy_queue->impl);

	std::span<std::byte const> raw(static_cast<std::byte const*>(data), size_in_bytes);

	return std::visit(
		[raw, offset]
		(auto&& resource, auto&& logical_device, auto&& copy_queue) {

			using Resource = std::decay_t<decltype(resource)>;
			using LogicalDevice = std::decay_t<decltype(logical_device)>;
			using CommandQueue = std::decay_t<decltype(copy_queue)>;

			static constexpr std::size_t resource_index
				= fyuu_rhi::RHITypeIndex<fyuu_rhi::Resource, Resource>;

			static constexpr std::size_t logical_device_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(logical_device_impl)>, LogicalDevice>;

			static constexpr std::size_t copy_queue_index
				= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(copy_queue_impl)>, CommandQueue>;


			static constexpr bool is_same_api =
				resource_index == logical_device_index &&
				logical_device_index == copy_queue_index;

			static constexpr bool is_valid =
				!std::same_as<std::monostate, Resource> &&
				!std::same_as<std::monostate, LogicalDevice> &&
				!std::same_as<std::monostate, CommandQueue>&&
				is_same_api;

			if constexpr (is_valid) {
				resource.SetBufferData(logical_device, copy_queue, raw, offset);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}

		},
		resource->impl->rhi_obj,
		*logical_device_impl,
		*copy_queue_impl
	);

}
catch (std::exception const& ex) {
	fyuu_engine::rhi::SetLastRHIErrorMessage(ex.what()); 
	return FyuuErrorCode::FyuuErrorCode_UnknownError;
}