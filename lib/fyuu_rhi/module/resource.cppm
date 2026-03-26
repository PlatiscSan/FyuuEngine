/* resource.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif
export module fyuu_rhi:resource;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :types;
import :enums;
import :logical_device;

namespace fyuu_rhi {

	export class Resource {
	private:
		UniqueResource m_impl;

	public:
		Resource(LogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags);

		Resource(LogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags);

		operator PolymorphicResourceBase*() const noexcept;

		PolymorphicResourceBase* GetHandle() const noexcept;
	};


} // namespace fyuu_rhi
