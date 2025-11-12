#include <Core/WideString.h>

#include <Core/StringUtils.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Vortex {

std::string Narrow(const wchar_t* s, int len) {
    if (s == nullptr || len <= 0) return std::string();
    auto new_size =
        WideCharToMultiByte(CP_UTF8, 0, s, len, nullptr, 0, nullptr, nullptr);
    std::string out(new_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, s, len, out.data(), out.size(), nullptr,
                        nullptr);
    return out;
}

std::wstring Widen(const char* s, int len) {
    std::wstring out;
    if (s == nullptr || len <= 0) return out;
    auto new_size = MultiByteToWideChar(CP_UTF8, 0, s, len, nullptr, 0);
    std::wstring out_str(new_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s, len, &out_str[0], new_size);
    out.assign(out_str.data(), out_str.data() + out_str.size());
    return out;
}

std::string Narrow(const wchar_t* s) { return Narrow(s, wcslen(s)); }

std::string Narrow(const std::wstring& s) {
    return Narrow(s.c_str(), s.size());
}

std::wstring Widen(const char* s) { return Widen(s, strlen(s)); }

std::wstring Widen(const std::string& s) { return Widen(s.data(), s.length()); }

};  // namespace Vortex
