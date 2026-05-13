module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:shader_data_segment;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export template <class Backend> class ShaderDataSegment {
	public:
		using Implementation = typename Backend::ShaderDataSegment;
	private:
		Implementation m_impl;
	public:
		template <std::convertible_to<Implementation> I>
		ShaderDataSegment(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

	};

}