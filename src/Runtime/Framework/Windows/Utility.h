#ifndef UTILITY_H
#define UTILITY_H

#include <string>

#include <Windows.h>

namespace Fyuu {

	namespace windows_util {

		std::string GetLastErrorFromWinAPI();
		std::variant<std::string, std::wstring> ConvertString(std::string str);

	#if defined(_UNICODE) || defined(UNICODE)
		using String = std::wstring;
	#else
		using String = std::string;
	#endif // defined(_UNICODE) || defined(UNICODE)


	}

}

#endif // !UTILITY_H
