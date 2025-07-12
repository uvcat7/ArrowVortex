#include <Core/StringUtils.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

# pragma warning(disable : 4996) // stricmp.

namespace Vortex {

inline int min(int a, int b) { return (a > b) ? b : a; }
inline int max(int a, int b) { return (a < b) ? b : a; }

inline char ToUpper(char c) { return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c; }
inline char ToLower(char c) { return (c >= 'A' && c <= 'Z') ? (c | 0x20) : c; }

// ================================================================================================
// Helper functions.

extern void StrAlloc(char*& s, int newLen);
extern void StrRealloc(char*& s, int newLen);
extern void StrFree(char*& s);
extern void StrSetLen(char* s, int n);

struct Str2 {

static inline int cap(const std::string& s)
{
	return *((int*)s.string_ - 2);
}

static std::string cat(const char* a, int n, const char* b, int m)
{
	char* mem;
	int len = n + m;
	StrAlloc(mem, len);
	memcpy(mem, a, n);
	memcpy(mem + n, b, m);
	mem[len] = 0;

	std::string out;
	out.string_ = mem;
	return out;
}

static void reallocate(std::string& s, int n)
{
	if(n > cap(s))
	{
		StrRealloc(s.string_, n);
	}
	else if(n <= 0)
	{
		s.clear();
	}
}

static void createGap(std::string& s, int pos, int size)
{
	int len = s.len();
	StrRealloc(s.string_, len + size);
	memmove(s.string_ + pos + size, s.string_ + pos, len + 1 - pos);
}

static void closeGap(std::string & s, int pos, int size)
{
	int len = s.len();
	memmove(s.string_ + pos, s.string_ + pos + size, len + 1 - pos - size);
	StrSetLen(s.string_, len - size);
}

}; // Str2

// ================================================================================================
// Str :: assign functions.

std::string Str::create(const char* begin, const char* end)
{
	return std::string(begin, end - begin);
}

void Str::assign(std::string& s, int n, char c)
{
	if(n > 0)
	{
		StrRealloc(s.string_, n);
		memset(s.string_, c, n);
		s.string_[n] = 0;
	}
	else
	{
		s.clear();
	}
}

void Str::assign(std::string& s, std::string&& str)
{
	s.swap(str);
}

void Str::assign(std::string& s, const std::string& str)
{
	assign(s, str.string_, str.len());
}

void Str::assign(std::string& s, const char* str)
{
	assign(s, str, strlen(str));
}

void Str::assign(String& s, const char* str, int n)
{
	StrRealloc(s.string_, n);
	memcpy(s.string_, str, n);
	s.string_[n] = 0;
}

// ================================================================================================
// Str :: append functions.

void Str::append(std::string& s, char c)
{
	int len = s.len();
	int n = len + 1;
	StrRealloc(s.string_, n);
	s.string_[len] = c;
	s.string_[n] = 0;
}

void Str::append(std::string& s, const std::string& str)
{
	append(s, str.string_, str.len());
}

void Str::append(std::string& s, const char* str)
{
	append(s, str, strlen(str));
}

void Str::append(std::string& s, const char* str, int n)
{
	if(n > 0)
	{
		int len = s.len();
		int newlen = len + n;
		StrRealloc(s.string_, newlen);
		memcpy(s.string_ + len, str, n);
		s.string_[newlen] = 0;
	}
}

// ================================================================================================
// Str :: insert functions.

void Str::insert(std::string& s, int pos, char c)
{
	int len = s.len();
	if(pos >= len)
	{
		append(s, c);
	}
	else if(pos >= 0)
	{
		Str2::createGap(s, pos, 1);
		s.string_[pos] = c;
	}
}

void Str::insert(std::string& s, int pos, const std::string& str)
{
	insert(s, pos, str.string_, str.len());
}

void Str::insert(std::string& s, int pos, const char* str)
{
	insert(s, pos, str, strlen(str));
}

void Str::insert(std::string& s, int pos, const char* str, int n)
{
	int len = s.len();
	if(pos >= len)
	{
		append(s, str, n);
	}
	else if(n > 0)
	{
		Str2::createGap(s, pos, n);
		memcpy(s.string_ + pos, str, n);
	}
}

// ================================================================================================
// Str :: resize functions.

void Str::truncate(std::string& s, int n)
{
	if(n < s.len())
	{
		if(n <= 0)
		{
			s.clear();
		}
		else
		{
			s.string_[n] = 0;
			StrSetLen(s.string_, n);
		}
	}
}

void Str::extend(std::string& s, int n, char c)
{
	int len = s.len();
	if(n > len)
	{
		StrRealloc(s.string_, n);
		memset(s.string_ + len, c, n - len);
		s.string_[n] = 0;
	}
}

void Str::resize(std::string& s, int n, char c)
{
	if(n > s.len())
	{
		extend(s, n, c);
	}
	else
	{
		truncate(s, n);
	}
}

// ================================================================================================
// Str :: value conversion.

int Str::readInt(const std::string& s, int alt)
{
	read(s, &alt);
	return alt;
}

uint32_t Str::readUint(const std::string& s, uint32_t alt)
{
	read(s, &alt);
	return alt;
}

float Str::readFloat(const std::string& s, float alt)
{
	read(s, &alt);
	return alt;
}

double Str::readDouble(const std::string& s, double alt)
{
	read(s, &alt);
	return alt;
}

bool Str::readBool(const std::string& s, bool alt)
{
	read(s, &alt);
	return alt;
}

double Str::readTime(const std::string& s, double alt)
{
	auto time = Str::split(s, ":", false, false);
	double v = 0.0;

	switch(time.size())
	{
	case 3:
		v = (readInt(time[0]) * 3600) + (readInt(time[1]) * 60) + readDouble(time[2]);
		break;
	case 2:
		v = (readInt(time[0]) * 60) + readDouble(time[1]);
		break;
	default:
		v = readDouble(time[0]);
		break;
	}

	if (v == 0 && *s.string_ == 0) return alt;
	alt = v;
	return alt;
}

bool Str::read(const std::string& s, int* out)
{
	char* end;
	int v = strtol(s.string_, &end, 10);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(const std::string& s, uint32_t* out)
{
	char* end;
	uint32_t v = strtoul(s.string_, &end, 10);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(const std::string& s, float* out)
{
	char* end;
	float v = strtof(s.string_, &end);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(const std::string& s, double* out)
{
	char* end;
	double v = strtod(s.string_, &end);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(const std::string& s, bool* out)
{
	if(stricmp(s.string_, "true") == 0 || stricmp(s.string_, "yes") == 0)
	{
		*out = true;
		return true;
	}
	else if(stricmp(s.string_, "false") == 0 || stricmp(s.string_, "no") == 0)
	{
		*out = false;
		return true;
	}
	else
	{
		int v;
		if(read(s, &v))
		{
			*out = (v > 0);
			return true;
		}
	}
	return false;
}

// ================================================================================================
// Str :: modifying functions.

static bool IsWhiteSpace(char c)
{
	return c == 0x20 || c == 0x9 || c == 0xA || c == 0xD;
}

void Str::trim(std::string& s)
{
	int n = 0, m = 0, len = s.len();
	while(IsWhiteSpace(s.string_[m])) ++m;
	if(s.string_[m] == 0) { s.clear(); return; }

	while(m < len) s.string_[n++] = s.string_[m++];
	while(IsWhiteSpace(s.string_[n - 1])) --n;
	s.string_[n] = 0;
	StrSetLen(s.string_, n);
}

void Str::simplify(std::string& s)
{
	trim(s);
	int n = 0, m = 0, w = 0, len = s.length();
	while(m < len)
	{
		w = m;
		while(IsWhiteSpace(s.string_[m])) ++m;
		if(m > w) s.string_[n++] = ' ';
		s.string_[n++] = s.string_[m++];
	}
	s.string_[n] = 0;
	StrSetLen(s.string_, n);
}

void Str::erase(std::string& s, int pos, int n)
{
	int len = s.length();
	if(pos < 0)	{ n += pos, pos = 0; }
	if(pos == 0 && n >= len)
	{
		s.clear();
	}
	else if(pos < len && n > 0)
	{
		n = min(n, len - pos);
		Str2::closeGap(s, pos, n);
	}
}

void Str::pop_back(std::string& s)
{
	int len = s.length();
	if(len)
	{
		s.string_[--len] = 0;
		StrSetLen(s.string_, len);
	}
}

void Str::replace(std::string& s, char find, char replace)
{
	char* c = s.string_;
	for(int i = 0, len = s.length(); i < len; ++i, ++c)
	{
		if(*c == find) *c = replace;
	}
}

void Str::replace(std::string& s, const char* fnd, const char* rep)
{
	if(*fnd == 0) return;

	int pos = find(s, fnd);
	if(pos == std::string::npos) return;

	int fndlen = strlen(fnd);
	int replen = strlen(rep);
	std::string out(s.string_, pos);
	
	while(pos != std::string::npos)
	{
		pos += fndlen;
		append(out, rep, replen);
		int next = find(s, fnd, pos);
		int end = (next != std::string::npos) ? next : s.length();
		append(out, s.string_ + pos, end - pos);
		pos = next;
	}

	s.swap(out);
}

void Str::toUpper(std::string& s)
{
	for(int i = 0, len = s.length(); i < len; ++i)
	{
		s.string_[i] = ToUpper(s.string_[i]);
	}
}

void Str::toLower(std::string& s)
{
	for(int i = 0, len = s.length(); i < len; ++i)
	{
		s.string_[i] = ToLower(s.string_[i]);
	}
}

// ================================================================================================
// Information functions

std::string Str::substr(const std::string& s, int pos, int n)
{
	int len = s.length();
	if(pos < 0) { n += pos, pos = 0; }
	if(pos == 0 && n >= len)
	{
		return s;
	}
	else if(pos < len && n > 0)
	{
		n = min(n, len - pos);
		return std::string(s.string_ + pos, n);
	}
	return {};
}

int Str::nextChar(const std::string& s, int pos)
{
	int len = s.length();
	if(pos >= len) return std::string::npos;
	do { ++pos; } while(pos < len && (s.string_[pos] & 0xC0) == 0x80);
	return pos;
}

int Str::prevChar(const std::string& s, int pos)
{
	if(pos <= 0) return -1;
	do { --pos; } while(pos >= 0 && (s.string_[pos] & 0xC0) == 0x80);
	return pos;
}

bool Str::isUnicode(const std::string& s)
{
	for(char c : s) { if(c & 0x80) return true; }
	return false;
}

int Str::find(const std::string& s, char c, int pos)
{
	int len = s.length();
	pos = max(pos, 0);
	while(pos < len && s.string_[pos] != c) ++pos;
	return (pos < len) ? pos : std::string::npos;
}

int Str::find(const std::string& s, const char* str, int pos)
{
	pos = max(pos, 0);
	int len = s.len();

	if(*str == 0 && pos <= len)
		return pos;

	for(int i = pos; i < len; ++i)
	{
		if(s.string_[i] == *str)
		{
			const char* a = str, *b = s.string_ + i;
			while(*a && *a == *b) ++a, ++b;
			if(*a == 0) return i;
		}
	}

	return std::string::npos;
}

int Str::findLast(const std::string& s, char c, int pos)
{
	int len = s.length();
	pos = min(pos, len - 1);
	while(pos >= 0 && s.string_[pos] != c) --pos;
	return (pos >= 0) ? pos : -1;
}

int Str::findAnyOf(const std::string& s, const char* c, int pos)
{
	int len = s.length();
	pos = max(pos, 0);
	while(pos < len)
	{
		const char a = s.string_[pos], *b = c;
		while(*b && *b != a) ++b;
		if(*b) break;
		++pos;
	}
	return (pos < len) ? pos : std::string::npos;
}

int Str::findLastOf(const std::string& s, const char* c, int pos)
{
	int len = s.length();
	pos = min(pos, len - 1);
	while(pos >= 0)
	{
		const char a = s.string_[pos], *b = c;
		while(*b && *b != a) ++b;
		if(*b) break;
		--pos;
	}
	return (pos >= 0) ? pos : -1;
}

// ================================================================================================
// Str :: compare functions.

static bool Equals(const char* a, const char* b, int len, bool caseSensitive)
{
	if(caseSensitive)
	{
		return strncmp(a, b, len) == 0;
	}
	else
	{
		return strnicmp(a, b, len) == 0;
	}
}

bool Str::endsWith(const std::string& s, const char* suffix, bool useCase)
{
	int len = s.length(), n = strlen(suffix), res = 1;
	if(len >= n) return Equals(s.string_ + len - n, suffix, n, useCase);
	return false;
}

bool Str::startsWith(const std::string& s, const char* prefix, bool useCase)
{
	int len = s.len(), n = strlen(prefix), res = 1;
	if(len >= n) return Equals(s.string_, prefix, n, useCase);
	return false;
}

int Str::compare(const std::string& a, const std::string& b)
{
	return strcmp(a.c_str(), b.c_str());
}

int Str::compare(const std::string& a, const char* b)
{
	return strcmp(a.c_str(), b);
}

int Str::compare(const char* a, const char* b)
{
	return strcmp(a, b);
}

int Str::icompare(const std::string& a, const std::string& b)
{
	return stricmp(a.c_str(), b.c_str());
}

int Str::icompare(const std::string& a, const char* b)
{
	return stricmp(a.c_str(), b);
}

int Str::icompare(const char* a, const char* b)
{
	return stricmp(a, b);
}

bool Str::equal(const std::string& a, const std::string& b)
{
	return strcmp(a.c_str(), b.c_str()) == 0;
}

bool Str::equal(const std::string& a, const char* b)
{
	return strcmp(a.c_str(), b) == 0;
}

bool Str::equal(const char* a, const char* b)
{
	return strcmp(a, b) == 0;
}

bool Str::iequal(const std::string& a, const std::string& b)
{
	return stricmp(a.c_str(), b.c_str()) == 0;
}

bool Str::iequal(const std::string& a, const char* b)
{
	return stricmp(a.c_str(), b) == 0;
}

bool Str::iequal(const char* a, const char* b)
{
	return stricmp(a, b) == 0;
}

// ================================================================================================
// Str :: formatting functions.

typedef Str::fmt Fmt;

static const int INT_BUFLEN = 32;
static const int DBL_BUFLEN = 64;

static int PrintInt(char* buf, int v, int minDig, bool hex)
{
	int len;
	if(minDig <= 0)
	{
		len = _snprintf(buf, INT_BUFLEN, hex ? "%X" : "%i", v);
	}
	else
	{
		len = _snprintf(buf, INT_BUFLEN, hex ? "%0*X" : "%0*i", minDig, v);
	}
	return (len < 0) ? DBL_BUFLEN : len;
}

static int PrintUint(char* buf, uint32_t v, int minDig, bool hex)
{
	int len;
	if(minDig <= 0)
	{
		len = _snprintf(buf, INT_BUFLEN, hex ? "%X" : "%u", v);
	}
	else
	{
		len = _snprintf(buf, INT_BUFLEN, hex ? "%0*X" : "%0*u", minDig, v);
	}
	return (len < 0) ? INT_BUFLEN : len;
}

static int PrintDouble(char* buf, double v, int minDec, int maxDec)
{
	minDec = min(max(minDec, 0), 16);
	maxDec = min(max(minDec, maxDec), 16);

	// Print the value.
	int len = _snprintf(buf, DBL_BUFLEN, "%.*f", maxDec, v);
	if(len < 0) len = DBL_BUFLEN;

	// Cap the number of decimal digits.
	int decimalPoint = len - 1;
	while(decimalPoint >= 0 && buf[decimalPoint] != '.') --decimalPoint;
	if(decimalPoint >= 0)
	{
		int decimalDigits = len - decimalPoint - 1;

		// Remove trailing zeroes.
		while(decimalDigits > minDec && buf[len - 1] == '0')
		{
			--len, --decimalDigits;
		}

		// Remove the decimal point if there are no decimal digits left.
		if(decimalDigits == 0 && decimalPoint >= 0)
		{
			--len;
		}
	}

	return len;
}

std::string Str::val(int v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return std::string(buf, PrintInt(buf, v, minDig, hex));
}

std::string Str::val(uint32_t v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return std::string(buf, PrintUint(buf, v, minDig, hex));
}

std::string Str::val(float v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return std::string(buf, PrintDouble(buf, v, minDec, maxDec));
}

std::string Str::val(double v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return std::string(buf, PrintDouble(buf, v, minDec, maxDec));
}

void Str::appendVal(std::string& s, int v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	append(s, buf, PrintInt(buf, v, minDig, hex));
}

void Str::appendVal(std::string& s, uint32_t v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	append(s, buf, PrintUint(buf, v, minDig, hex));
}

void Str::appendVal(std::string& s, float v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	append(s, buf, PrintDouble(buf, v, minDec, maxDec));
}

void Str::appendVal(std::string& s, double v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	append(s, buf, PrintDouble(buf, v, minDec, maxDec));
}


Fmt::fmt(const std::string& format) : str(format)
{
}

Fmt::fmt(const char* format) : str(format)
{
}

Fmt& Fmt::arg(int v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return arg(buf, PrintInt(buf, v, minDig, hex));
}

Fmt& Fmt::arg(uint32_t v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return arg(buf, PrintUint(buf, v, minDig, hex));
}

Fmt& Fmt::arg(float v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return arg(buf, PrintDouble(buf, v, minDec, maxDec));
}

Fmt& Fmt::arg(double v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return arg(buf, PrintDouble(buf, v, minDec, maxDec));
}

Fmt& Fmt::arg(char c)
{
	return arg(&c, 1);
}

Fmt& Fmt::arg(const std::string& s)
{
	return arg(s.data(), s.length());
}

Fmt& Fmt::arg(const char* s)
{
	return arg(s, strlen(s));
}

Fmt& Fmt::arg(const char* s, int n)
{
	int fmtLen = str.length();

	// find the lowest marker position.
	int markerPos = fmtLen;
	int markerLen = 0;
	if(fmtLen > 0)
	{
		int lowestMarker = 100;
		const char* p = str.c_str();
		for(int i = 0; i < fmtLen; ++i)
		{
			if(p[i] == '%')
			{
				char c = p[i + 1];
				if(c >= '0' && c <= '9')
				{
					int len = 2;
					int marker = c - '0';
					c = p[i + 2];
					if(c >= '0' && c <= '9')
					{
						marker = marker * 10 + c - '0';
						++len;
					}
					if(marker > 0 && marker < lowestMarker)
					{
						lowestMarker = marker;
						markerPos = i;
						markerLen = len;
					}
				}
			}
		}
	}

	// Insert the string at the marker position.
	if(n > markerLen)
	{
		Str2::createGap(str, markerPos + markerLen, n - markerLen);
		memcpy(str.string_ + markerPos, s, n);
	}
	else
	{
		Str2::closeGap(str, markerPos, markerLen - n);
		memcpy(str.string_ + markerPos, s, n);
	}

	return *this;
}

// ================================================================================================
// Str:: expression parsing.

static const uint8_t* ParseNestedExpression(const uint8_t* p, double& out);

inline const uint8_t* SkipWs(const uint8_t* p)
{
	while(*p == ' ' || *p == '\t') ++p;
	return p;
}

static const uint8_t* ParseNumber(const uint8_t* p, double& out)
{
	// Digits leading up to the decimal-point.
	uint64_t sum = 0;
	for(; *p >= '0' && *p <= '9'; ++p)
	{
		sum = sum * 10 + (*p - '0');
	}
	out = (double)sum;

	// Digits after the decimal-point.
	if(*p == '.' || *p == ',')
	{
		uint64_t dec = 0;
		int digits = 0;
		for(++p; *p >= '0' && *p <= '9'; ++p, ++digits)
		{
			dec = dec * 10 + (*p - '0');
		}
		out += (double)dec / pow(10.0, digits);
	}

	// Optional exponent.
	if(*p == 'e' || *p == 'E')
	{
		++p;

		// Plus/minus sign.
		bool neg = false;
		if(*p == '-' || *p == '+')
		{
			neg = (*p++ == '-');
		}

		// Exponent digits.
		int exp = 0;
		for(; *p >= '0' && *p <= '9'; ++p)
		{
			exp = exp * 10 + (*p - '0');
		}
		if(neg) exp = -exp;

		out *= pow(10.0, exp);
	}

	return SkipWs(p);
}

static const uint8_t* ParseOperandWithSign(const uint8_t* p, double& out)
{
	char sign = *p;
	p = SkipWs(++p);
	if(*p == '(')
	{
		p = ParseNestedExpression(SkipWs(++p), out);
		if(*p == ')') p = SkipWs(++p);
	}
	else if(*p >= '0' && *p <= '9')
	{
		p = ParseNumber(p, out);
	}
	if(sign == '-') out = -out;
	return p;
}

static const uint8_t* ParseMultiplicationOperand(const uint8_t* p, double& out)
{
	if(*p == '(')
	{
		p = ParseNestedExpression(SkipWs(++p), out);
		if(*p == ')') p = SkipWs(++p);
	}
	else if(*p >= '0' && *p <= '9')
	{
		p = ParseNumber(p, out);
	}
	else if(*p == '+' || *p == '-')
	{
		p = ParseOperandWithSign(p, out);
	}
	return p;
}

static const uint8_t* ParseAdditionOperand(const uint8_t* p, double& out)
{
	p = ParseMultiplicationOperand(p, out);
	while(*p == '*' || *p == '/')
	{
		char op = *p;
		double rhs = 0.0;
		p = ParseMultiplicationOperand(SkipWs(++p), rhs);
		if(op == '*') out *= rhs; else out /= rhs;
	}
	return p;
}

static const uint8_t* ParseNestedExpression(const uint8_t* p, double& out)
{
	p = ParseAdditionOperand(p, out);
	while(*p == '+' || *p == '-')
	{
		char op = *p;
		double rhs = 0.0;
		p = ParseAdditionOperand(SkipWs(++p), rhs);
		if(op == '+') out += rhs; else out -= rhs;
	}
	return p;
}

bool Str::parse(const char* expr, double& out)
{
	double tmp = 0.0;
	const uint8_t* begin = SkipWs((const uint8_t*)expr);
	const uint8_t* p = ParseNestedExpression(begin, tmp);
	if(p > begin) out = tmp;
	return (p > begin);
}

// ================================================================================================
// Str :: string splitting and joining.

Vector<std::string> Str::split(const std::string& s)
{
	Vector<std::string> out;
	const char* p = s.begin();
	while(IsWhiteSpace(*p)) ++p;
	while(*p)
	{
		const char* start = p;
		while(*p && !IsWhiteSpace(*p)) ++p;
		out.push_back(std::string(start, p - start));
		while(IsWhiteSpace(*p)) ++p;
	}
	return out;
}

Vector<std::string> Str::split(const std::string& s, const char* lim, bool trim, bool skip)
{
	Vector<std::string> out;
	int limlen = strlen(lim);
	int slen = s.length() - limlen, start = 0;
	for(int i = 0; i <= slen;)
	{
		if(memcmp(s.begin() + i, lim, limlen) == 0)
		{
			std::string sub = Str::substr(s, start, i - start);
			if(trim) Str::trim(sub);
			if(sub.len() || !skip) out.push_back(sub);
			i += limlen, start = i;
		}
		else ++i;
	}
	std::string sub = Str::substr(s, start, std::string::npos);
	if(trim) Str::trim(sub);
	if(sub.len() || !skip) out.push_back(sub);
	return out;
}

std::string Str::join(const Vector<std::string>& list, const char* lim)
{
	if(list.empty()) return {};

	// Determine the total String length and allocate the output string.
	int limlen = strlen(lim);
	int len = (list.size() - 1) * limlen;
	for(auto& s : list) len += s.length();
	std::string out(len, 0);

	// Copy the first item without the delimiter.
	char* p = out.begin();
	auto it = list.begin(), end = list.end();
	memcpy(p, it->begin(), it->length());
	p += it->length();

	// Copy each additional item with a delimiter.
	for(++it; it != end; ++it)
	{
		memcpy(p, lim, limlen);
		p += limlen;
		memcpy(p, it->begin(), it->length());
		p += it->length();
	}

	return out;
}

// ================================================================================================
// Str :: time formatting.

std::string Str::formatTime(double seconds, bool precise)
{
	bool negative = false;
	if(seconds < 0.0)
	{
		negative = true;
		seconds = -seconds;
	}

	int64_t t = (int64_t)(seconds * 1000.0);
	int64_t min = t / (60 * 1000);
	t -= min * (60 * 1000);
	int64_t sec = t / 1000;

	auto fmt = Str::fmt("%1:%2.").arg((int)min, 2).arg((int)sec, 2);

	if(precise)
	{
		t -= sec * 1000;
		Str::appendVal(fmt, (int)t, 3);
	}
	else
	{
		t -= sec * 1000;
		t /= 100;
		Str::appendVal(fmt, (int)t);
	}

	if(negative)
	{
		Str::insert(fmt, 0, '-');
	}

	return fmt;
}

// ================================================================================================
// Global string operators.

std::string operator + (const std::string& a, char b)
{
	return Str2::cat(a.begin(), a.length(), &b, 1);
}

std::string operator + (char a, const std::string& b)
{
	return Str2::cat(&a, 1, b.begin(), b.length());
}

std::string operator + (const std::string& a, const char* b)
{
	return Str2::cat(a.begin(), a.length(), b, strlen(b));
}

std::string operator + (const char* a, const std::string& b)
{
	return Str2::cat(a, strlen(a), b.begin(), b.length());
}

std::string operator + (const std::string& a, const std::string& b)
{
	return Str2::cat(a.begin(), a.length(), b.begin(), b.length());
}

std::string& operator += (std::string& a, char b)
{
	Str::append(a, b);
	return a;
}

std::string& operator += (std::string& a, const char* b)
{
	Str::append(a, b, strlen(b));
	return a;
}

std::string& operator += (std::string& a, const std::string& b)
{
	Str::append(a, b.begin(), b.length());
	return a;
}

bool operator < (const std::string& a, const char* b)
{
	return strcmp(a.str(), b) < 0;
}

bool operator < (const char* a, const std::string& b)
{
	return strcmp(a, b.str()) < 0;
}

bool operator < (const std::string& a, const std::string& b)
{
	return strcmp(a.str(), b.str()) < 0;
}

bool operator > (const std::string& a, const char* b)
{
	return strcmp(a.str(), b) > 0;
}

bool operator > (const char* a, const std::string& b)
{
	return strcmp(a, b.str()) > 0;
}

bool operator > (const std::string& a, const std::string& b)
{
	return strcmp(a.str(), b.str()) > 0;
}

bool operator == (const std::string& a, const char* b)
{
	return strcmp(a.str(), b) == 0;
}

bool operator == (const char* a, const std::string& b)
{
	return strcmp(a, b.str()) == 0;
}

bool operator == (const std::string& a, const std::string& b)
{
	int len1 = a.len(), len2 = b.len();
	if(len1 != len2) return false;
	return memcmp(a.str(), b.str(), len1) == 0;
}

bool operator != (const std::string& a, const char* b)
{
	return strcmp(a.str(), b) != 0;
}

bool operator != (const char* a, const std::string& b)
{
	return strcmp(a, b.str()) != 0;
}

bool operator != (const std::string& a, const std::string& b)
{
	return !(a == b);
}

}; // namespace Vortex
