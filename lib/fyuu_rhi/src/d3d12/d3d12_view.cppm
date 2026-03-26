module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <variant>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
export module fyuu_rhi:d3d12_view;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_descriptor_allocator;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	export class D3D12View
		: public PolymorphicViewBase,
		public D3D12Common<D3D12View> {
	private:
		using Handle = std::variant<
			std::monostate,
			D3D12UniversalViewAllocator::Allocation,
			D3D12RTVAllocator::Allocation,
			D3D12DSVAllocator::Allocation
		>;

		Handle m_handle;

	public:
		D3D12View(D3D12LogicalDevice const& logical_device, D3D12Resource const& resource, BufferSize buffer_size, BufferSize offset);

		D3D12View(D3D12LogicalDevice const& logical_device, D3D12Resource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count);

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetNative() const noexcept;

	};

}

#endif // defined(_WIN32)