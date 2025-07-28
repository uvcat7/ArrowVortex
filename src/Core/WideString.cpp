#include <Core/WideString.h>

#include <Core/StringUtils.h>

#include <stdlib.h>
#include <string.h>

namespace Vortex {

namespace {

// ================================================================================================
// Reference management functions
// cap[0] : capacity, cap[1] : length

static const int metaSize = sizeof(int) * 2;
static char metaData[metaSize + sizeof(wchar_t)] = {};
static wchar_t* nullstr = (wchar_t*)(metaData + metaSize);

static void StrAlloc(wchar_t*& s, int n) {
    int* cap = (int*)malloc(metaSize + sizeof(wchar_t) * (n + 1));
    cap[0] = cap[1] = n;
    s = (wchar_t*)(cap + 2);
}

static void StrRealloc(wchar_t*& s, int n) {
    if (s != nullstr) {
        int* cap = (int*)s - 2;
        if (*cap < n) {
            *cap = *cap << 1;
            if (*cap < n) *cap = n;
            cap = (int*)realloc(cap, metaSize + sizeof(wchar_t) * (*cap + 1));
            s = (wchar_t*)(cap + 2);
        }
        cap[1] = n;
    } else {
        StrAlloc(s, n);
    }
}

inline void StrFree(wchar_t* s) {
    if (s != nullstr) {
        free((int*)s - 2);
    }
}

inline void StrSet(wchar_t* s, wchar_t c, int n) {
    for (int i = 0; i < n; ++i) s[i] = c;
}

inline void StrCopy(wchar_t* dst, const wchar_t* src, int n) {
    memcpy(dst, src, sizeof(wchar_t) * n);
}

};  // anonymous namespace

// ================================================================================================
// Wide String

WideString::~WideString() { StrFree(widestring_); }

WideString::WideString() : widestring_(nullstr) {}

WideString::WideString(WideString&& s) : widestring_(s.widestring_) {
    s.widestring_ = nullstr;
}

WideString::WideString(const WideString& s) {
    int n = s.size();
    StrAlloc(widestring_, n);
    StrCopy(widestring_, s.widestring_, n + 1);
}

WideString::WideString(int n, wchar_t c) {
    StrAlloc(widestring_, n);
    StrSet(widestring_, c, n);
    widestring_[n] = 0;
}

WideString::WideString(const wchar_t* s) {
    int n = wcslen(s);
    StrAlloc(widestring_, n);
    StrCopy(widestring_, s, n + 1);
}

WideString::WideString(const wchar_t* s, int n) {
    StrAlloc(widestring_, n);
    StrCopy(widestring_, s, n);
    widestring_[n] = 0;
}

WideString& WideString::operator=(WideString&& s) {
    swap(s);
    return *this;
}

WideString& WideString::operator=(const WideString& s) {
    assign(s.begin(), s.end());
    return *this;
}

void WideString::clear() {
    StrFree(widestring_);
    widestring_ = nullstr;
}

void WideString::assign(WideString&& s) { swap(s); }

void WideString::assign(const WideString& str) {
    assign(str.begin(), str.end());
}

void WideString::assign(const wchar_t* begin, const wchar_t* end) {
    int n = end - begin;
    StrRealloc(widestring_, n);
    StrCopy(widestring_, begin, n);
    widestring_[n] = 0;
}

void WideString::swap(WideString& s) {
    wchar_t* tmp = widestring_;
    widestring_ = s.widestring_;
    s.widestring_ = tmp;
}

void WideString::push_back(wchar_t c) {
    int len = size(), n = len + 1;
    StrRealloc(widestring_, n);
    widestring_[len] = c;
    widestring_[n] = 0;
}

void WideString::append(const WideString& str) {
    append(str.begin(), str.end());
}

void WideString::append(const wchar_t* str) { append(str, str + wcslen(str)); }

void WideString::append(const wchar_t* begin, const wchar_t* end) {
    int n = end - begin;
    if (n > 0) {
        int len = size(), newlen = len + n;
        StrRealloc(widestring_, newlen);
        StrCopy(widestring_ + len, begin, n);
        widestring_[newlen] = 0;
    }
}

bool operator<(const WideString& a, const WideString& b) {
    return wcscmp(a.str(), b.str()) < 0;
}

bool operator==(const WideString& a, const WideString& b) {
    return (a.size() == b.size() &&
            memcmp(a.str(), b.str(), a.size() * sizeof(wchar_t)) == 0);
}

// ================================================================================================
// Unicode conversion functions.

namespace {

static const uint32_t utf8Offsets[6] = {0x00000000, 0x00003080, 0x000E2080,
                                        0x03C82080, 0xFA082080, 0x82082080};

static const uint8_t utf8TrailingBytes[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

static const uint8_t utf8LeadingByte[7] = {0x00, 0x00, 0xC0, 0xE0,
                                           0xF0, 0xF8, 0xFC};

static uint32_t ReadU8(const uint8_t*& p, const uint8_t* end) {
    uint32_t c = *p;
    ++p;
    if (c >= 0x80) {
        int numBytes = utf8TrailingBytes[c];
        if (p + numBytes <= end) {
            switch (numBytes) {
                case 5:
                    c <<= 6;
                    c += *p;
                    ++p;
                case 4:
                    c <<= 6;
                    c += *p;
                    ++p;
                case 3:
                    c <<= 6;
                    c += *p;
                    ++p;
                case 2:
                    c <<= 6;
                    c += *p;
                    ++p;
                case 1:
                    c <<= 6;
                    c += *p;
                    ++p;
            };
            c -= utf8Offsets[numBytes];
        } else
            c = '~', p = end;
    }
    return c;
}

static uint32_t ReadU16(const uint16_t*& p, const uint16_t* end) {
    uint32_t c = *p;
    ++p;
    if (c >= 0xD800 && c <= 0xDBFF && p < end) {
        c = ((c - 0xD800) << 10) + (*p - 0xDC00) + 0x0010000;
        ++p;
    }
    return c;
}

static void WriteU8(std::string& str, uint32_t c) {
    if (c < 0x80) {
        Str::append(str, (char)c);
    } else if (c > 0x10FFFF) {
        Str::append(str, (char)c);
    } else {
        int numBytes = (c < 0x800) ? 2 : ((c < 0x10000) ? 3 : 4);
        uint8_t buffer[4];
        switch (numBytes) {
            case 4:
                buffer[3] = (c | 0x80) & 0xBF;
                c >>= 6;
            case 3:
                buffer[2] = (c | 0x80) & 0xBF;
                c >>= 6;
            case 2:
                buffer[1] = (c | 0x80) & 0xBF;
                c >>= 6;
        };
        buffer[0] = c | utf8LeadingByte[numBytes];
        Str::append(str, (const char*)buffer, numBytes);
    }
}

static void WriteU16(WideString& str, uint32_t c) {
    if (c < 0xFFFF)
        str.push_back((uint16_t)c);
    else if (c > 0x10FFFF)
        str.push_back(L'~');
    else {
        c -= 0x10000;
        str.push_back((uint16_t)(c >> 10) + 0xD800);
        str.push_back((uint16_t)(c & 0x3FFUL) + 0xDC00);
    }
}

};  // anonymous namespace.

std::string Narrow(const wchar_t* s, int len) {
    std::string out;
    if (sizeof(wchar_t) == sizeof(uint16_t)) {
        for (auto p = (const uint16_t*)s, end = p + len; p != end;)
            WriteU8(out, ReadU16(p, end));
    } else if (sizeof(wchar_t) == sizeof(uint32_t)) {
        for (auto p = (const uint32_t*)s, end = p + len; p != end;)
            WriteU8(out, *p);
    }
    return out;
}

WideString Widen(const char* s, int len) {
    WideString out;
    if (sizeof(wchar_t) == sizeof(uint16_t)) {
        for (auto p = (const uint8_t*)s, end = p + len; p != end;)
            WriteU16(out, ReadU8(p, end));
    } else if (sizeof(wchar_t) == sizeof(uint32_t)) {
        for (auto p = (const uint8_t*)s, end = p + len; p != end;)
            out.push_back(ReadU8(p, end));
    }
    return out;
}

std::string Narrow(const wchar_t* s) { return Narrow(s, wcslen(s)); }

std::string Narrow(const WideString& s) { return Narrow(s.str(), s.size()); }

WideString Widen(const char* s) { return Widen(s, strlen(s)); }

WideString Widen(const std::string& s) { return Widen(s.data(), s.length()); }

};  // namespace Vortex
