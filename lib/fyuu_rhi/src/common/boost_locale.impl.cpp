module;
#include <boost/locale.hpp>

module fyuu_rhi:boost_locale;
import plastic.other;

namespace fyuu_rhi::common {
	
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