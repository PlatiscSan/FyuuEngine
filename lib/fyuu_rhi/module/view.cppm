/* view.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif
export module fyuu_rhi:view;
#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :enums;
import :logical_device;
import :resource;

namespace fyuu_rhi {

	export class View {
	private:
		std::shared_ptr<PolymorphicViewBase> m_impl;

	public:
		View(LogicalDevice const& logical_device, Resource const& resource, BufferSize buffer_size, BufferSize offset);

		View(LogicalDevice const& logical_device, Resource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count);

		operator PolymorphicViewBase* () const noexcept;

		PolymorphicViewBase* GetHandle() const noexcept;

	};

} // namespace fyuu_rhi
