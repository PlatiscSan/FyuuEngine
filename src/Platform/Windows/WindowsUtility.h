#ifndef WINDOWS_UTILITY_H
#define WINDOWS_UTILITY_H

#include <stdexcept>
#include <string>
#include <variant>

#include <Windows.h>
#include <windowsx.h>

namespace Fyuu::windows_util {

#if defined(_UNICODE) || defined(UNICODE)
	using String = std::wstring;
#else
	using String = std::string;
#endif // defined(_UNICODE) || defined(UNICODE)

	std::string GetLastErrorFromWinAPI();
	std::variant<std::string, std::wstring> ConvertString(std::string const& str);
	std::string WStringToString(std::wstring const& wstr);


}

#endif // !WINDOWS_UTILITY_H
