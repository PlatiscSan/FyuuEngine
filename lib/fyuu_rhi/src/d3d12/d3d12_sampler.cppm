module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <variant>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
export module fyuu_rhi:d3d12_sampler;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :d3d12_common;
import :d3d12_descriptor_allocator;

namespace fyuu_rhi::d3d12 {

	export class D3D12Sampler
		: public PolymorphicSamplerBase,
		public D3D12Common<D3D12Sampler> {
	private:
		D3D12SamplerAllocator::Allocation m_alloc;
		
	public:
		D3D12Sampler(D3D12LogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment);

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetNative() const noexcept;

	};


}

#endif // defined(_WIN32)