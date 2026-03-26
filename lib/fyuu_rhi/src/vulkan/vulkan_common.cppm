/* vulkan_common.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_common;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :common;


namespace fyuu_rhi::vulkan {
    
    export template <class Derived> class VulkanCommon 
		: public ::fyuu_rhi::RHICommon<Derived> {
	private:
		using Base = ::fyuu_rhi::RHICommon<Derived>;

	public:
		VulkanCommon(Derived* derived) noexcept
			: Base(derived) {

		}

		VulkanCommon(VulkanCommon&& other) noexcept = default;

		static constexpr API GetAPI() noexcept {
			return API::Vulkan;
		}

		static constexpr std::string_view GetAPIString() noexcept {
			return "Vulkan";
		}

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)