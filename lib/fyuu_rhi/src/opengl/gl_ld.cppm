module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <memory_resource>
#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)
#include "glad/glad.h"
#if defined(_WIN32)
#include "glad/glad_wgl.h"
#else
#if defined(__linux__)
#include "glad/glad_glx.h"
#endif // defined(__linux__)
#include "glad/glad_egl.h"
#endif // defined(_WIN32)
#endif //!defined(__APPLE__)
module fyuu_rhi:opengl_logical_device;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_traits;
import :resource_types;
import :command_types;

namespace {

	using namespace fyuu_rhi;

	GLbitfield ExtractBufferFlags(ResourceFlags const& flags) noexcept {
		GLbitfield gl_flags = 0;
		if (flags.Test(ResourceFlagBits::CopySRC)) {
			gl_flags |= GL_COPY_READ_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::CopyDST)) {
			gl_flags |= GL_COPY_WRITE_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::UniformTexelBuffer)) {
			gl_flags |= GL_TEXTURE_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::StorageTexelBuffer)) {
			gl_flags |= GL_TEXTURE_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::UniformBuffer)) {
			gl_flags |= GL_UNIFORM_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::StorageBuffer)) {
			gl_flags |= GL_SHADER_STORAGE_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::IndexBuffer)) {
			gl_flags |= GL_ELEMENT_ARRAY_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::VertexBuffer)) {
			gl_flags |= GL_ARRAY_BUFFER;
		}
		if (flags.Test(ResourceFlagBits::IndirectBuffer)) {
			gl_flags |= GL_DRAW_INDIRECT_BUFFER;
		}
		return gl_flags;
	}

	GLenum ExtractTextureTarget(ResourceFlags const& flags, std::size_t depth_arr_layers, GLsizei sample_cnt) {
		
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Texture1D, ResourceFlagBits::Texture3D);
		if (is_conflicting) {
			throw std::invalid_argument("Texture1D Texture2D or Texture3D are set simultaneously");
		}

		bool is_1d = false;
		bool is_2d = false;
		bool is_3d = false;
		if (flags.Test(ResourceFlagBits::Texture1D)) {
			is_1d = true;
		}
		else if (flags.Test(ResourceFlagBits::Texture2D)) {
			is_2d = true;
		}
		else if (flags.Test(ResourceFlagBits::Texture3D)) {
			is_3d = true;
		}
		else {
			is_2d = true;
		}

		if (is_1d) {
			if (sample_cnt > 1) {
				throw std::invalid_argument("Multisample not supported for 1D textures");
			}
			return (depth_arr_layers > 1) ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;
		}
		else if (is_2d) {
			if (sample_cnt > 1) {
				return (depth_arr_layers > 1) ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE;
			} else {
				return (depth_arr_layers > 1) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
			}
		}
		else if (is_3d) {
			if (sample_cnt > 1) {
				throw std::invalid_argument("Multisample not supported for 3D textures");
			}
			if (depth_arr_layers <= 1) {
				throw std::invalid_argument("3D texture must have depth > 1");
			}
			return GL_TEXTURE_3D;
		}
		else {
			return GL_TEXTURE_2D;
		}

	}

	GLsizei ExtractSampleCount(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Sample1, ResourceFlagBits::Sample64);
		if (is_conflicting) {
			throw std::invalid_argument("Only one sample count can be set");
		}
		if (flags.Test(ResourceFlagBits::Sample1)) return 1;
		else if (flags.Test(ResourceFlagBits::Sample2)) return 2;
		else if (flags.Test(ResourceFlagBits::Sample4)) return 4;
		else if (flags.Test(ResourceFlagBits::Sample8)) return 8;
		else if (flags.Test(ResourceFlagBits::Sample16)) return 16;
		else if (flags.Test(ResourceFlagBits::Sample32)) return 32;
		else if (flags.Test(ResourceFlagBits::Sample64)) return 64;
		else return 1;
	}

	GLenum ExtractInternalFormat(ResourceFlags const& flags) {

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::R8Unorm, ResourceFlagBits::Bc7UnormSrgb);
		if (is_conflicting) {
			throw std::invalid_argument("Only one format can be set");
		}

		if (flags.Test(ResourceFlagBits::R8Unorm)) return GL_R8;
		else if (flags.Test(ResourceFlagBits::R8Snorm)) return GL_R8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8Uint)) return GL_R8UI;
		else if (flags.Test(ResourceFlagBits::R8Sint)) return GL_R8I;
	
		// 8‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8Unorm)) return GL_RG8;
		else if (flags.Test(ResourceFlagBits::R8G8Snorm)) return GL_RG8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8G8Uint)) return GL_RG8UI;
		else if (flags.Test(ResourceFlagBits::R8G8Sint)) return GL_RG8I;
	
		// 8‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Unorm)) return GL_RGBA8;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Snorm)) return GL_RGBA8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Uint)) return GL_RGBA8UI;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Sint)) return GL_RGBA8I;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Srgb)) return GL_SRGB8_ALPHA8;
		else if (flags.Test(ResourceFlagBits::B8G8R8A8Srgb)) return GL_SRGB8_ALPHA8;
	
		// 16‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R16Unorm)) return GL_R16;
		else if (flags.Test(ResourceFlagBits::R16Snorm)) return GL_R16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16Uint)) return GL_R16UI;
		else if (flags.Test(ResourceFlagBits::R16Sint)) return GL_R16I;
		else if (flags.Test(ResourceFlagBits::R16Float)) return GL_R16F;
	
		// 16‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16Unorm)) return GL_RG16;
		else if (flags.Test(ResourceFlagBits::R16G16Snorm)) return GL_RG16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16G16Uint)) return GL_RG16UI;
		else if (flags.Test(ResourceFlagBits::R16G16Sint)) return GL_RG16I;
		else if (flags.Test(ResourceFlagBits::R16G16Float)) return GL_RG16F;
	
		// 16‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Unorm)) return GL_RGBA16;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Snorm)) return GL_RGBA16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Uint)) return GL_RGBA16UI;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Sint)) return GL_RGBA16I;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Float)) return GL_RGBA16F;
	
		// 32‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R32Uint)) return GL_R32UI;
		else if (flags.Test(ResourceFlagBits::R32Sint)) return GL_R32I;
		else if (flags.Test(ResourceFlagBits::R32Float)) return GL_R32F;
	
		// 32‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32Uint)) return GL_RG32UI;
		else if (flags.Test(ResourceFlagBits::R32G32Sint)) return GL_RG32I;
		else if (flags.Test(ResourceFlagBits::R32G32Float)) return GL_RG32F;
	
		// 32‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Uint)) return GL_RGBA32UI;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Sint)) return GL_RGBA32I;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Float)) return GL_RGBA32F;
	
		// Packed formats
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Unorm)) return GL_RGB10_A2;
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Uint)) return GL_RGB10_A2UI;
		else if (flags.Test(ResourceFlagBits::R11G11B10Float)) return GL_R11F_G11F_B10F;
		else if (flags.Test(ResourceFlagBits::R9G9B9E5SharedExp)) return GL_RGB9_E5;
	
		// Depth/stencil
		else if (flags.Test(ResourceFlagBits::D16Unorm)) return GL_DEPTH_COMPONENT16;
		else if (flags.Test(ResourceFlagBits::D24UnormS8Uint)) return GL_DEPTH24_STENCIL8;
		else if (flags.Test(ResourceFlagBits::D32Float)) return GL_DEPTH_COMPONENT32F;
		else if (flags.Test(ResourceFlagBits::D32FloatS8X24Uint)) return GL_DEPTH32F_STENCIL8;
	
		// BC compressed formats (using standard EXT/ARB constants)
		else if (flags.Test(ResourceFlagBits::Bc1Unorm)) return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		else if (flags.Test(ResourceFlagBits::Bc1UnormSrgb)) return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		else if (flags.Test(ResourceFlagBits::Bc2Unorm)) return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		else if (flags.Test(ResourceFlagBits::Bc2UnormSrgb)) return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		else if (flags.Test(ResourceFlagBits::Bc3Unorm)) return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		else if (flags.Test(ResourceFlagBits::Bc3UnormSrgb)) return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		else if (flags.Test(ResourceFlagBits::Bc4Unorm)) return GL_COMPRESSED_RED_RGTC1;
		else if (flags.Test(ResourceFlagBits::Bc4Snorm)) return GL_COMPRESSED_SIGNED_RED_RGTC1;
		else if (flags.Test(ResourceFlagBits::Bc5Unorm)) return GL_COMPRESSED_RG_RGTC2;
		else if (flags.Test(ResourceFlagBits::Bc5Snorm)) return GL_COMPRESSED_SIGNED_RG_RGTC2;
		else if (flags.Test(ResourceFlagBits::Bc6HUfloat)) return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		else if (flags.Test(ResourceFlagBits::Bc6HSfloat)) return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
		else if (flags.Test(ResourceFlagBits::Bc7Unorm)) return GL_COMPRESSED_RGBA_BPTC_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc7UnormSrgb)) return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
	
		else return GL_RGBA8UI;

	}

}

