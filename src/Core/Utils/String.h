#pragma once

#include <Precomp.h>

namespace AV {
namespace String {

// String-to-value conversion:
	
// Low-level parsing functions.
// On success: returns true, <outValue> contains the read value, <outCharsRead> contains the number of read chars.
// On failure: returns false, <outValue> remains unchanged, <outCharsRead> is set to zero.

bool readInt(const char* s, int& outValue, int& outCharsRead);
bool readUint(const char* s, uint& outValue, int& outCharsRead);
bool readFloat(const char* s, float& outValue, int& outCharsRead);
bool readDouble(const char* s, double& outValue, int& outCharsRead);
bool readBool(const char* s, bool& outValue, int& outCharsRead);

// String-to-value with out parameter.
// On success: returns true, <outValue> contains the converted value.
// On failure: returns false, <outValue> remains unchanged.

bool readInt(stringref s, int& out);
bool readUint(stringref s, uint& out);
bool readFloat(stringref s, float& out);
bool readDouble(stringref s, double& out);
bool readBool(stringref s, bool& out);

// String-to-value conversion with fallback.
// On success: returns the converted value.
// On failure: returns the <fallback> value.

int toInt(stringref s, int fallback);
uint toUint(stringref s, uint fallback);
float toFloat(stringref s, float fallback);
double toDouble(stringref s, double fallback);
bool toBool(stringref s, bool fallback);

// String-to-value with multiple out values.
// On success: returns true, <outValues> contains the converted values.
// On failure: returns false, <outValues> remains unchanged.

bool readInts(stringref s, int* out, size_t n, char separator = ',');
bool readUints(stringref s, uint* out, size_t n, char separator = ',');
bool readFloats(stringref s, float* out, size_t n, char separator = ',');
bool readDoubles(stringref s, double* out, size_t n, char separator = ',');
bool readBools(stringref s, bool* out, size_t n, char separator = ',');

template <size_t N>
bool readInts(stringref s, int(&out)[N], char separator = ',') { return readInts(s, out, N, separator); }
template <size_t N>
bool readUints(stringref s, uint(&out)[N], char separator = ',') { return readUints(s, out, N, separator); }
template <size_t N>
bool readFloats(stringref s, float(&out)[N], char separator = ',') { return readFloats(s, out, N, separator); }
template <size_t N>
bool readDoubles(stringref s, double(&out)[N], char separator = ',') { return readDoubles(s, out, N, separator); }
template <size_t N>
bool readBools(stringref s, bool(&out)[N], char separator = ',') { return readBools(s, out, N, separator); }

// Returns the result of evaluating the given algebraic expression.
bool evaluateExpression(stringref s, double& out);

// String information:

// Searches the string for character <c>.
// Returns the position of the first occurence on or after <pos>, or npos if no match is found.
size_t find(stringref s, char c, size_t pos = 0);

// Searches the string for substring <s>.
// Returns the position of the first occurence on or after <pos>, or npos if no match is found.
size_t find(stringref s, const char* str, size_t pos = 0);

// Searches the string for character <c>.
// Returns the position of the last occurence on or before <pos>, or npos if no match is found.
size_t findLast(stringref s, char c, size_t pos = std::string::npos);

// Searches the string for any of the characters in <c>.
// Returns the position of the first occurence on or after <pos>, or npos if no match is found.
size_t findAnyOf(stringref s, const char* c, size_t pos = 0);

// Searches the string for any of the characters in <c>.
// Returns the position of the last occurence on or before <pos>, or npos if no match is found.
size_t findLastOf(stringref s, const char* c, size_t pos = std::string::npos);

// Returns true if the end of the string matches <suffix>.
bool endsWith(stringref s, const char* suffix, bool caseSensitive = true);

// Returns true if the start of the string matches <prefix>.
bool startsWith(stringref s, const char* prefix, bool caseSensitive = true);

// Returns the substring up to the first whitespace run, or the end if no whitespace occurs.
std::string head(stringref s);

// Returns the substring starting after the first whitespace run, or an empty string if no whitespace occurs.
std::string tail(stringref s);

// String manipulation:
	
// Replaces all occurrences of <find> with <replace>.
void replace(std::string& s, const char* find, const char* replace);

// Makes sure the string is at most length <n>, removing characters at the end if necessary.
void truncate(std::string& s, size_t n);

// Converts the alphabetic characters in the string string to upper case.
void toUpper(std::string& s);

// Converts the alphabetic characters in the string string to lower case.
void toLower(std::string& s);

// Collapses every whitespace substrings into a single space.
void collapseWhitespace(std::string& s);

// Splits a string into a string list based on word boundaries.
// For example, SplitString("Hello World") returns {"Hello", "World"}.
vector<std::string> split(stringref s);

// Splits a string into a string list based on a delimiter.
// For example, SplitString("Hello ~ World", "~") returns {"Hello", "World"}.
// If trim is true, whitespace surrounding each element is removed.
// If skipEmpty is true, empty elements are not added to the list.
vector<std::string> split(stringref s, const char* delimiter, bool trim = true, bool skipEmpty = true);

// Splits a string into two parts separated by whitespace.
// For example, splitting "A B C" returns "A" and "B C".
bool splitOnce(stringref s, std::string& outLeft, std::string& outRight);

// Splits a string into two parts based on a delimiter.
// For example, splitting "A,B,C" with delimiter "," returns "A" and "B,C".
bool splitOnce(stringref s, std::string& outLeft, std::string& outRight, const char* delimiter, bool trim = true);

// Returns the concatenation of the list of strings, seperated by the given delimiter.
std::string join(const vector<std::string>& list, const char* delimiter);

// String comparison:
	
// Equivalence check using strcmp, case-sensitive.
bool equal(const char* a, const char* b);

// Equivalence check using stricmp, case-insensitive.
bool iequal(stringref a, stringref b);
bool iequal(stringref a, const char* b);
bool iequal(const char* a, stringref b);
bool iequal(const char* a, const char* b);
	
// Value-to-string conversion:
	
// Converts the given value to a string.
std::string fromBool(bool v, bool upperCase = false);
std::string fromInt(int64_t v, int minDigits = 0, bool hex = false);
std::string fromUint(uint64_t v, int minDigits = 0, bool hex = false);
std::string fromFloat(float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
std::string fromDouble(double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

// Converts the given value and appends it to the string.
void appendBool(std::string& s, bool v, bool upperCase = false);
void appendInt(std::string& s, int64_t v, int minDigits = 0, bool hex = false);
void appendUint(std::string& s, uint64_t v, int minDigits = 0, bool hex = false);
void appendFloat(std::string& s, float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
void appendDouble(std::string& s, double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

// String formatting:

// Returns a formatted string of the given startTime.
std::string formatTime(double seconds, bool showMilliseconds = true);

} // namespace String
} // namespace AV