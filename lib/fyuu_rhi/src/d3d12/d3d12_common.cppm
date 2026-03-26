/* d3d12_common.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:d3d12_common;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :common;


namespace fyuu_rhi::d3d12 {
    
    export template <class Derived> class D3D12Common 
		: public ::fyuu_rhi::RHICommon<Derived> {
	private:
		using Base = ::fyuu_rhi::RHICommon<Derived>;

	public:
		D3D12Common(Derived* derived) noexcept
			: Base(derived) {

		}

		D3D12Common(D3D12Common&& other) noexcept = default;

		static constexpr API GetAPI() noexcept {
			return API::DirectX12;
		}

		static constexpr std::string_view GetAPIString() noexcept {
			return "DirectX12";
		}

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)