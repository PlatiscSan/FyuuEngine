module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#include <stdexcept>
#endif
#include "glad.hpp"
export module fyuu_rhi:opengl_view_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_view;
import plastic.resource;
import :types;
import :opengl_common;
import :opengl_logical_device;
import :opengl_resource;

namespace fyuu_rhi::opengl {

	OpenGLView::OpenGLView(OpenGLLogicalDevice const& logical_device, OpenGLResource const& resource, BufferSize buffer_size, BufferSize offset)
		: PolymorphicViewBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, &resource, buffer_size, offset]() {

				MakeCurrent();

				GLuint native = resource.GetNative();
				GLenum target = resource.GetTarget();
				if (target == 0u) {
					throw std::invalid_argument("Invalid resource for view creation");
				}

				if (target != GL_ARRAY_BUFFER || target != GL_SHADER_STORAGE_BUFFER) {
					throw std::invalid_argument("Resource is not a buffer");
				}

				GLenum internal_format = resource.GetInternalFormat();
				if (internal_format == 0) {
					throw std::invalid_argument("Buffer resource has no internal format; cannot create texel buffer view");
				}
				GLuint tex = 0u;
				glCreateTextures(GL_TEXTURE_BUFFER, 1u, &tex);
				if (!tex) {
					throw std::runtime_error("Failed to create buffer texture");
				}
	
				glTextureBuffer(tex, internal_format, native);

				return tex;

			}()) {
	}

	OpenGLView::OpenGLView(OpenGLLogicalDevice const& logical_device, OpenGLResource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count) 
		: PolymorphicViewBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, &resource, base_mip_level, level_count, base_layer, layer_count]() {

				MakeCurrent();

				GLenum target = resource.GetTarget();
				if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D && target != GL_TEXTURE_3D && target != GL_TEXTURE_CUBE_MAP) {
					throw std::invalid_argument("Resource is not a texture");
				}
		
				GLuint orig_tex = resource.GetNative();
				GLenum internal_format = resource.GetInternalFormat();

				GLuint view_tex = 0;
				glCreateTextures(target, 1, &view_tex);
				if (!view_tex) {
					throw std::runtime_error("Failed to create texture view");
				}
		
				glTextureView(
					view_tex, 
					target, 
					orig_tex, 
					internal_format,
					static_cast<GLuint>(base_mip_level), 
					static_cast<GLuint>(level_count),
					static_cast<GLuint>(base_layer), 
					static_cast<GLuint>(layer_count)
				);

				if (glGetError() != GL_NO_ERROR) {
					glDeleteTextures(1, &view_tex);
					throw std::runtime_error("glTextureView failed");
				}

				return view_tex;
		
			}()) {
	}

	OpenGLView::OpenGLView(OpenGLView&& other) noexcept
		: PolymorphicViewBase(std::move(other)),
		OpenGLCommon(std::move(other)),
		m_impl(std::exchange(other.m_impl, 0u)) {
	}

	OpenGLView::~OpenGLView() noexcept {
		if (m_impl) {
			MakeCurrent();
			glDeleteTextures(1, &m_impl);
		}
	}

	GLuint OpenGLView::GetNative() const noexcept {
		return m_impl;
	}
}

#endif // !defined(__APPLE__)