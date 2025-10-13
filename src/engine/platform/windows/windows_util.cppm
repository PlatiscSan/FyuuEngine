module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

export module windows_utils;

import std;
export import synchronized_function;

namespace fyuu_engine::windows {
#ifdef WIN32
	export extern concurrency::SynchronizedFunction<boost::uuids::uuid()> GenerateUUID;

#ifdef UNICODE
	export std::string TStringToString(std::wstring_view str);
	export std::wstring StringToTString(std::string_view str);
#else
	export inline std::string_view TStringToString(std::string_view str) {
		return str;
	}
	export inline std::string_view StringToTString(std::string_view str) {
		return str;
	}
#endif // UNICODE
#endif // WIN32
}