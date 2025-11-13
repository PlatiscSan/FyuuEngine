export module reflective:nameof;
import <nameof.hpp>;

namespace fyuu_engine::reflection {
	export template <class T> constexpr std::string_view NameOf() {
		return NAMEOF_TYPE(T);
	}
}