module;
#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#endif // WIN32

export module win32exception;
import std;

namespace fyuu_engine::windows {
	export class Win32Exception : public std::exception {
	private:
		std::vector<TCHAR> m_error_message;
#ifdef UNICODE
		mutable std::string m_multi_bytes_error_message;
#endif // UNICODE

	public:
		explicit Win32Exception(DWORD error_code = ::GetLastError());
		TCHAR const* What() const noexcept;
		char const* what() const noexcept override;

	};
}