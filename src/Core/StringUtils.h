#pragma once

#include <Core/String.h>
#include <Core/Vector.h>

namespace Vortex {

struct Str
{
	/// Creates a string from a range of characters.
	static String create(const char* begin, const char* end);

	/// Sets the string to character c repeated n times.
	static void assign(String& s, int n, char c);

	/// Sets the string to a copy of str.
	static void assign(String& s, String&& str);

	/// Sets the string to a copy of str.
	static void assign(String& s, StringRef str);

	/// Sets the string to a copy of str.
	static void assign(String& s, const char* str);

	/// Sets the string to the first n characters of str.
	static void assign(String& s, const char* str, int n);

	/// Appends character c at the end of the string.
	static void append(String& s, char c);

	/// Appends str at the end of the string.
	static void append(String& s, StringRef str);

	/// Appends str at the end of the string.
	static void append(String& s, const char* str);

	/// Appends the first n characters from str at the end of the string.
	static void append(String& s, const char* str, int n);

	/// Inserts character c into the string at position pos.
	static void insert(String& s, int pos, char c);

	/// Inserts str into the string at position pos.
	static void insert(String& s, int pos, StringRef str);

	/// Inserts str into the string at position pos.
	static void insert(String& s, int pos, const char* str);

	/// Inserts the first n characters from str into the string at position pos.
	static void insert(String& s, int pos, const char* str, int n);

	/// Makes sure the string is at most length n, removing characters at the end if necessary.
	static void truncate(String& s, int n);

	/// Makes sure the string is at least length n, appending c at the end if necessary.
	static void extend(String& s, int n, char c);

	/// Makes sure the string is exactly length n, truncating or extending with c if necessary.
	static void resize(String& s, int n, char c);

	/// Converts the string to an integer; returns alt on failure.
	static int readInt(StringRef s, int alt = 0);

	/// Converts the string to an uinteger; returns alt on failure.
	static uint readUint(StringRef s, uint alt = 0);

	/// Converts the string to a float; returns alt on failure.
	static float readFloat(StringRef s, float alt = 0);

	/// Converts the string to a double; returns alt on failure.
	static double readDouble(StringRef s, double alt = 0);

	/// Converts the string to a boolean; returns alt on failure.
	static bool readBool(StringRef s, bool alt = false);

	/// Converts the string to a boolean; returns alt on failure.
	static bool read(StringRef s, bool alt = false);

	/// Converts the string to an integer; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(StringRef s, int* out);

	/// Converts the string to an uinteger; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(StringRef s, uint* out);

	/// Converts the string to a float; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(StringRef s, float* out);

	/// Converts the string to a double; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(StringRef s, double* out);

	/// Converts the string to a boolean; returns true on success.
	/// On failure, out remains unchanged.
	static bool read(StringRef s, bool* out);

	/// Returns the result of parsing the given algebraic expression.
	static bool parse(const char* expression, double& out);

	/// Removes whitespace at the start and end of the string.
	static void trim(String& s);

	/// Removes whitespace at the start and end of the string,
	/// and collapses internal whitespace to a single space.
	static void simplify(String& s);

	/// Erases a sequence of n characters starting at position pos.
	static void erase(String& s, int pos = 0, int n = String::npos);

	/// Removes the last character from the string.
	static void pop_back(String& s);

	/// Replaces every occurence of the find character with the replace character.
	static void replace(String& s, char find, char replace);

	/// Replaces every occurence of the find String with the replace string.
	static void replace(String& s, const char* find, const char* replace);

	/// Converts the alphabetic characters in the string String to upper case.
	static void toUpper(String& s);

	/// Converts the alphabetic characters in the string String to lower case.
	static void toLower(String& s);

	/// Returns a substring of the string starting at position pos.
	/// The substring spans n characters (or until the end of the string, whichever comes first).
	static String substr(StringRef s, int pos = 0, int n = String::npos);

	/// Interprets the string as UTF-8 and returns the position of the next character after pos,
	/// which may be an offset of more than one when dealing with multibyte UTF-8 characters.
	/// If the next character is past the end of the string, npos is returned.
	static int nextChar(StringRef s, int pos = 0);
	
	/// Interprets the string as UTF-8 and returns the position of the previous character before
	/// pos, which may be an offset of more than one when dealing with multibyte UTF-8 characters.
	/// If the previous character is before the start of the string, -1 is returend.
	static int prevChar(StringRef s, int pos = String::npos);

	/// Returns true if the string contains non-ascii characters.
	static bool isUnicode(StringRef s);

	/// Searches the string for character c.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int find(StringRef s, char c, int pos = 0);