namespace fyuu_rhi::opengl {

	std::shared_ptr<GLuint> Backend::CreateBuffer(Backend::Dummy, std::size_t size_in_bytes, ResourceFlags const& flags) {

		static thread_local std::array<std::byte, 1024 * sizeof(GLuint)> fixed_buffer;
		static thread_local std::pmr::monotonic_buffer_resource fixed_upstream(
			fixed_buffer.data(), fixed_buffer.size(),
			std::pmr::new_delete_resource()
		);

		static thread_local std::pmr::unsynchronized_pool_resource pool(&fixed_upstream);
		static thread_local std::pmr::polymorphic_allocator<GLuint> pmr_alloc(&pool);

		auto buf = pmr_alloc.new_object<GLuint>(0u);
		glCreateBuffers(1u, buf);
		glNamedBufferStorage(*buf, size_in_bytes, nullptr, ExtractBufferFlags(flags));

		std::shared_ptr<GLuint> shared_buf(
			buf,
			[](GLuint* buf) {
				glDeleteBuffers(1u, buf);
				pmr_alloc.delete_object(buf);
			}
		);

		return shared_buf;

	}

	std::shared_ptr<GLuint> Backend::CreateTexture(Dummy, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) {
		
		static thread_local std::array<std::byte, 1024 * sizeof(GLuint)> fixed_buffer;
		static thread_local std::pmr::monotonic_buffer_resource fixed_upstream(
			fixed_buffer.data(), fixed_buffer.size(),
			std::pmr::new_delete_resource()
		);

		static thread_local std::pmr::unsynchronized_pool_resource pool(&fixed_upstream);
		static thread_local std::pmr::polymorphic_allocator<GLuint> pmr_alloc(&pool);

		GLsizei sample_cnt = ExtractSampleCount(flags);
		GLenum target = ExtractTextureTarget(flags, depth_arr_layers, sample_cnt);
		GLenum internal_format = ExtractInternalFormat(flags);

		if (sample_cnt > 1 && mip_lvl_cnt != 1) {
			throw std::invalid_argument("Multisample textures must have mip_lvl_cnt == 1");
		}

		auto tex = pmr_alloc.new_object<GLuint>(0u);
		glCreateTextures(target, 1u, tex);
		
		switch (target) {
		case GL_TEXTURE_1D:
			glTextureStorage1D(*tex, static_cast<GLsizei>(mip_lvl_cnt), internal_format, static_cast<GLsizei>(width));
			break;
		case GL_TEXTURE_1D_ARRAY:
			glTextureStorage2D(
				*tex, static_cast<GLsizei>(mip_lvl_cnt), internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(depth_arr_layers)
			);
			break;
		case GL_TEXTURE_2D:
			glTextureStorage2D(
				*tex, static_cast<GLsizei>(mip_lvl_cnt), internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(height)
			);
			break;
		case GL_TEXTURE_2D_ARRAY:
			glTextureStorage3D(
				*tex, static_cast<GLsizei>(mip_lvl_cnt), internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth_arr_layers)
			);
			break;
		case GL_TEXTURE_2D_MULTISAMPLE:
			glTextureStorage2DMultisample(
				*tex, sample_cnt, internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_TRUE
			);
			break;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			glTextureStorage3DMultisample(
				*tex, sample_cnt, internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(height),
				static_cast<GLsizei>(depth_arr_layers), GL_TRUE
			);
			break;
		case GL_TEXTURE_3D:
			glTextureStorage3D(
				*tex, static_cast<GLsizei>(mip_lvl_cnt), internal_format,
				static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth_arr_layers)
			);
			break;
		default:
			glDeleteTextures(1u, tex);
			throw std::runtime_error("Unsupported texture target");
		}

		glTextureParameteri(*tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(*tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(*tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(*tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) {
			glTextureParameteri(*tex, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}

		std::shared_ptr<GLuint> shared_tex(
			tex,
			[](GLuint* tex) {
				glDeleteTextures(1u, tex);
				pmr_alloc.delete_object(tex);
			}
		);
		return shared_tex;

	}

}
#endif // !defined(__APPLE__)