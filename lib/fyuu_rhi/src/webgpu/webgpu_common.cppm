/* webgpu_common.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:webgpu_common;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :common;


namespace fyuu_rhi::webgpu {
    
    export template <class Derived> class WebGPUCommon 
		: public ::fyuu_rhi::RHICommon<Derived> {
	private:
		using Base = ::fyuu_rhi::RHICommon<Derived>;

	public:
		WebGPUCommon(Derived* derived) noexcept
			: Base(derived) {

		}

		WebGPUCommon(WebGPUCommon&& other) noexcept = default;

		static constexpr API GetAPI() noexcept {
			return API::WebGPU;
		}

		static constexpr std::string_view GetAPIString() noexcept {
			return "WebGPU";
		}

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)