	/// Searches the string for substring s.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int find(StringRef s, const char* str, int pos = 0);

	/// Searches the string for character c.
	/// Returns the position of the last occurence on or before pos, or -1 if no match is found.
	static int findLast(StringRef s, char c, int pos = String::npos);

	/// Searches the string for any of the characters in c.
	/// Returns the position of the first occurence on or after pos, or npos if no match is found.
	static int findAnyOf(StringRef s, const char* c, int pos = 0);

	/// Searches the string for any of the characters in c.
	/// Returns the position of the last occurence on or before pos, or -1 if no match is found.
	static int findLastOf(StringRef s, const char* c, int pos = String::npos);

	// Returns true if the end of the string matches suffix.
	static bool endsWith(StringRef s, const char* suffix, bool caseSensitive = true);

	// Returns true if the start of the string matches prefix.
	static bool startsWith(StringRef s, const char* prefix, bool caseSensitive = true);

	// Comparison using strcmp, case-sensitive.
	static int compare(StringRef a, StringRef b);
	static int compare(StringRef a, const char* b);
	static int compare(const char* a, const char* b);

	// Comparison using stricmp, not case-sensitive.
	static int icompare(StringRef a, StringRef b);
	static int icompare(StringRef a, const char* b);
	static int icompare(const char* a, const char* b);

	// Equivalence check using strcmp, case-sensitive.
	static bool equal(StringRef a, StringRef b);
	static bool equal(StringRef a, const char* b);
	static bool equal(const char* a, const char* b);

	// Equivalence check using stricmp, not case-sensitive.
	static bool iequal(StringRef a, StringRef b);
	static bool iequal(StringRef a, const char* b);
	static bool iequal(const char* a, const char* b);

	/// Converts the given value to a string.
	static String val(int v, int minDigits = 0, bool hex = false);
	static String val(uint v, int minDigits = 0, bool hex = false);
	static String val(float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
	static String val(double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

	/// Converts the given value and appends it to the string.
	static void appendVal(String& s, int v, int minDigits = 0, bool hex = false);
	static void appendVal(String& s, uint v, int minDigits = 0, bool hex = false);
	static void appendVal(String& s, float v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);
	static void appendVal(String& s, double v, int minDecimalPlaces = 0, int maxDecimalPlaces = 6);

	/// Helper struct used in string formatting.
	struct fmt
	{
		fmt(StringRef format);
		fmt(const char* format);

		fmt& arg(char c);
		fmt& arg(StringRef s);
		fmt& arg(const char* s);
		fmt& arg(const char* s, int n);
		fmt& arg(int v, int minDigits = 0, bool hex = false);
		fmt& arg(uint v, int minDigits = 0, bool hex = false);
		fmt& arg(float v, int minDecimals = 0, int maxDecimals = 6);
		fmt& arg(double v, int minDecimals = 0, int maxDecimals = 6);

		inline operator const char*() { return str.str(); }
		inline operator String&() { return str; }

		String str;
	};

	/// Returns a formatted string of the given time.
	static String formatTime(double seconds, bool showMilliseconds = true);

	/// Splits a string into a string list based on word boundaries.
	/// For example, SplitString("Hello World") returns {"Hello", "World"}.
	static Vector<String> split(StringRef s);

	/// Splits a string into a string list based on a delimiter.
	/// For example, SplitString("Hello ~ World", "~") returns {"Hello", "World"}.
	/// If trim is true, whitespace surrounding each element is removed.
	/// If skipEmpty is true, empty elements are not added to the list.
	static Vector<String> split(StringRef s, const char* delimiter, bool trim = true, bool skipEmpty = true);

	/// Returns the concatenation of the list of strings, seperated by the given delimiter.
	static String join(const Vector<String>& list, const char* delimiter);
};

// Concatenation operators.

String operator + (StringRef a, char b);
String operator + (char a, StringRef b);
String operator + (StringRef a, const char* b);
String operator + (const char* a, StringRef b);
String operator + (StringRef a, StringRef b);

String& operator += (String& a, char b);
String& operator += (String& a, const char* b);
String& operator += (String& a, StringRef b);

// Alternative lexographical comparison operators.

bool operator < (StringRef a, const char* b);
bool operator < (const char* a, StringRef b);

bool operator > (StringRef a, const char* b);
bool operator > (const char* a, StringRef b);
bool operator > (StringRef a, StringRef b);

bool operator == (StringRef a, const char* b);
bool operator == (const char* a, StringRef b);

bool operator != (StringRef a, const char* b);
bool operator != (const char* a, StringRef b);
bool operator != (StringRef a, StringRef b);

}; // namespace Vortex
