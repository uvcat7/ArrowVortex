#pragma once

#include <string>

namespace Vortex {

// Wide string, stores UTF-16.
class WideString {
   public:
    ~WideString();
    WideString();
    WideString(WideString&& s) noexcept;
    WideString(const WideString& s);
    WideString& operator=(WideString&& s) noexcept;
    WideString& operator=(const WideString& s);

    /// Constructs a string with character c repeated n times.
    WideString(int n, wchar_t c);

    /// Constructs a copy of String s.
    WideString(const wchar_t* s);

    /// Constructs a copy of the first n characters of str.
    WideString(const wchar_t* str, int n);

    /// Empties the string and releases the reserved memory.
    void clear();

    /// Sets the string to a copy of String s.
    void assign(WideString&& s);

    /// Sets the string to a copy of String s.
    void assign(const WideString& str);

    /// Sets the string to a copy of String s.
    void assign(const wchar_t* begin, const wchar_t* end);

    /// Swaps contents with another String.
    void swap(WideString& s);

    /// Appends character c at the end of the string.
    void push_back(wchar_t c);

    /// Appends String s at the end of the string.
    void append(const WideString& s);

    /// Appends String s at the end of the string.
    void append(const wchar_t* s);

    /// Appends String s at the end of the string.
    void append(const wchar_t* begin, const wchar_t* end);

    /// Returns the number of characters in the string.
    inline int size() const { return *((int*)widestring_ - 1); }

    /// Returns the number of characters in the string.
    inline int length() const { return size(); }

    /// Returns true if the string has a length of zero, false otherwise.
    inline bool empty() const { return !size(); }

    /// Returns a pointer to the string contents.
    inline const wchar_t* str() const { return widestring_; }

    /// Returns a pointer to the start of the string.
    inline wchar_t* begin() { return widestring_; }

    /// Returns a const pointer to the start of the string.
    inline const wchar_t* begin() const { return widestring_; }

    /// Returns a pointer to one character past the end of the string.
    inline wchar_t* end() { return widestring_ + size(); }

    /// Returns a const pointer to one character past the end of the string.
    inline const wchar_t* end() const { return widestring_ + size(); }

   private:
    wchar_t* widestring_;
};

// Wide String operators.
bool operator<(const WideString& a, const WideString& b);
bool operator==(const WideString& a, const WideString& b);

// Converts a wide String (UTF-16/UTF-32) to a string (UTF-8).
extern std::string Narrow(const wchar_t* str, int len);
extern std::string Narrow(const wchar_t* str);
extern std::string Narrow(const WideString& str);

// Converts a string (UTF-8) to a wide String (UTF-16/UTF-32).
extern WideString Widen(const char* str, int len);
extern WideString Widen(const char* str);
extern WideString Widen(const std::string& str);

};  // namespace Vortex
