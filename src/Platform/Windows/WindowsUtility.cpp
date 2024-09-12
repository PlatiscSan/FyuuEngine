
#include "pch.h"
#include "WindowsUtility.h"

std::string Fyuu::utility::windows::GetLastErrorMessageFromWinAPI() {

    DWORD raw_error = GetLastError();
    if (raw_error == 0) {  
        return "No error occurred";
    }

    char* msg_buff = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        raw_error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&msg_buff,
        0,
        nullptr);

    if (size == 0) {
        DWORD format_error = GetLastError();
        throw std::runtime_error("Failed to format message. Error code: " + std::to_string(format_error));
    }

    std::string error_str(msg_buff, size);

    LocalFree(msg_buff);

    return error_str;


}

std::variant<std::string, std::wstring> Fyuu::utility::windows::ConvertString(std::string const& str) {

#if defined(_UNICODE)
    int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.size()), nullptr, 0);
    if (size_needed == 0) {
        throw std::runtime_error("MultiByteToWideChar failed");
    }
    std::wstring wstr(size_needed, 0);
    if (!MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.size()), &wstr[0], size_needed)) {
        throw std::runtime_error("MultiByteToWideChar failed");
    }
    return wstr;
#else
    return str;
#endif // defined(_UNICODE)


}

std::string Fyuu::utility::windows::WStringToString(std::wstring const& wstr) {

    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed == 0) {
        throw std::runtime_error("WideCharToMultiByte failed");
    }

    std::string str(size_needed, '\0');
  
    if (!WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), &str[0], size_needed, nullptr, nullptr)) {
        throw std::runtime_error("WideCharToMultiByte failed");
    }

    str.resize(size_needed);

    return str;

}
