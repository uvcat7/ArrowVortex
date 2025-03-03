#pragma once

#include <Precomp.h>

namespace AV {
namespace Ascii {

// Matches range '0' to '9'.
constexpr bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Matches range 'a' to 'z'.
constexpr bool isLower(char c) { return c >= 'a' && c <= 'z'; }

// Matches range 'A' to 'Z'.
constexpr bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }

// Matches whitespace characters ' ', '\t', '\n', '\v', '\f', '\r'.
constexpr bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

// Converts range 'a' to 'z' into uppercase.
constexpr char toLower(char c) { return isUpper(c) ? c + 32 : c; }

// Converts range 'A' to 'Z' into lowercase.
constexpr char toUpper(char c) { return isLower(c) ? c - 32 : c; }

} // namespace Ascii
} // namespace AV