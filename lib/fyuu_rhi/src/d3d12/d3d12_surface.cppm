/* d3d12_surface.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:d3d12_surface;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
    
    export class D3D12Surface
        : public PolymorphicSurfaceBase,
		public D3D12Common<D3D12Surface> {
	private:
		PlatformHandle m_handle;

	public:
		D3D12Surface(D3D12PhysicalDevice const& physical_device, PlatformHandle const& handle);

		std::pair<std::size_t, std::size_t> GetSize() const noexcept;

		::fyuu_rhi::PlatformHandle const& GetPlatformHandle() const noexcept;

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)