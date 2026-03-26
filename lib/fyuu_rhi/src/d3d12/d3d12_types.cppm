/* d3d12_types.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <type_traits>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "boost.hpp"
export module fyuu_rhi:d3d12_types;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;

namespace fyuu_rhi::d3d12 {
	export using EventHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>;
	export EventHandle CreateEventHandle() {
		if (HANDLE event_handle = CreateEvent(nullptr, false, false, nullptr)) {
			return EventHandle(event_handle, CloseHandle);
		}
		throw boost::system::system_error(
			GetLastError(),
			boost::system::system_category()
		);
	}

	export D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(CommandObjectType type) {
		switch (type) {
		case fyuu_rhi::CommandObjectType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;			
		case fyuu_rhi::CommandObjectType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case fyuu_rhi::CommandObjectType::AllCommands:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		default:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}
	}

	export DXGI_FORMAT DetermineFormat(ResourceFlags flags) noexcept {

		if (HasConflictingFlags(flags, ResourceFlags::AllFormats)) {
			return DXGI_FORMAT_UNKNOWN;
		}
	
		if ((flags & ResourceFlags::BGRA8_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		if ((flags & ResourceFlags::RGBA8_Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8B8A8_UINT;
		if ((flags & ResourceFlags::RGBA8_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		if ((flags & ResourceFlags::RGBA16_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		if ((flags & ResourceFlags::RGBA16_Snorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16G16B16A16_SNORM;
		if ((flags & ResourceFlags::RGBA16_Sfloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		if ((flags & ResourceFlags::RGBA32_Sfloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		if ((flags & ResourceFlags::R8_Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8_UINT;
		if ((flags & ResourceFlags::R16_Float) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16_FLOAT;
		if ((flags & ResourceFlags::R32_Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R32_UINT;
		if ((flags & ResourceFlags::R32_Float) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R32_FLOAT;
		if ((flags & ResourceFlags::BC1_RGB_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC1_UNORM;
		if ((flags & ResourceFlags::BC1_RGB_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		if ((flags & ResourceFlags::BC1_RGBA_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC1_UNORM;
		if ((flags & ResourceFlags::BC1_RGBA_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		if ((flags & ResourceFlags::BC2_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC2_UNORM;
		if ((flags & ResourceFlags::BC2_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		if ((flags & ResourceFlags::BC3_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC3_UNORM;
		if ((flags & ResourceFlags::BC3_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		if ((flags & ResourceFlags::BC4_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC4_UNORM;
		if ((flags & ResourceFlags::BC4_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC4_UNORM;
		if ((flags & ResourceFlags::BC5_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC5_UNORM;
		if ((flags & ResourceFlags::BC5_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC5_UNORM;
		if ((flags & ResourceFlags::D32SFloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_D32_FLOAT;
		if ((flags & ResourceFlags::D24UnormS8Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		if ((flags & ResourceFlags::R8_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8_UNORM;
		if ((flags & ResourceFlags::R8_Snorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8_SNORM;
		if ((flags & ResourceFlags::RG8_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8_UNORM;
		if ((flags & ResourceFlags::RG8_Snorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8_SNORM;
		if ((flags & ResourceFlags::R16_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16_UNORM;
		if ((flags & ResourceFlags::R16_Snorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16_SNORM;
		if ((flags & ResourceFlags::RG16_Sfloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16G16_FLOAT;
		if ((flags & ResourceFlags::R32_Sint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R32_SINT;
		if ((flags & ResourceFlags::RG32_Sfloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R32G32_FLOAT;
		if ((flags & ResourceFlags::RGB10A2_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		if ((flags & ResourceFlags::R11G11B10_UFloat) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R11G11B10_FLOAT;
		if ((flags & ResourceFlags::RGBA8_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		if ((flags & ResourceFlags::BGRA8_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		if ((flags & ResourceFlags::RGBA8_Snorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		if ((flags & ResourceFlags::RGBA8_Sint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R8G8B8A8_SINT;
		if ((flags & ResourceFlags::R16_Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_R16_UINT;
		if ((flags & ResourceFlags::D16_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_D16_UNORM;
		if ((flags & ResourceFlags::D32FloatS8Uint) != ResourceFlags::Unknown)
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		if ((flags & ResourceFlags::BC6H_UF16) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC6H_UF16;
		if ((flags & ResourceFlags::BC6H_SF16) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC6H_SF16;
		if ((flags & ResourceFlags::BC7_Unorm) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC7_UNORM;
		if ((flags & ResourceFlags::BC7_SRGB) != ResourceFlags::Unknown)
			return DXGI_FORMAT_BC7_UNORM_SRGB;
	
		return DXGI_FORMAT_UNKNOWN;
	}

	#include <cstddef>

	// Assumes DXGI_FORMAT is defined in the current scope (e.g., via <dxgiformat.h>)
	
	export constexpr std::size_t DXGIFormatSize(DXGI_FORMAT format) noexcept {
		switch (format) {
		// Unknown
		case DXGI_FORMAT_UNKNOWN:
			return 0;

		// 16 bytes (4 components, 32-bit each)
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;

		// 12 bytes (3 components, 32-bit each)
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;

		// 8 bytes (4 components, 16-bit each)
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8;

		// 8 bytes (2 components, 32-bit each)
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8;

		// 8 bytes (depth/stencil formats with 32+8+24 bits)
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return 8;

		// 4 bytes (10:10:10:2, 11:11:10, 8:8:8:8, 16:16, 32-bit, 24:8, etc.)
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:      // packed 10-bit, 4 bytes per 2 pixels? but treat as 4 for simplicity
			return 4;

		// 2 bytes (8:8, 16-bit, 5:6:5, 5:5:5:1, 4:4:4:4, etc.)
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
		case DXGI_FORMAT_YUY2:      // 4 bytes per 2 pixels = 2 bytes per pixel
		case DXGI_FORMAT_A8P8:      // 2 bytes (alpha + palette)
			return 2;

		// 1 byte (8-bit, A8, etc.)
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_AI44:      // 4-bit alpha + 4-bit intensity, but stored as 1 byte
		case DXGI_FORMAT_IA44:      // 4-bit intensity + 4-bit alpha
		case DXGI_FORMAT_P8:        // 8-bit palette index
			return 1;

		// BC1 (8 bytes per 4x4 block)
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return 8;

		// BC2 (16 bytes per 4x4 block)
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return 16;

		// BC3 (16 bytes per 4x4 block)
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return 16;

		// BC4 (8 bytes per 4x4 block)
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 8;

		// BC5 (16 bytes per 4x4 block)
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return 16;

		// BC6H (16 bytes per 4x4 block)
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
			return 16;

		// BC7 (16 bytes per 4x4 block)
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16;

		// Formats with non‑integral or planar layout – return 0 to indicate unsupported.
		case DXGI_FORMAT_R1_UNORM:
		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
		case DXGI_FORMAT_NV11:
		case DXGI_FORMAT_P208:
		case DXGI_FORMAT_V208:
		case DXGI_FORMAT_V408:
		case DXGI_FORMAT_Y416:      // 8 bytes per pixel, but planar? return 0 for safety
		default:
			return 0;
		}
	}

}
#endif // defined(_WIN32)