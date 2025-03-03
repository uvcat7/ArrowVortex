#pragma once

#include <Precomp.h>

namespace AV {
namespace Unicode {

// Constants to subtract from decoded UTF-8 codepoint to cancel out non-codepoint bits.
// Indexed by number of trailing bytes (0-5).
extern const uint32_t utf8MultibyteResidu[6];

// Indicates how many trailing bytes follow a leading byte of UTF-8 codepoint.
// Indexed by the leading byte (0-255).
extern const uint8_t utf8TrailingBytes[256];

// Constants to add to the leading byte of UTF-8 codepoint to indicate trailing bytes.
// Indexed by the total number of bytes (1-6).
extern const uint8_t utf8EncodeConstants[7];

// Converts a c-style wide string (UTF-16/UTF-32) to a string (UTF-8).
std::string narrow(const wchar_t* str, size_t len);
std::string narrow(const wchar_t* str);
std::string narrow(const std::wstring& str);

// Converts a string (UTF-8) to a wide string (UTF-16).
std::wstring widen(const char* str, size_t len);
std::wstring widen(const char* str);
std::wstring widen(stringref str);

// Returns true if the string contains non-ascii characters.
bool isNonAscii(stringref s);

// Interprets the string as UTF-8 and returns the position of the next character after pos,
// which may be an offset of more than one when dealing with multibyte UTF-8 characters.
// If the next character is past the end of the string, the string length is returned.
int64_t nextChar(stringref s, int64_t pos = 0);

// Interprets the string as UTF-8 and returns the position of the previous character before
// pos, which may be an offset of more than one when dealing with multibyte UTF-8 characters.
// If the previous character is before the start of the string, -1 is returend.
int64_t prevChar(stringref s, int64_t pos = INT64_MAX);

} // namespace Unicode
} // namespace AV