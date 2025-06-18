#pragma once

#include <Core/Core.h>

namespace Vortex {

// Basic string, stores UTF-8.
class String
{
public:
	~String();
	String();
	String(String&& s);
	String(StringRef s);
	String& operator = (String&& s);
	String& operator = (StringRef s);

	/// An arbitrary large value; used to indicate end-of-range.
	static const int npos;

	/// Sets the string to character c, repeated n times.
	String(int n, char c);

	/// Sets the string to a copy of str.
	String(const char* str);

	/// Sets the string to the first n characters of str.
	String(const char* str, int n);

	/// Swaps contents with another String.
	void swap(String& s);

	/// Empties the string but keeps the reserved memory.
	void clear();

	/// Empties the string and releases the reserved memory.
	void release();

	/// Returns the number of characters in the string.
	inline int len() const { return *((int*)myStr - 1); }

	/// Returns true if the string has a length of zero, false otherwise.
	inline bool empty() const { return !len(); }

	/// Returns a pointer to the start of the string.
	inline const char* str() const { return myStr; }

	/// Returns a pointer to the start of the string.
	inline char* begin() { return myStr; }

	/// Returns a const pointer to the start of the string.
	inline const char* begin() const { return myStr; }

	/// Returns a pointer to one character past the end of the string.
	inline char* end() { return myStr + len(); }

	/// Returns a const pointer to one character past the end of the string.
	inline const char* end() const { return myStr + len(); }

	/// Returns the first character of the string; does not perform an out-of-bounds check.
	inline char front() const { return myStr[0]; }

	/// Returns the final character of the string; does not perform an out-of-bounds check.
	inline char back() const { return myStr[len() - 1]; }

	/// Returns the character at position pos; does not perform an out-of-bounds check.
	inline char operator [] (int pos) const { return myStr[pos]; }

	// Friend structs used in StringUtils.
	friend struct Str2;
	friend struct Str;

private:
	char* myStr;
};

// Basic lexographical comparison operators.

bool operator < (StringRef a, StringRef b);
bool operator == (StringRef a, StringRef b);

}; // namespace Vortex
