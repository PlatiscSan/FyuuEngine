module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <variant>
#endif
#include "glad.hpp"
export module fyuu_rhi:opengl_types;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :enums;

namespace fyuu_rhi::opengl {

	export std::pair<GLenum, GLenum> DetermineFormat(ResourceFlags const flags) noexcept {
        // Handle conflicts or unknown by returning a safe default (RGBA8_Unorm)
        if (HasConflictingFlags(flags, ResourceFlags::AllFormats) ||
            (flags & ResourceFlags::AllFormats) == ResourceFlags::Unknown) {
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        }

        // RGBA8_Uint
        if ((flags & ResourceFlags::RGBA8_Uint) != ResourceFlags::Unknown)
            return { GL_RGBA_INTEGER, GL_UNSIGNED_BYTE };
        // RGBA8_Unorm
        if ((flags & ResourceFlags::RGBA8_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        // RGBA16_Unorm
        if ((flags & ResourceFlags::RGBA16_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_SHORT };
        // RGBA16_Snorm
        if ((flags & ResourceFlags::RGBA16_Snorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_SHORT };
        // RGBA16_Sfloat
        if ((flags & ResourceFlags::RGBA16_Sfloat) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_HALF_FLOAT };
        // RGBA32_Sfloat
        if ((flags & ResourceFlags::RGBA32_Sfloat) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_FLOAT };
        // R8_Uint
        if ((flags & ResourceFlags::R8_Uint) != ResourceFlags::Unknown)
            return { GL_RED_INTEGER, GL_UNSIGNED_BYTE };
        // R16_Float
        if ((flags & ResourceFlags::R16_Float) != ResourceFlags::Unknown)
            return { GL_RED, GL_HALF_FLOAT };
        // R32_Uint
        if ((flags & ResourceFlags::R32_Uint) != ResourceFlags::Unknown)
            return { GL_RED_INTEGER, GL_UNSIGNED_INT };
        // R32_Float
        if ((flags & ResourceFlags::R32_Float) != ResourceFlags::Unknown)
            return { GL_RED, GL_FLOAT };

        // Compressed formats: return a placeholder (they are not uploaded with format/type)
        // The actual compressed texture upload uses glCompressedTexImage2D with internal format only.
        if ((flags & ResourceFlags::BC1_RGB_Unorm) != ResourceFlags::Unknown)
            return { GL_RGB, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC1_RGB_SRGB) != ResourceFlags::Unknown)
            return { GL_RGB, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC1_RGBA_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC1_RGBA_SRGB) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC2_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC2_SRGB) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC3_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC3_SRGB) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC4_Unorm) != ResourceFlags::Unknown)
            return { GL_RED, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC4_SRGB) != ResourceFlags::Unknown)
            return { GL_RED, GL_BYTE };
        if ((flags & ResourceFlags::BC5_Unorm) != ResourceFlags::Unknown)
            return { GL_RG, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC5_SRGB) != ResourceFlags::Unknown)
            return { GL_RG, GL_BYTE };
        if ((flags & ResourceFlags::BC6H_UF16) != ResourceFlags::Unknown)
            return { GL_RGB, GL_UNSIGNED_BYTE };  // placeholder
        if ((flags & ResourceFlags::BC6H_SF16) != ResourceFlags::Unknown)
            return { GL_RGB, GL_BYTE };           // placeholder
        if ((flags & ResourceFlags::BC7_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BC7_SRGB) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };

        // Depth/stencil formats
        if ((flags & ResourceFlags::D32SFloat) != ResourceFlags::Unknown)
            return { GL_DEPTH_COMPONENT, GL_FLOAT };
        if ((flags & ResourceFlags::D24UnormS8Uint) != ResourceFlags::Unknown)
            return { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 };
        if ((flags & ResourceFlags::D16_Unorm) != ResourceFlags::Unknown)
            return { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT };
        if ((flags & ResourceFlags::D32FloatS8Uint) != ResourceFlags::Unknown)
            return { GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV };

        // Other unorm/snorm formats
        if ((flags & ResourceFlags::R8_Unorm) != ResourceFlags::Unknown)
            return { GL_RED, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::R8_Snorm) != ResourceFlags::Unknown)
            return { GL_RED, GL_BYTE };
        if ((flags & ResourceFlags::RG8_Unorm) != ResourceFlags::Unknown)
            return { GL_RG, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::RG8_Snorm) != ResourceFlags::Unknown)
            return { GL_RG, GL_BYTE };
        if ((flags & ResourceFlags::R16_Unorm) != ResourceFlags::Unknown)
            return { GL_RED, GL_UNSIGNED_SHORT };
        if ((flags & ResourceFlags::R16_Snorm) != ResourceFlags::Unknown)
            return { GL_RED, GL_SHORT };
        if ((flags & ResourceFlags::RG16_Sfloat) != ResourceFlags::Unknown)
            return { GL_RG, GL_HALF_FLOAT };
        if ((flags & ResourceFlags::R32_Sint) != ResourceFlags::Unknown)
            return { GL_RED_INTEGER, GL_INT };
        if ((flags & ResourceFlags::RG32_Sfloat) != ResourceFlags::Unknown)
            return { GL_RG, GL_FLOAT };
        if ((flags & ResourceFlags::RGB10A2_Unorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV };
        if ((flags & ResourceFlags::R11G11B10_UFloat) != ResourceFlags::Unknown)
            return { GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV };
        if ((flags & ResourceFlags::RGBA8_SRGB) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::BGRA8_SRGB) != ResourceFlags::Unknown)
            return { GL_BGRA, GL_UNSIGNED_BYTE };
        if ((flags & ResourceFlags::RGBA8_Snorm) != ResourceFlags::Unknown)
            return { GL_RGBA, GL_BYTE };
        if ((flags & ResourceFlags::RGBA8_Sint) != ResourceFlags::Unknown)
            return { GL_RGBA_INTEGER, GL_BYTE };
        if ((flags & ResourceFlags::R16_Uint) != ResourceFlags::Unknown)
            return { GL_RED_INTEGER, GL_UNSIGNED_SHORT };

        // Fallback
        return { GL_RGBA, GL_UNSIGNED_BYTE };
    }

	export GLenum DetermineInternalFormat(ResourceFlags const flags) noexcept {

		if (HasConflictingFlags(flags, ResourceFlags::AllFormats)) {
			return GL_RGBA8;
		}
	
		if ((flags & ResourceFlags::RGBA8_Uint) != ResourceFlags::Unknown)
			return GL_RGBA8UI;
		if ((flags & ResourceFlags::RGBA8_Unorm) != ResourceFlags::Unknown)
			return GL_RGBA8;
		if ((flags & ResourceFlags::RGBA16_Unorm) != ResourceFlags::Unknown)
			return GL_RGBA16;
		if ((flags & ResourceFlags::RGBA16_Snorm) != ResourceFlags::Unknown)
			return GL_RGBA16_SNORM;
		if ((flags & ResourceFlags::RGBA16_Sfloat) != ResourceFlags::Unknown)
			return GL_RGBA16F;
		if ((flags & ResourceFlags::RGBA32_Sfloat) != ResourceFlags::Unknown)
			return GL_RGBA32F;
		if ((flags & ResourceFlags::R8_Uint) != ResourceFlags::Unknown)
			return GL_R8UI;
		if ((flags & ResourceFlags::R16_Float) != ResourceFlags::Unknown)
			return GL_R16F;
		if ((flags & ResourceFlags::R32_Uint) != ResourceFlags::Unknown)
			return GL_R32UI;
		if ((flags & ResourceFlags::R32_Float) != ResourceFlags::Unknown)
			return GL_R32F;
		if ((flags & ResourceFlags::BC1_RGB_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		if ((flags & ResourceFlags::BC1_RGB_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
		if ((flags & ResourceFlags::BC1_RGBA_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		if ((flags & ResourceFlags::BC1_RGBA_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		if ((flags & ResourceFlags::BC2_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		if ((flags & ResourceFlags::BC2_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		if ((flags & ResourceFlags::BC3_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		if ((flags & ResourceFlags::BC3_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		if ((flags & ResourceFlags::BC4_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RED_RGTC1;
		if ((flags & ResourceFlags::BC4_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SIGNED_RED_RGTC1;
		if ((flags & ResourceFlags::BC5_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RG_RGTC2;
		if ((flags & ResourceFlags::BC5_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SIGNED_RG_RGTC2;
		if ((flags & ResourceFlags::D32SFloat) != ResourceFlags::Unknown)
			return GL_DEPTH_COMPONENT32F;
		if ((flags & ResourceFlags::D24UnormS8Uint) != ResourceFlags::Unknown)
			return GL_DEPTH24_STENCIL8;

		if ((flags & ResourceFlags::R8_Unorm) != ResourceFlags::Unknown)
			return GL_R8;
		if ((flags & ResourceFlags::R8_Snorm) != ResourceFlags::Unknown)
			return GL_R8_SNORM;
		if ((flags & ResourceFlags::RG8_Unorm) != ResourceFlags::Unknown)
			return GL_RG8;
		if ((flags & ResourceFlags::RG8_Snorm) != ResourceFlags::Unknown)
			return GL_RG8_SNORM;
		if ((flags & ResourceFlags::R16_Unorm) != ResourceFlags::Unknown)
			return GL_R16;
		if ((flags & ResourceFlags::R16_Snorm) != ResourceFlags::Unknown)
			return GL_R16_SNORM;
		if ((flags & ResourceFlags::RG16_Sfloat) != ResourceFlags::Unknown)
			return GL_RG16F;
		if ((flags & ResourceFlags::R32_Sint) != ResourceFlags::Unknown)
			return GL_R32I;
		if ((flags & ResourceFlags::RG32_Sfloat) != ResourceFlags::Unknown)
			return GL_RG32F;
		if ((flags & ResourceFlags::RGB10A2_Unorm) != ResourceFlags::Unknown)
			return GL_RGB10_A2;
		if ((flags & ResourceFlags::R11G11B10_UFloat) != ResourceFlags::Unknown)
			return GL_R11F_G11F_B10F;
		if ((flags & ResourceFlags::RGBA8_SRGB) != ResourceFlags::Unknown)
			return GL_SRGB8_ALPHA8;
		if ((flags & ResourceFlags::BGRA8_SRGB) != ResourceFlags::Unknown)
			return GL_SRGB8_ALPHA8;                 // Same internal format, data order specified separately
		if ((flags & ResourceFlags::RGBA8_Snorm) != ResourceFlags::Unknown)
			return GL_RGBA8_SNORM;
		if ((flags & ResourceFlags::RGBA8_Sint) != ResourceFlags::Unknown)
			return GL_RGBA8I;
		if ((flags & ResourceFlags::R16_Uint) != ResourceFlags::Unknown)
			return GL_R16UI;
		if ((flags & ResourceFlags::D16_Unorm) != ResourceFlags::Unknown)
			return GL_DEPTH_COMPONENT16;
		if ((flags & ResourceFlags::D32FloatS8Uint) != ResourceFlags::Unknown)
			return GL_DEPTH32F_STENCIL8;
		if ((flags & ResourceFlags::BC6H_UF16) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		if ((flags & ResourceFlags::BC6H_SF16) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
		if ((flags & ResourceFlags::BC7_Unorm) != ResourceFlags::Unknown)
			return GL_COMPRESSED_RGBA_BPTC_UNORM;
		if ((flags & ResourceFlags::BC7_SRGB) != ResourceFlags::Unknown)
			return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
	
		return GL_RGBA8;
	}
}
#endif // !defined(__APPLE__)