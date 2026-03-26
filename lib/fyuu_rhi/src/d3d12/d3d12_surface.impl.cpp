/* d3d12_surface.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
module fyuu_rhi:d3d12_surface_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_surface;
import :types;
import :platform_handle;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

    D3D12Surface::D3D12Surface(D3D12PhysicalDevice const& physical_device, PlatformHandle const& handle)
        : PolymorphicSurfaceBase(this),
        D3D12Common(this),
        m_handle(handle) {
    }

    std::pair<size_t, size_t> D3D12Surface::GetSize() const noexcept {
        return ::fyuu_rhi::GetSize(m_handle);
    }

    ::fyuu_rhi::PlatformHandle const& D3D12Surface::GetPlatformHandle() const noexcept {
        return m_handle;
    }

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)