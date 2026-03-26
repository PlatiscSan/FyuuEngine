/* boost_locale.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string>
#include <string_view>
#include <memory_resource>
#endif // !defined(__cpp_lib_modules)
#include "boost.hpp"
module fyuu_rhi:boost_locale;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.other;

namespace fyuu_rhi {
	void InitializeBoostLocale() {
		plastic::utility::InitializeGlobalInstance(
			[]() {
				boost::locale::generator gen;
				std::locale::global(gen(""));
			}
		);
	}

	std::wstring UTF8ToUTF16(std::string_view view) {
		return boost::locale::conv::utf_to_utf<wchar_t>(view.data());
	}

	std::pmr::wstring UTF8ToUTF16(std::string_view view, std::pmr::polymorphic_allocator<wchar_t> alloc) {
		return boost::locale::conv::utf_to_utf<wchar_t>(view.data(), alloc);
	}

	std::string UTF16ToUTF8(std::wstring_view view) {
		return boost::locale::conv::utf_to_utf<char>(view.data());
	}

	std::pmr::string UTF8ToUTF16(std::wstring_view view, std::pmr::polymorphic_allocator<char> alloc) {
		return boost::locale::conv::utf_to_utf<char>(view.data(), alloc);
	}

}