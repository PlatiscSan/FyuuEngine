
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <vector>
#include <variant>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
module fyuu_rhi:opengl_sampler_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_sampler;
import :types;
import :opengl_common;
import :opengl_logical_device;
import :opengl_shader_data_segment;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::opengl;

	GLuint CreateSampler(SamplerFlags flags, SamplerAttachmentInfo const& attachment) {
		if (HasConflictingFlags(flags, SamplerFlags::FilterModes))
			throw std::invalid_argument("Multiple filter modes in sampler flags");
		if (HasConflictingFlags(flags, SamplerFlags::AddressModes))
			throw std::invalid_argument("Multiple address modes in sampler flags");
		if (HasConflictingFlags(flags, SamplerFlags::CompareFunctions))
			throw std::invalid_argument("Multiple compare functions in sampler flags");
		if (HasConflictingFlags(flags, SamplerFlags::BorderColors))
			throw std::invalid_argument("Multiple border colors in sampler flags");
		if (HasConflictingFlags(flags, SamplerFlags::ReductionModes))
			throw std::invalid_argument("Multiple reduction modes in sampler flags");
		if (HasConflictingFlags(flags, SamplerFlags::AnisotropyModes))
			throw std::invalid_argument("Multiple anisotropy levels in sampler flags");

		GLuint sampler;
		glGenSamplers(1, &sampler);

		GLenum mag = ((flags & SamplerFlags::MagNearest) != SamplerFlags::Unknown) ? GL_NEAREST : GL_LINEAR;
		glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, mag);

		GLenum min = GL_LINEAR;
		bool min_nearest = (flags & SamplerFlags::MinNearest) != SamplerFlags::Unknown;
		bool mip_nearest = (flags & SamplerFlags::MipNearest) != SamplerFlags::Unknown;
		if (min_nearest && mip_nearest)      min = GL_NEAREST_MIPMAP_NEAREST;
		else if (min_nearest && !mip_nearest) min = GL_NEAREST_MIPMAP_LINEAR;
		else if (!min_nearest && mip_nearest) min = GL_LINEAR_MIPMAP_NEAREST;
		else                                   min = GL_LINEAR_MIPMAP_LINEAR;
		glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, min);

		GLenum address = GL_REPEAT;
		if ((flags & SamplerFlags::Repeat) != SamplerFlags::Unknown)
			address = GL_REPEAT;
		else if ((flags & SamplerFlags::MirroredRepeat) != SamplerFlags::Unknown)
			address = GL_MIRRORED_REPEAT;
		else if ((flags & SamplerFlags::ClampToEdge) != SamplerFlags::Unknown)
			address = GL_CLAMP_TO_EDGE;
		else if ((flags & SamplerFlags::ClampToBorder) != SamplerFlags::Unknown)
			address = GL_CLAMP_TO_BORDER;
		else if ((flags & SamplerFlags::MirrorClampToEdge) != SamplerFlags::Unknown)
			address = GL_MIRROR_CLAMP_TO_EDGE;
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, address);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, address);
		glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, address);

		GLenum compare = GL_ALWAYS;
		if ((flags & SamplerFlags::Never) != SamplerFlags::Unknown)
			compare = GL_NEVER;
		else if ((flags & SamplerFlags::Less) != SamplerFlags::Unknown)
			compare = GL_LESS;
		else if ((flags & SamplerFlags::Equal) != SamplerFlags::Unknown)
			compare = GL_EQUAL;
		else if ((flags & SamplerFlags::LessOrEqual) != SamplerFlags::Unknown)
			compare = GL_LEQUAL;
		else if ((flags & SamplerFlags::Greater) != SamplerFlags::Unknown)
			compare = GL_GREATER;
		else if ((flags & SamplerFlags::NotEqual) != SamplerFlags::Unknown)
			compare = GL_NOTEQUAL;
		else if ((flags & SamplerFlags::GreaterOrEqual) != SamplerFlags::Unknown)
			compare = GL_GEQUAL;

		if ((flags & SamplerFlags::CompareFunctions) != SamplerFlags::Unknown) {
			glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, compare);
		} else {
			glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		}

		std::array border = {0.0f, 0.0f, 0.0f, 0.0f};
		if ((flags & SamplerFlags::FloatTransparentBlack) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = border[3] = 0.0f;
		} else if ((flags & SamplerFlags::FloatOpaqueBlack) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = 0.0f; border[3] = 1.0f;
		} else if ((flags & SamplerFlags::FloatOpaqueWhite) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = 1.0f; border[3] = 1.0f;
		} else if ((flags & SamplerFlags::IntTransparentBlack) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = border[3] = 0.0f;
		} else if ((flags & SamplerFlags::IntOpaqueBlack) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = 0.0f; border[3] = 1.0f;
		} else if ((flags & SamplerFlags::IntOpaqueWhite) != SamplerFlags::Unknown) {
			border[0] = border[1] = border[2] = 1.0f; border[3] = 1.0f;
		}
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border.data());

		if ((flags & SamplerFlags::AnisotropyModes) != SamplerFlags::Unknown) {
			GLfloat max_aniso = 1.0f;
			if ((flags & SamplerFlags::Anisotropy2x) != SamplerFlags::Unknown)
				max_aniso = 2.0f;
			else if ((flags & SamplerFlags::Anisotropy4x) != SamplerFlags::Unknown)
				max_aniso = 4.0f;
			else if ((flags & SamplerFlags::Anisotropy8x) != SamplerFlags::Unknown)
				max_aniso = 8.0f;
			else if ((flags & SamplerFlags::Anisotropy16x) != SamplerFlags::Unknown)
				max_aniso = 16.0f;
			glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
		}

		glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, attachment.min_lod);
		glSamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, attachment.max_lod);
		glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, attachment.mip_lod_bias);

		if ((flags & SamplerFlags::Min) != SamplerFlags::Unknown) {
			glSamplerParameteri(sampler, GL_TEXTURE_REDUCTION_MODE_EXT, GL_MIN);
		} else if ((flags & SamplerFlags::Max) != SamplerFlags::Unknown) {
			glSamplerParameteri(sampler, GL_TEXTURE_REDUCTION_MODE_EXT, GL_MAX);
		}

		return sampler;
	}

}

namespace fyuu_rhi::opengl {

	OpenGLSampler::OpenGLSampler(OpenGLLogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment) 
		: PolymorphicSamplerBase(this),
		OpenGLCommon(this, logical_device),
		m_impl(
			[this, flags, &attachment]() {
				MakeCurrent();
				return CreateSampler(flags, attachment);
			}()) {

	}

	OpenGLSampler::OpenGLSampler(OpenGLShaderDataSegment const* segment, SamplerFlags flags, SamplerAttachmentInfo const &attachment)
		: PolymorphicSamplerBase(this),
		OpenGLCommon(this, segment),
		m_impl(
			[this, flags, &attachment]() {
				MakeCurrent();
				return CreateSampler(flags, attachment);
			}()) {
	}

	OpenGLSampler::OpenGLSampler(OpenGLSampler&& other) noexcept
		: PolymorphicSamplerBase(std::move(other)),
		OpenGLCommon(std::move(other)),
		m_impl(std::exchange(other.m_impl, 0u)) {

	}

	OpenGLSampler::~OpenGLSampler() noexcept {
		if (m_impl) {
			MakeCurrent();
			glDeleteSamplers(1u, &m_impl);
			m_impl = 0u;
		}
	}

	GLuint OpenGLSampler::GetNative() const noexcept {
		return m_impl;
	}
	
}

#endif // !defined(__APPLE__)