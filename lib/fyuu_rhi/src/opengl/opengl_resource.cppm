module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <optional>
#include <variant>
#include <span>
#include <concepts>
#endif
#include "glad.hpp"
export module fyuu_rhi:opengl_resource;
#if !defined(__APPLE__) 
#if defined(__cpp_lib_modules)
import std;
#endif
import :enums;
import :types;
import :opengl_common;
import :opengl_types;

namespace fyuu_rhi::opengl {

	export class OpenGLResource
		: public PolymorphicResourceBase,
		public OpenGLCommon<OpenGLResource> {
	private:
		static GLenum DetermineBufferTarget(ResourceFlags flags) noexcept;
		static GLbitfield VideoMemoryTypeToBufferFlags(VideoMemoryType type) noexcept;
		static GLenum DetermineTextureTarget(ResourceFlags flags) noexcept;

		struct Buffer 
			: public OpenGLStateMachine {
			GLenum target;
			GLenum internal_format;
			GLbitfield flags;
			BufferSize size;
			GLuint impl;

			template <std::derived_from<OpenGLStateMachine> Derived>
			Buffer(Derived* derived, ResourceFlags flags_, VideoMemoryType type, BufferSize buffer_size)
				: OpenGLStateMachine(derived),
				target(DetermineBufferTarget(flags_)),
				internal_format(DetermineInternalFormat(flags_)),
				flags(VideoMemoryTypeToBufferFlags(type)),
				size(buffer_size),
				impl(
					[this]() {
						MakeCurrent();
						GLuint impl_ = 0u;
						glCreateBuffers(1u, &impl_);

						if (!impl_) {
							throw std::runtime_error("Failed to create buffer");
						}

						glNamedBufferStorage(target, size, nullptr, flags);
						return impl_;
					}()) {

			}

			Buffer(Buffer const&) = delete;
			Buffer(Buffer&& other) noexcept;

			~Buffer() noexcept;

		};

		struct Texture 
			: public OpenGLStateMachine {
			GLenum target;
			GLenum internal_format;
			std::pair<GLenum, GLenum> format;
			TextureSize size;
			GLuint impl;

			template <std::derived_from<OpenGLStateMachine> Derived>
			Texture(Derived* derived, ResourceFlags flags, VideoMemoryType type, TextureSize texture_size)
				: OpenGLStateMachine(derived),
				target(DetermineTextureTarget(flags)),
				internal_format(DetermineInternalFormat(flags)),
				format(DetermineFormat(flags)),
				size(texture_size),
				impl(
					[this]() {
						MakeCurrent();

						GLuint impl_ = 0u;
						glCreateTextures(target, 1u, &impl_);
						if (!impl_) {
							throw std::runtime_error("Failed to create texture");
						}

						auto width = static_cast<GLsizei>(size.width);
						auto height = static_cast<GLsizei>(size.height);
						auto depth = static_cast<GLsizei>(size.depth);
						switch (target) {
						case GL_TEXTURE_1D:
							glTextureStorage1D(impl_, 1u, internal_format, width);
							break;
						case GL_TEXTURE_2D:
							glTextureStorage2D(impl_, 1u, internal_format, width, height);
							break;
						case GL_TEXTURE_3D:
							glTextureStorage3D(impl_, 1u, internal_format, width, height, depth);
							break;
						case GL_TEXTURE_CUBE_MAP:
							if (height != width) {
								throw std::runtime_error("Cube map texture must have square dimensions");
							}
							glTextureStorage2D(impl_, 6, internal_format, width, height);
							break;
						default:
							throw std::runtime_error("Unsupported texture target");
						}

						return impl_;

					}()) {

			}


			Texture(Texture const&) = delete;
			Texture(Texture&& other) noexcept;

			~Texture() noexcept;

		};

		using HandleVariant = std::variant<std::monostate, Buffer, Texture>;

		HandleVariant m_handle;

	public:
		OpenGLResource(OpenGLLogicalDevice const& device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags);

		OpenGLResource(OpenGLLogicalDevice const& device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags);

		OpenGLResource(OpenGLResource&& other) noexcept = default;

		GLuint GetNative() const noexcept;

		GLenum GetTarget() const noexcept;

		GLenum GetInternalFormat() const noexcept;

		std::pair<GLenum, GLenum> GetFormat() const noexcept;

		std::optional<std::size_t> GetBytesPerPixel() const noexcept;

	};

}

#endif // !defined(__APPLE__) 