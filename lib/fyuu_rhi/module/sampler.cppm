/* sampler.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif
export module fyuu_rhi:sampler;
#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif
import :types;
import :enums;
import :logical_device;

namespace fyuu_rhi {

	export class Sampler {
	private:
		UniqueSampler m_impl;

	public:
		Sampler(LogicalDevice const& logical_device, SamplerFlags flags, SamplerAttachmentInfo const& attachment);

		PolymorphicSamplerBase* GetHandle() const noexcept;

	};


} // namespace fyuu_rhi
