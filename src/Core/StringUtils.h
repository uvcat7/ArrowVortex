#pragma once

#include <Core/Vector.h>

namespace Vortex {

struct Str
{
	/// Creates a string from a range of characters.
	static std::string create(const char* begin, const char* end);

	/// Sets the string to character c repeated n times.
	static void assign(std::string& s, int n, char c);

	/// Sets the string to a copy of str.
	static void assign(std::string& s, std::string&& str);

	/// Sets the string to a copy of str.
	static void assign(std::string& s, const std::string& str);

	/// Sets the string to a copy of str.
	static void assign(std::string& s, const char* str);

	/// Sets the string to the first n characters of str.
	static void assign(std::string& s, const char* str, int n);

	/// Appends character c at the end of the string.
	static void append(std::string& s, char c);

	/// Appends str at the end of the string.
	static void append(std::string& s, const std::string& str);

	/// Appends str at the end of the string.
	static void append(std::string& s, const char* str);

	/// Appends the first n characters from str at the end of the string.
	static void append(std::string& s, const char* str, int n);

	/// Inserts character c into the string at position pos.
	static void insert(std::string& s, int pos, char c);

	/// Inserts str into the string at position pos.
	static void insert(std::string& s, int pos, const std::string& str);

	/// Inserts str into the string at position pos.
	static void insert(std::string& s, int pos, const char* str);

	/// Inserts the first n characters from str into the string at position pos.
	static void insert(std::string& s, int pos, const char* str, int n);

	/// Makes sure the string is at most length n, removing characters at the end if necessary.
	static void truncate(std::string& s, int n);

	/// Makes sure the string is at least length n, appending c at the end if necessary.
	static void extend(std::string& s, int n, char c);

	/// Makes sure the string is exactly length n, truncating or extending with c if necessary.
	static void resize(std::string& s, int n, char c);

	/// Converts the string to an integer; returns alt on failure.
	static int readInt(const std::string& s, int alt = 0);

	/// Converts the string to an uinteger; returns alt on failure.
	static uint readUint(const std::string& s, uint32_t alt = 0);

	/// Converts the string to a float; returns alt on failure.
	static float readFloat(const std::string& s, float alt = 0);

	/// Converts the string to a double; returns alt on failure.
	static double readDouble(const std::string& s, double alt = 0);

	/// Converts the string to a boolean; returns alt on failure.
	static bool readBool(const std::string& s, bool alt = false);

	/// Converts the string to a timestamp double; returns alt on failure.
	static double readTime(const std::string& s, double alt = 0);

	/// Converts the string to a boolean; returns alt on failure.
	static bool read(const std::string& s, bool alt = false);

	/// Converts the string to an integer; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(const std::string& s, int* out);

	/// Converts the string to an uinteger; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(const std::string& s, uint32_t* out);

	/// Converts the string to a float; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(const std::string& s, float* out);

	/// Converts the string to a double; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(const std::string& s, double* out);

	/// Converts the string to a boolean; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(const std::string& s, bool* out);

	/// Returns the result of parsing the given algebraic expression.
	static bool parse(const char* expression, double& out);

	/// Removes whitespace at the start and end of the string.
	static void trim(std::string& s);

	/// Removes whitespace at the start and end of the string,
	/// and collapses internal whitespace to a single space.
	static void simplify(std::string& s);

	/// Erases a sequence of n characters starting at position pos.
	static void erase(std::string& s, int pos = 0, int n = std::string::npos);

	/// Removes the last character from the string.
	static void pop_back(std::string& s);

	/// Replaces every occurence of the find character with the replace character.
	static void replace(std::string& s, char find, char replace);

	/// Replaces every occurence of the find String with the replace string.
	static void replace(std::string& s, const char* find, const char* replace);

	/// Converts the alphabetic characters in the string String to upper case.
	static void toUpper(std::string& s);

	/// Converts the alphabetic characters in the string String to lower case.
	static void toLower(std::string& s);

	/// Returns a substring of the string starting at position pos.
	/// The substring spans n characters (or until the end of the string, whichever comes first).
	static std::string substr(const std::string& s, int pos = 0, int n = std::string::npos);

	/// Interprets the string as UTF-8 and returns the position of the next character after pos,
	/// which may be an offset of more than one when dealing with multibyte UTF-8 characters.
	/// If the next character is past the end of the string, npos is returned.
	static int nextChar(const std::string& s, int pos = 0);
	
