module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <ctime>
#endif
#include "glad.hpp"
export module fyuu_rhi:opengl_view;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.resource;
import :types;
import :opengl_common;
import :opengl_resource;

namespace fyuu_rhi::opengl {

	export class OpenGLView
		: public PolymorphicViewBase,
		public OpenGLCommon<OpenGLView> {
	private:
		GLuint m_impl;
		
	public:
		OpenGLView(OpenGLLogicalDevice const& logical_device, OpenGLResource const& resource, BufferSize buffer_size, BufferSize offset);

		OpenGLView(OpenGLLogicalDevice const& logical_device, OpenGLResource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count);

		OpenGLView(OpenGLView&& other) noexcept;

		~OpenGLView() noexcept;

		GLuint GetNative() const noexcept;

	};

}

#endif // !defined(__APPLE__)