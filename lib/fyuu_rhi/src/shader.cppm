module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:shader;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export template <class Backend> class Shader {
	public:
		using Implementation = typename Backend::Shader;
	private:
		Implementation m_impl;
	public:
		template <std::convertible_to<Implementation> I>
		Shader(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

	};

}