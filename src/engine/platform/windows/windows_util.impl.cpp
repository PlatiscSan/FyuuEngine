module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

module windows_utils;

namespace fyuu_engine::windows {
#ifdef WIN32
	concurrency::SynchronizedFunction<boost::uuids::uuid()> GenerateUUID = []() {
		static boost::uuids::random_generator gen;
		return gen();
		};

#ifdef UNICODE
	std::string TStringToString(std::wstring_view src) {

		int utf8_len = WideCharToMultiByte(
			CP_UTF8,
			0,
			reinterpret_cast<LPCWSTR>(src.data()),
			static_cast<int>(src.size()),
			nullptr,
			0,
			nullptr,
			nullptr
		);
		if (utf8_len <= 0) {
			return "Win32Exception: Encoding conversion failed.\0";
		}

		std::string output(static_cast<std::size_t>(utf8_len) + 1u, 0);
		WideCharToMultiByte(
			CP_UTF8,
			0,
			reinterpret_cast<LPCWSTR>(src.data()),
			static_cast<int>(src.size()),
			output.data(),
			utf8_len,
			nullptr,
			nullptr
		);
		output[utf8_len] = '\0';
		return output;
	}

	std::wstring StringToTString(std::string_view src) {
		if (src.empty()) {
			return std::basic_string<TCHAR>();
		}

		int wide_len = MultiByteToWideChar(
			CP_UTF8,
			0,
			src.data(),
			static_cast<int>(src.size()),
			nullptr,
			0
		);

		if (wide_len <= 0) {
			return _T("Win32Exception: Encoding conversion failed.");
		}

		std::basic_string<TCHAR> output;
		output.resize(static_cast<size_t>(wide_len));

		int result = MultiByteToWideChar(
			CP_UTF8,
			0,
			src.data(),
			static_cast<int>(src.size()),
			output.data(),
			wide_len
		);

		if (result == 0) {
			return _T("Win32Exception: Encoding conversion failed.");
		}

		return output;

	}
#endif //UNICODE
#endif // WIN32
}