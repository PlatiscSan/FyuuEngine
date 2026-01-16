#pragma once

#include "fyuu_class.h"
#include "fyuu_enum.h"

#if defined(__cplusplus)
#include <cstdint>
#include <memory>
#include <string_view>
#include <span>
#include <stdexcept>
#else
#include <stdint.h>
#include <stdbool.h>
#endif // defined(__cplusplus)

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <wayland-client-core.h>
#include <wayland-util.h>
#elif defined(__ANDROID__)
#include <android/native_window.h>
#endif // defined(_WIN32)

DECLARE_FYUU_ENUM(API, PlatformDefault, Vulkan, DirectX12, Metal, OpenGL)
DECLARE_FYUU_ENUM(ErrorCode, Success, Unsupported, BadAllocation, InvalidPointer, InvalidParameter, SystemError, UnknownError)
DECLARE_FYUU_ENUM(CommandObjectType, AllCommands, Compute, Copy)
DECLARE_FYUU_ENUM(QueuePriority, High, Medium, Low)
DECLARE_FYUU_ENUM(VideoMemoryType, DeviceLocal, HostVisible, DeviceReadback)

// TODO: more shader language supports
DECLARE_FYUU_ENUM(ShaderLanguage, HLSL, GLSL)

// TODO: more shader stages
DECLARE_FYUU_ENUM(ShaderStage, Vertex, Fragment)

// TODO: more memory usages
DECLARE_FYUU_ENUM(VideoMemoryUsage, VertexBuffer, IndexBuffer, Texture1D, Texture2D, Texture3D)

// TODO: more memory usages
DECLARE_FYUU_ENUM(ResourceType, VertexBuffer, IndexBuffer, Texture1D, Texture2D, Texture3D)

// TODO: more flags
DECLARE_FYUU_FLAG_AUTO(SurfaceFlag, Wayland)

DECLARE_FYUU_CLASS_C(FyuuPhysicalDevice)
DECLARE_FYUU_CLASS_C(FyuuLogicalDevice)
DECLARE_FYUU_CLASS_C(FyuuCommandQueue)
DECLARE_FYUU_CLASS_C(FyuuSurface)
DECLARE_FYUU_CLASS_C(FyuuSwapChain)
DECLARE_FYUU_CLASS_C(FyuuShaderLibrary)
DECLARE_FYUU_CLASS_C(FyuuVideoMemory)
DECLARE_FYUU_CLASS_C(FyuuResource)
DECLARE_FYUU_CLASS_C(FyuuPipelineLayout)
DECLARE_FYUU_CLASS_C(FyuuRenderPass)
DECLARE_FYUU_CLASS_C(FyuuRenderingPipeline)
DECLARE_FYUU_CLASS_C(FyuuComputingPipeline)
DECLARE_FYUU_CLASS_C(FyuuSampler)
DECLARE_FYUU_CLASS_C(FyuuView)
DECLARE_FYUU_CLASS_C(FyuuFence)

