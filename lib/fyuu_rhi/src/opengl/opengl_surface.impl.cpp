/* opengl_surface.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:opengl_surface_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_surface;
import :types;
import :platform_handle;
import :opengl_common;
import :opengl_physical_device;

namespace fyuu_rhi::opengl {

    OpenGLSurface::OpenGLSurface(
		OpenGLPhysicalDevice const& physical_device,
		PlatformHandle const& handle
	) : PolymorphicSurfaceBase(this),
        OpenGLCommon(this, physical_device),
        m_handle(handle) {

    }

    std::pair<std::size_t, std::size_t> OpenGLSurface::GetSize() const noexcept {
        return ::fyuu_rhi::GetSize(m_handle);
    }

    PlatformHandle const &OpenGLSurface::GetPlatformHandle() const noexcept {
        return m_handle;
    }
}

#endif // !defined(__APPLE__)