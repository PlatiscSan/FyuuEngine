module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_surface;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

    export class WebGPUSurface
        : public PolymorphicSurfaceBase,
        public WebGPUCommon<WebGPUSurface> {
    private:
        PlatformHandle m_handle;
        wgpu::Surface m_impl;

    public:
        WebGPUSurface(
            WebGPUPhysicalDevice const& physical_device,
            PlatformHandle const& handle
        );

        std::pair<std::size_t, std::size_t> GetSize() const noexcept;

        PlatformHandle const& GetPlatformHandle() const noexcept;

        wgpu::Surface GetNative() const noexcept;
    };

} // namespace fyuu_rhi::webgpu