typedef struct FyuuInitOptions {
	/// @brief Name of app.
	char const* app_name;

	typedef struct Version {
		uint8_t variant;
		uint8_t major;
		uint8_t minor;
		uint8_t patch;
	} Version;

	/// @brief The version number of app.
	Version app_version;

	bool software_fallback;

} FyuuInitOptions;

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreatePhysicalDevice(FyuuPhysicalDevice* physical_device, FyuuInitOptions const* init_options, FyuuAPI api) NO_EXCEPT;
DECLARE_FYUU_DESTROY(PhysicalDevice)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateLogicalDevice(FyuuLogicalDevice* logical_device, FyuuPhysicalDevice* physical_device) NO_EXCEPT;
DECLARE_FYUU_DESTROY(LogicalDevice)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateCommandQueue(
	FyuuCommandQueue* queue, 
	FyuuLogicalDevice* logical_device,
	FyuuCommandObjectType queue_type,
	FyuuQueuePriority priority
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(CommandQueue)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateSurface(
	FyuuSurface* surface,
	FyuuPhysicalDevice* physical_device,
	uint32_t width,
	uint32_t height,
	FyuuSurfaceFlag flags
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(Surface)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuSetSurfaceTitle(
	FyuuSurface* surface,
#if defined(WIN32)
	TCHAR const* title
#else
	char const* title
#endif // defined(WIN32)
) NO_EXCEPT;

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateSwapChain(
	FyuuSwapChain* swap_chain,
	FyuuPhysicalDevice* physical_device,
	FyuuLogicalDevice* logical_device,
	FyuuCommandQueue* present_queue,
	FyuuSurface* surface,
	uint32_t buffer_size
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(SwapChain)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateShaderLibrary(
	FyuuShaderLibrary* shader_library,
	FyuuLogicalDevice* logical_device,
	char const* source,
	size_t source_size_in_bytes,
	FyuuShaderStage shader_stage,
	FyuuShaderLanguage shader_language
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(ShaderLibrary)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuAllocateVideoMemory(
	FyuuVideoMemory* video_memory,
	FyuuLogicalDevice* logical_device,
	size_t size_in_bytes,
	FyuuVideoMemoryUsage usage,
	FyuuVideoMemoryType type
) NO_EXCEPT;
EXTERN_C DLL_API void DLL_CALL FyuuFreeVideoMemory(FyuuVideoMemory* obj) NO_EXCEPT;

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateResource(
	FyuuResource* resource,
	FyuuVideoMemory* video_memory,
	size_t width, 
	size_t height, 
	size_t depth, 
	FyuuResourceType type
) NO_EXCEPT;
DECLARE_FYUU_DESTROY(Resource)

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuSetBufferData(
	FyuuResource* resource,
	FyuuLogicalDevice* logical_device,
	FyuuCommandQueue* copy_queue,
	void const* data,
	size_t size_in_bytes,
	size_t offset
) NO_EXCEPT;

EXTERN_C DLL_API char const* DLL_CALL FyuuGetLastRHIErrorMessage() NO_EXCEPT;

#if defined(__cplusplus)
namespace fyuu_engine {

	struct InitOptions : public FyuuInitOptions {};

	DECLARE_FYUU_CLASS(PhysicalDevice) {
		PhysicalDevice(InitOptions const& init_options, API api) {
			auto result = static_cast<ErrorCode>(FyuuCreatePhysicalDevice(this, &init_options, static_cast<FyuuAPI>(api)));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(PhysicalDevice)

	};

	DECLARE_FYUU_CLASS(LogicalDevice) {
		LogicalDevice(PhysicalDevice& physical_device) {
			auto result = static_cast<ErrorCode>(FyuuCreateLogicalDevice(this, &physical_device));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(LogicalDevice)

	};

	DECLARE_FYUU_CLASS(CommandQueue) {
		CommandQueue(LogicalDevice& logical_device, CommandObjectType queue_type, QueuePriority priority = QueuePriority::High) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCreateCommandQueue(
						this,
						&logical_device,
						static_cast<FyuuCommandObjectType>(queue_type),
						static_cast<FyuuQueuePriority>(priority)
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(CommandQueue)

	};

	DECLARE_FYUU_CLASS(Surface) {
		Surface(
			PhysicalDevice& physical_device,
			uint32_t width,
			uint32_t height,
			SurfaceFlag flags
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCreateSurface(
						this,
						&physical_device,
						width,
						height,
						static_cast<FyuuSurfaceFlag>(flags)
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		void SetTitle(
#if defined(_WIN32)
			std::span<TCHAR const> title
#else
			std::string_view title
#endif // defined(_WIN32)
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuSetSurfaceTitle(
						this,
						title.data()
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(Surface)

	};

	DECLARE_FYUU_CLASS(SwapChain) {
		SwapChain(
			PhysicalDevice& physical_device,
			LogicalDevice& logical_device,
			CommandQueue& present_queue,
			Surface& surface,
			std::uint32_t buffer_size = 3u
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCreateSwapChain(
						this,
						&physical_device,
						&logical_device,
						&present_queue,
						&surface,
						buffer_size
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(SwapChain)

	};

	DECLARE_FYUU_CLASS(ShaderLibrary) {
		ShaderLibrary(
			LogicalDevice & logical_device,
			std::string_view source,
			ShaderStage shader_stage,
			ShaderLanguage shader_language
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCreateShaderLibrary(
						this,
						&logical_device,
						source.data(),
						source.size(),
						static_cast<FyuuShaderStage>(shader_stage),
						static_cast<FyuuShaderLanguage>(shader_language)
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(ShaderLibrary)

	};

	DECLARE_FYUU_CLASS(VideoMemory) {
		VideoMemory(
			LogicalDevice& logical_device,
			std::size_t size_in_bytes,
			VideoMemoryUsage usage,
			VideoMemoryType type = VideoMemoryType::DeviceLocal
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuAllocateVideoMemory(
						this,
						&logical_device,
						size_in_bytes,
						static_cast<FyuuVideoMemoryUsage>(usage),
						static_cast<FyuuVideoMemoryType>(type)
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		VideoMemory(VideoMemory&& other) noexcept {
			impl = std::exchange(other.impl, nullptr);
		} 

		VideoMemory& operator=(VideoMemory&& other) noexcept {

			if (this != &other) {
				FyuuFreeVideoMemory(this); 
				impl = std::exchange(other.impl, nullptr);
			} 

			return *this;

		} 

		~VideoMemory() noexcept {
			FyuuFreeVideoMemory(this);
		} 

	};

	DECLARE_FYUU_CLASS(Resource) {
		Resource(
			VideoMemory& video_memory,
			std::size_t width,
			std::size_t height,
			std::size_t depth,
			ResourceType type
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuCreateResource(
						this,
						&video_memory,
						width,
						height,
						depth,
						static_cast<FyuuResourceType>(type)
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		void SetBufferData(
			LogicalDevice& logical_device,
			CommandQueue& copy_queue,
			std::span<std::byte const> raw,
			std::size_t offset = 0
		) {
			auto result
				= static_cast<ErrorCode>(
					FyuuSetBufferData(
						this,
						&logical_device,
						&copy_queue,
						raw.data(),
						raw.size(),
						offset
					));
			if (result != ErrorCode::Success) {
				throw std::runtime_error(FyuuGetLastRHIErrorMessage());
			}
		}

		FYUU_CLASS_COMMON_PARTS(Resource)

	};

}
#endif // defined(__cplusplus)
