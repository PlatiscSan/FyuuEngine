module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:surface;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :physical_device;
import :types;

namespace fyuu_rhi {

    export class Surface {
    private:
        UniqueSurface m_impl;
    
    public:
        Surface(PhysicalDevice const& physical_device, PlatformHandle const& handle);

        std::pair<std::size_t, std::size_t> GetSize() const noexcept;

        ::fyuu_rhi::PlatformHandle const& GetPlatformHandle() const noexcept;

        PolymorphicSurfaceBase* GetHandle() const noexcept;

		API GetAPI() const noexcept;

		std::string_view GetAPIString() const noexcept;

    };

}