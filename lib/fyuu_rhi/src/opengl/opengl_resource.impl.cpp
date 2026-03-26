module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <system_error>
#include <stdexcept>
#include <optional>
#include <variant>
#include <span>
#include <format>
#include <concepts>
#endif
#include "glad.hpp"
module fyuu_rhi:opengl_resource_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_resource;
import :enums;
import :types;
import :opengl_common;
import :opengl_logical_device; 
import :opengl_types;

namespace fyuu_rhi::opengl {

	GLenum OpenGLResource::DetermineBufferTarget(ResourceFlags flags) noexcept {
		if ((flags & ResourceFlags::IndexBuffer) != ResourceFlags::Unknown)
			return GL_ELEMENT_ARRAY_BUFFER;
		if ((flags & ResourceFlags::VertexBuffer) != ResourceFlags::Unknown)
			return GL_ARRAY_BUFFER;
		if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown)
			return GL_UNIFORM_BUFFER;
		if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown)
			return GL_SHADER_STORAGE_BUFFER;
		if ((flags & ResourceFlags::IndirectBuffer) != ResourceFlags::Unknown)
			return GL_DRAW_INDIRECT_BUFFER;
		return GL_ARRAY_BUFFER;
	}

	GLbitfield OpenGLResource::VideoMemoryTypeToBufferFlags(VideoMemoryType type) noexcept {
		switch (type) {
		case VideoMemoryType::DeviceLocal:   	return 0;
		case VideoMemoryType::HostVisible:   	return GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_CLIENT_STORAGE_BIT;
		case VideoMemoryType::DeviceReadback:	return GL_MAP_READ_BIT;
		default:							 	return 0;
		}
	}

	GLenum OpenGLResource::DetermineTextureTarget(ResourceFlags flags) noexcept {
		if ((flags & ResourceFlags::Texture1D) != ResourceFlags::Unknown)
			return GL_TEXTURE_1D;
		if ((flags & ResourceFlags::Texture2D) != ResourceFlags::Unknown)
			return GL_TEXTURE_2D;
		if ((flags & ResourceFlags::Texture3D) != ResourceFlags::Unknown)
			return GL_TEXTURE_3D;
		if ((flags & ResourceFlags::TextureCube) != ResourceFlags::Unknown)
			return GL_TEXTURE_CUBE_MAP;
		return GL_TEXTURE_2D;
	}

	OpenGLResource::Buffer::Buffer(Buffer&& other) noexcept
		: OpenGLStateMachine(std::move(other)),
		target(std::exchange(other.target, 0u)),
		flags(std::exchange(other.flags, 0u)),
		size(std::exchange(other.size, 0u)),
		impl(std::exchange(other.impl, 0u)) {
	}

	OpenGLResource::Buffer::~Buffer() noexcept {
		if (impl) {
			MakeCurrent();
			glDeleteBuffers(1u, &impl);
		}
	}

	OpenGLResource::Texture::Texture(Texture&& other) noexcept 
		: OpenGLStateMachine(std::move(other)),
		target(std::exchange(other.target, 0u)),
		internal_format(std::exchange(other.internal_format, 0u)),
		size(std::move(other.size)),
		impl(std::exchange(other.impl, 0u)) {
	}

	OpenGLResource::Texture::~Texture() noexcept {
		if (impl) {
			MakeCurrent();
			InvalidateTexture(impl);
			glDeleteTextures(1u, &impl);
		}
	}

	OpenGLResource::OpenGLResource(OpenGLLogicalDevice const& device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		  OpenGLCommon(this, device),
		  m_handle(std::in_place_type<Buffer>, this, flags, type , buffer_size) {
	}

	OpenGLResource::OpenGLResource(OpenGLLogicalDevice const& device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		OpenGLCommon(this, device),
		m_handle(std::in_place_type<Texture>, this, flags, type, texture_size) {

	}

	GLuint OpenGLResource::GetNative() const noexcept {
		return std::visit(
			[](auto&& handle) -> GLuint {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, Buffer>) {
					return handle.impl;
				}
				else if constexpr (std::same_as<Handle, Texture>) {
					return handle.impl;
				}
				else {
					return 0u;
				}
			},
			m_handle
		);
	}

	GLenum OpenGLResource::GetTarget() const noexcept {
		return std::visit(
			[](auto&& handle) -> GLuint {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, Buffer>) {
					return handle.target;
				}
				else if constexpr (std::same_as<Handle, Texture>) {
					return handle.target;
				}
				else {
					return 0u;
				}
			},
			m_handle
		);
	}

	GLenum OpenGLResource::GetInternalFormat() const noexcept {
		return std::visit(
			[](auto&& handle) -> GLenum {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, Buffer>) {
					return handle.internal_format;
				}
				else if constexpr (std::same_as<Handle, Texture>) {
					return handle.internal_format;
				}
				else {
					return 0u;
				}
			},
			m_handle
		);
	}

	std::pair<GLenum, GLenum> OpenGLResource::GetFormat() const noexcept {
		return std::visit(
			[](auto&& handle) -> std::pair<GLenum, GLenum> {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, Texture>) {
					return handle.format;
				}
				else {
					return {};
				}
			},
			m_handle
		);
	}

	std::optional<std::size_t> OpenGLResource::GetBytesPerPixel() const noexcept {
		return std::visit(
			[](auto&& handle) -> std::optional<std::size_t> {
				using Handle = std::decay_t<decltype(handle)>;
				if constexpr (std::same_as<Handle, Texture>) {
					// Map OpenGL internal format to bytes per pixel.
					GLenum fmt = handle.internal_format;
					switch (fmt) {
					// 8‑bit per channel
					case GL_R8:      return 1;
					case GL_RG8:     return 2;
					case GL_RGB8:    return 3;
					case GL_RGBA8:   return 4;
					case GL_SRGB8:   return 3;
					case GL_SRGB8_ALPHA8: return 4;
	
					// 16‑bit per channel
					case GL_R16:     return 2;
					case GL_RG16:    return 4;
					case GL_RGB16:   return 6;
					case GL_RGBA16:  return 8;
	
					// 32‑bit float per channel
					case GL_R32F:    return 4;
					case GL_RG32F:   return 8;
					case GL_RGB32F:  return 12;
					case GL_RGBA32F: return 16;
	
					// 16‑bit float per channel
					case GL_R16F:    return 2;
					case GL_RG16F:   return 4;
					case GL_RGB16F:  return 6;
					case GL_RGBA16F: return 8;
	
					// Depth / stencil formats
					case GL_DEPTH_COMPONENT16:  return 2;
					case GL_DEPTH_COMPONENT24:  return 3;
					case GL_DEPTH_COMPONENT32F: return 4;
					case GL_DEPTH24_STENCIL8:   return 4;
					case GL_DEPTH32F_STENCIL8:  return 8;
	
					// Compressed formats are not trivial; fall back to nullopt.
					default:
						return std::nullopt;
					}
				} else {
					// Not a texture – no meaningful pixel size.
					return std::nullopt;
				}
			},
			m_handle
		);
	}

} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)