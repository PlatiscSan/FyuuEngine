/* physical_device.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <string_view>
#endif
export module fyuu_rhi:physical_device;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :types;
import :enums;

namespace fyuu_rhi {

	export class PhysicalDevice {
	private:
		UniquePhysicalDevice m_impl;

	public:
		PhysicalDevice(API api, InitOptions const& init);

		std::uint32_t GetVendorID() const noexcept;

		std::uint32_t GetID() const noexcept;

		std::string GetDescription() const;

		PolymorphicPhysicalDeviceBase* GetHandle() const noexcept;

		API GetAPI() const noexcept;

		std::string_view GetAPIString() const noexcept;

	};


} // namespace fyuu_rhi