	/// Interprets the string as UTF-8 and returns the position of the previous character before
	/// pos, which may be an offset of more than one when dealing with multibyte UTF-8 characters.
	/// If the previous character is before the start of the string, -1 is returend.
	static int prevChar(const std::string& s, int pos = std::string::npos);

	/// Returns true if the string contains non-ascii characters.
	static bool isUnicode(const std::string& s);

	/// Searches the string for character c.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int find(const std::string& s, char c, int pos = 0);

	/// Searches the string for substring s.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int find(const std::string& s, const char* str, int pos = 0);

	/// Searches the string for character c.
	/// Returns the position of the last occurence on or before pos, or -1 if no match is found.
	static int findLast(const std::string& s, char c, int pos = std::string::npos);

	/// Searches the string for any of the characters in c.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int findAnyOf(const std::string& s, const char* c, int pos = 0);

	/// Searches the string for any of the characters in c.
	/// Returns the position of the last occurence on or before pos, or -1 if no match is found.
	static int findLastOf(const std::string& s, const char* c, int pos = std::string::npos);

	// Returns true if the end of the string matches suffix.
	static bool endsWith(const std::string& s, const char* suffix, bool caseSensitive = true);

	// Returns true if the start of the string matches prefix.
	static bool startsWith(const std::string& s, const char* prefix, bool caseSensitive = true);

	// Comparison using strcmp, case-sensitive.
	static int compare(const std::string& a, const std::string& b);
	static int compare(const std::string& a, const char* b);
	static int compare(const char* a, const char* b);

	// Comparison using stricmp, not case-sensitive.
	static int icompare(const std::string& a, const std::string& b);
	static int icompare(const std::string& a, const char* b);
	static int icompare(const char* a, const char* b);

	// Equivalence check using strcmp, case-sensitive.
	static bool equal(const std::string& a, const std::string& b);
	static bool equal(const std::string& a, const char* b);
	static bool equal(const char* a, const char* b);

	// Equivalence check using stricmp, not case-sensitive.
	static bool iequal(const std::string& a, const std::string& b);
	static bool iequal(const std::string& a, const char* b);
	static bool iequal(const char* a, const char* b);

	/// Converts the given value to a string.
	static std::string val(int v, int minDigits = 0, bool hex = false);
	static std::string val(uint v, int minDigits = 0, bool hex = false);
	static std::string val(float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
	static std::string val(double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

	/// Converts the given value and appends it to the string.
	static void appendVal(std::string& s, int v, int minDigits = 0, bool hex = false);
	static void appendVal(std::string& s, uint v, int minDigits = 0, bool hex = false);
	static void appendVal(std::string& s, float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
	static void appendVal(std::string& s, double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

	/// Helper struct used in string formatting.
	struct fmt
	{
		fmt(const std::string& format);
		fmt(const char* format);

		fmt& arg(char c);
		fmt& arg(const std::string& s);
		fmt& arg(const char* s);
		fmt& arg(const char* s, size_t n);
		fmt& arg(int v, int minDigits = 0, bool hex = false);
		fmt& arg(uint32_t v, int minDigits = 0, bool hex = false);
		fmt& arg(float v, int minDecimals = 0, int maxDecimals = 6);
		fmt& arg(double v, int minDecimals = 0, int maxDecimals = 6);

		inline operator const char*() { return str.data(); }
		inline operator std::string&() { return str; }

		std::string str;
	};

	/// Returns a formatted string of the given time.
	static std::string formatTime(double seconds, bool showMilliseconds = true);

	/// Splits a string into a string list based on word boundaries.
	/// For example, SplitString("Hello World") returns {"Hello", "World"}.
	static Vector<std::string> split(const std::string& s);

	/// Splits a string into a string list based on a delimiter.
	/// For example, SplitString("Hello ~ World", "~") returns {"Hello", "World"}.
	/// If trim is true, whitespace surrounding each element is removed.
	/// If skipEmpty is true, empty elements are not added to the list.
	static Vector<std::string> split(const std::string& s, const char* delimiter, bool trim = true, bool skipEmpty = true);

	/// Returns the concatenation of the list of strings, seperated by the given delimiter.
	static std::string join(const Vector<std::string>& list, const char* delimiter);
};

}; // namespace Vortex
