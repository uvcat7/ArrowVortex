#pragma once

#include <string>

namespace Vortex {

// Converts a wide String (UTF-16) to a string (UTF-8).
extern std::string Narrow(const wchar_t* str, int len);
extern std::string Narrow(const wchar_t* str);
extern std::string Narrow(const std::wstring& str);

// Converts a string (UTF-8) to a wide String (UTF-16).
extern std::wstring Widen(const char* str, int len);
extern std::wstring Widen(const char* str);
extern std::wstring Widen(const std::string& str);

};  // namespace Vortex
