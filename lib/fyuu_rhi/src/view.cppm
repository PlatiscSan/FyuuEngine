module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:view;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export template <class Backend> class View {
	public:
		using Implementation = typename Backend::View;
	private:
		Implementation m_impl;
	public:
		template <std::convertible_to<Implementation> I>
		View(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

	};

}