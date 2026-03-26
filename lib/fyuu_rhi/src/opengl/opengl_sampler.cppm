
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <variant>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
export module fyuu_rhi:opengl_sampler;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :opengl_common;

namespace fyuu_rhi::opengl {
        
    export class OpenGLSampler
        : public PolymorphicSamplerBase,
		public OpenGLCommon<OpenGLSampler> {
	private:
		GLuint m_impl;

	public:
		OpenGLSampler(OpenGLLogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment);
		OpenGLSampler(OpenGLShaderDataSegment const* segment, SamplerFlags flags, SamplerAttachmentInfo const& attachment);
		OpenGLSampler(OpenGLSampler&& other) noexcept;
		~OpenGLSampler() noexcept;

		GLuint GetNative() const noexcept;
	};

}

#endif // !defined(__APPLE__)