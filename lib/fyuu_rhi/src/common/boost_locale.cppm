export module fyuu_rhi:boost_locale;
import std;

namespace fyuu_rhi::common {
	export extern void InitializeBoostLocale();

	export std::wstring UTF8ToUTF16(std::string_view view);
	export std::pmr::wstring UTF8ToUTF16(std::string_view view, std::pmr::polymorphic_allocator<wchar_t> alloc);

	export std::string UTF16ToUTF8(std::wstring_view view);
	export std::pmr::string UTF8ToUTF16(std::wstring_view view, std::pmr::polymorphic_allocator<char> alloc);

}