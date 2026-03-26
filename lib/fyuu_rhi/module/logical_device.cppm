module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string_view>
#endif
export module fyuu_rhi:logical_device;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :types;
import :physical_device;

namespace fyuu_rhi {

	export class LogicalDevice {
	private:
		UniqueLogicalDevice m_impl;

	public:
		LogicalDevice(PhysicalDevice const& physical_device);

		PolymorphicLogicalDeviceBase* GetHandle() const noexcept;

		API GetAPI() const noexcept;

		std::string_view GetAPIString() const noexcept;

	};


} // namespace fyuu_rhi