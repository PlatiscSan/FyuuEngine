module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:gpu_executable;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export template <class Backend> class GPUExecutable {
	public:
		using Implementation = typename Backend::GPUExecutable;
	private:
		Implementation m_impl;
	public:
		template <std::convertible_to<Implementation> I>
		GPUExecutable(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

	};

}