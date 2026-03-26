/* opengl_surface.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:opengl_surface;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :opengl_common;

namespace fyuu_rhi::opengl {
        
    export class OpenGLSurface
        : public PolymorphicSurfaceBase,
		public OpenGLCommon<OpenGLSurface> {
	private:
        PlatformHandle m_handle;

	public:
		OpenGLSurface(
			OpenGLPhysicalDevice const& physical_device,
			PlatformHandle const& handle
		);

		std::pair<std::size_t, std::size_t> GetSize() const noexcept;

		PlatformHandle const& GetPlatformHandle() const noexcept;

	};

}

#endif // !defined(__APPLE__)