/* common pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

}
*/
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string>
#include <optional>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:core_types;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export struct PhysicalDeviceInfo {
		
		enum class Type : std::uint8_t {
			Unknown,
			DiscreteGPU, 
			IntegratedGPU, 
			CPU, 
			Virtual
		};

		std::string name;
		std::optional<std::uint32_t> vendor_id;
		std::optional<std::uint32_t> device_id;
		std::optional<std::size_t> dedicated_memory;
		Type type;
	};

}
