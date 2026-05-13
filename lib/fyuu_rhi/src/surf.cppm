module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:surface;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export template <class Backend> class Surface {
	public:
		using Implementation = typename Backend::Surface;
	private:
		Implementation m_impl;

	public:
		template <std::convertible_to<Implementation> I>
		Surface(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

	};

}