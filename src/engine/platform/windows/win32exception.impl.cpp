module;
#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#endif // WIN32

module win32exception;

namespace fyuu_engine::windows {
#if WIN32
	Win32Exception::Win32Exception(DWORD error_code)
		: m_error_message()
#ifdef UNICODE
		, m_multi_bytes_error_message()
#endif // UNICODE
	{

		TCHAR* buffer = nullptr;
		DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS;

		DWORD size = FormatMessage(
			flags,
			nullptr,
			error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<TCHAR*>(&buffer),
			0,
			nullptr
		);

		if (size > 0 && buffer != nullptr) {

			std::size_t length = static_cast<std::size_t>(size) + 1;
			m_error_message.resize(length);
			std::memcpy(m_error_message.data(), buffer, length);
			LocalFree(buffer);

		}
		else {
			TCHAR const default_msg[] = TEXT("Unknown Win32 error\0");
			constexpr std::size_t length = sizeof(default_msg) / sizeof(TCHAR);
			m_error_message.resize(length, 0);
			std::memcpy(m_error_message.data(), default_msg, length);

		}

	}

	TCHAR const* Win32Exception::What() const noexcept {
		return m_error_message.data();
	}

	char const* Win32Exception::what() const noexcept {
#ifdef UNICODE
		if (m_multi_bytes_error_message.empty()) {
			m_multi_bytes_error_message = TStringToString(m_error_message.data());
		}
		return m_multi_bytes_error_message.data();
#else
		return m_error_message.data();
#endif // UNICODE
	}

#endif // WIN32
}