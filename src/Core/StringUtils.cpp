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

static inline int cap(StringRef s)
{
	return *((int*)s.string_ - 2);
}

static String cat(const char* a, int n, const char* b, int m)
{
	char* mem;
	int len = n + m;
	StrAlloc(mem, len);
	memcpy(mem, a, n);
	memcpy(mem + n, b, m);
	mem[len] = 0;

	String out;
	out.string_ = mem;
	return out;
}

static void reallocate(String& s, int n)
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

static void createGap(String& s, int pos, int size)
{
	int len = s.len();
	StrRealloc(s.string_, len + size);
	memmove(s.string_ + pos + size, s.string_ + pos, len + 1 - pos);
}

static void closeGap(String& s, int pos, int size)
{
	int len = s.len();
	memmove(s.string_ + pos, s.string_ + pos + size, len + 1 - pos - size);
	StrSetLen(s.string_, len - size);
}

}; // Str2

// ================================================================================================
// Str :: assign functions.

String Str::create(const char* begin, const char* end)
{
	return String(begin, end - begin);
}

void Str::assign(String& s, int n, char c)
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

void Str::assign(String& s, String&& str)
{
	s.swap(str);
}

void Str::assign(String& s, StringRef str)
{
	assign(s, str.string_, str.len());
}

void Str::assign(String& s, const char* str)
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

void Str::append(String& s, char c)
{
	int len = s.len();
	int n = len + 1;
	StrRealloc(s.string_, n);
	s.string_[len] = c;
	s.string_[n] = 0;
}

void Str::append(String& s, StringRef str)
{
	append(s, str.string_, str.len());
}

void Str::append(String& s, const char* str)
{
	append(s, str, strlen(str));
}

void Str::append(String& s, const char* str, int n)
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

void Str::insert(String& s, int pos, char c)
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

void Str::insert(String& s, int pos, StringRef str)
{
	insert(s, pos, str.string_, str.len());
}

void Str::insert(String& s, int pos, const char* str)
{
	insert(s, pos, str, strlen(str));
}

void Str::insert(String& s, int pos, const char* str, int n)
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

void Str::truncate(String& s, int n)
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

void Str::extend(String& s, int n, char c)
{
	int len = s.len();
	if(n > len)
	{
		StrRealloc(s.string_, n);
		memset(s.string_ + len, c, n - len);
		s.string_[n] = 0;
	}
}

void Str::resize(String& s, int n, char c)
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

int Str::readInt(StringRef s, int alt)
{
	read(s, &alt);
	return alt;
}

uint Str::readUint(StringRef s, uint alt)
{
	read(s, &alt);
	return alt;
}

float Str::readFloat(StringRef s, float alt)
{
	read(s, &alt);
	return alt;
}

double Str::readDouble(StringRef s, double alt)
{
	read(s, &alt);
	return alt;
}

bool Str::readBool(StringRef s, bool alt)
{
	read(s, &alt);
	return alt;
}

double Str::readTime(StringRef s, double alt)
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

bool Str::read(StringRef s, int* out)
{
	char* end;
	int v = strtol(s.string_, &end, 10);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(StringRef s, uint* out)
{
	char* end;
	uint v = strtoul(s.string_, &end, 10);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(StringRef s, float* out)
{
	char* end;
	float v = strtof(s.string_, &end);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(StringRef s, double* out)
{
	char* end;
	double v = strtod(s.string_, &end);
	if(v == 0 && (*s.string_ == 0 || *end != 0)) return false;
	*out = v;
	return true;
}

bool Str::read(StringRef s, bool* out)
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

void Str::trim(String& s)
{
	int n = 0, m = 0, len = s.len();
	while(IsWhiteSpace(s.string_[m])) ++m;
	if(s.string_[m] == 0) { s.clear(); return; }

	while(m < len) s.string_[n++] = s.string_[m++];
	while(IsWhiteSpace(s.string_[n - 1])) --n;
	s.string_[n] = 0;
	StrSetLen(s.string_, n);
}

void Str::simplify(String& s)
{
	trim(s);
	int n = 0, m = 0, w = 0, len = s.len();
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

void Str::erase(String& s, int pos, int n)
{
	int len = s.len();
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

void Str::pop_back(String& s)
{
	int len = s.len();
	if(len)
	{
		s.string_[--len] = 0;
		StrSetLen(s.string_, len);
	}
}

void Str::replace(String& s, char find, char replace)
{
	char* c = s.string_;
	for(int i = 0, len = s.len(); i < len; ++i, ++c)
	{
		if(*c == find) *c = replace;
	}
}

void Str::replace(String& s, const char* fnd, const char* rep)
{
	if(*fnd == 0) return;

	int pos = find(s, fnd);
	if(pos == String::npos) return;

	int fndlen = strlen(fnd);
	int replen = strlen(rep);
	String out(s.string_, pos);
	
	while(pos != String::npos)
	{
		pos += fndlen;
		append(out, rep, replen);
		int next = find(s, fnd, pos);
		int end = (next != String::npos) ? next : s.len();
		append(out, s.string_ + pos, end - pos);
		pos = next;
	}

	s.swap(out);
}

void Str::toUpper(String& s)
{
	for(int i = 0, len = s.len(); i < len; ++i)
	{
		s.string_[i] = ToUpper(s.string_[i]);
	}
}

void Str::toLower(String& s)
{
	for(int i = 0, len = s.len(); i < len; ++i)
	{
		s.string_[i] = ToLower(s.string_[i]);
	}
}

// ================================================================================================
// Information functions

String Str::substr(StringRef s, int pos, int n)
{
	int len = s.len();
	if(pos < 0) { n += pos, pos = 0; }
	if(pos == 0 && n >= len)
	{
		return s;
	}
	else if(pos < len && n > 0)
	{
		n = min(n, len - pos);
		return String(s.string_ + pos, n);
	}
	return {};
}

int Str::nextChar(StringRef s, int pos)
{
	int len = s.len();
	if(pos >= len) return String::npos;
	do { ++pos; } while(pos < len && (s.string_[pos] & 0xC0) == 0x80);
	return pos;
}

int Str::prevChar(StringRef s, int pos)
{
	if(pos <= 0) return -1;
	do { --pos; } while(pos >= 0 && (s.string_[pos] & 0xC0) == 0x80);
	return pos;
}

bool Str::isUnicode(StringRef s)
{
	for(char c : s) { if(c & 0x80) return true; }
	return false;
}

int Str::find(StringRef s, char c, int pos)
{
	int len = s.len();
	pos = max(pos, 0);
	while(pos < len && s.string_[pos] != c) ++pos;
	return (pos < len) ? pos : String::npos;
}

int Str::find(StringRef s, const char* str, int pos)
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

	return String::npos;
}

int Str::findLast(StringRef s, char c, int pos)
{
	int len = s.len();
	pos = min(pos, len - 1);
	while(pos >= 0 && s.string_[pos] != c) --pos;
	return (pos >= 0) ? pos : -1;
}

int Str::findAnyOf(StringRef s, const char* c, int pos)
{
	int len = s.len();
	pos = max(pos, 0);
	while(pos < len)
	{
		const char a = s.string_[pos], *b = c;
		while(*b && *b != a) ++b;
		if(*b) break;
		++pos;
	}
	return (pos < len) ? pos : String::npos;
}

int Str::findLastOf(StringRef s, const char* c, int pos)
{
	int len = s.len();
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

bool Str::endsWith(StringRef s, const char* suffix, bool useCase)
{
	int len = s.len(), n = strlen(suffix), res = 1;
	if(len >= n) return Equals(s.string_ + len - n, suffix, n, useCase);
	return false;
}

bool Str::startsWith(StringRef s, const char* prefix, bool useCase)
{
	int len = s.len(), n = strlen(prefix), res = 1;
	if(len >= n) return Equals(s.string_, prefix, n, useCase);
	return false;
}

int Str::compare(StringRef a, StringRef b)
{
	return strcmp(a.str(), b.str());
}

int Str::compare(StringRef a, const char* b)
{
	return strcmp(a.str(), b);
}

int Str::compare(const char* a, const char* b)
{
	return strcmp(a, b);
}

int Str::icompare(StringRef a, StringRef b)
{
	return stricmp(a.str(), b.str());
}

int Str::icompare(StringRef a, const char* b)
{
	return stricmp(a.str(), b);
}

int Str::icompare(const char* a, const char* b)
{
	return stricmp(a, b);
}

bool Str::equal(StringRef a, StringRef b)
{
	return strcmp(a.str(), b.str()) == 0;
}

bool Str::equal(StringRef a, const char* b)
{
	return strcmp(a.str(), b) == 0;
}

bool Str::equal(const char* a, const char* b)
{
	return strcmp(a, b) == 0;
}

bool Str::iequal(StringRef a, StringRef b)
{
	return stricmp(a.str(), b.str()) == 0;
}

bool Str::iequal(StringRef a, const char* b)
{
	return stricmp(a.str(), b) == 0;
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

static int PrintUint(char* buf, uint v, int minDig, bool hex)
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

String Str::val(int v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return String(buf, PrintInt(buf, v, minDig, hex));
}

String Str::val(uint v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	return String(buf, PrintUint(buf, v, minDig, hex));
}

String Str::val(float v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return String(buf, PrintDouble(buf, v, minDec, maxDec));
}

String Str::val(double v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	return String(buf, PrintDouble(buf, v, minDec, maxDec));
}

void Str::appendVal(String& s, int v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	append(s, buf, PrintInt(buf, v, minDig, hex));
}

void Str::appendVal(String& s, uint v, int minDig, bool hex)
{
	char buf[INT_BUFLEN];
	append(s, buf, PrintUint(buf, v, minDig, hex));
}

void Str::appendVal(String& s, float v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	append(s, buf, PrintDouble(buf, v, minDec, maxDec));
}

void Str::appendVal(String& s, double v, int minDec, int maxDec)
{
	char buf[DBL_BUFLEN];
	append(s, buf, PrintDouble(buf, v, minDec, maxDec));
}


Fmt::fmt(StringRef format) : str(format)
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

Fmt& Fmt::arg(uint v, int minDig, bool hex)
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

Fmt& Fmt::arg(StringRef s)
{
	return arg(s.string_, s.len());
}

Fmt& Fmt::arg(const char* s)
{
	return arg(s, strlen(s));
}

Fmt& Fmt::arg(const char* s, int n)
{
	int fmtLen = str.len();

	// find the lowest marker position.
	int markerPos = fmtLen;
	int markerLen = 0;
	if(fmtLen > 0)
	{
		int lowestMarker = 100;
		const char* p = str.str();
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

static const uchar* ParseNestedExpression(const uchar* p, double& out);

inline const uchar* SkipWs(const uchar* p)
{
	while(*p == ' ' || *p == '\t') ++p;
	return p;
}

static const uchar* ParseNumber(const uchar* p, double& out)
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

static const uchar* ParseOperandWithSign(const uchar* p, double& out)
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

static const uchar* ParseMultiplicationOperand(const uchar* p, double& out)
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

static const uchar* ParseAdditionOperand(const uchar* p, double& out)
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

static const uchar* ParseNestedExpression(const uchar* p, double& out)
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
	const uchar* begin = SkipWs((const uchar*)expr);
	const uchar* p = ParseNestedExpression(begin, tmp);
	if(p > begin) out = tmp;
	return (p > begin);
}

// ================================================================================================
// Str :: string splitting and joining.

Vector<String> Str::split(StringRef s)
{
	Vector<String> out;
	const char* p = s.begin();
	while(IsWhiteSpace(*p)) ++p;
	while(*p)
	{
		const char* start = p;
		while(*p && !IsWhiteSpace(*p)) ++p;
		out.push_back(String(start, p - start));
		while(IsWhiteSpace(*p)) ++p;
	}
	return out;
}

Vector<String> Str::split(StringRef s, const char* lim, bool trim, bool skip)
{
	Vector<String> out;
	int limlen = strlen(lim);
	int slen = s.len() - limlen, start = 0;
	for(int i = 0; i <= slen;)
	{
		if(memcmp(s.begin() + i, lim, limlen) == 0)
		{
			String sub = Str::substr(s, start, i - start);
			if(trim) Str::trim(sub);
			if(sub.len() || !skip) out.push_back(sub);
			i += limlen, start = i;
		}
		else ++i;
	}
	String sub = Str::substr(s, start, String::npos);
	if(trim) Str::trim(sub);
	if(sub.len() || !skip) out.push_back(sub);
	return out;
}

String Str::join(const Vector<String>& list, const char* lim)
{
	if(list.empty()) return {};

	// Determine the total String length and allocate the output string.
	int limlen = strlen(lim);
	int len = (list.size() - 1) * limlen;
	for(auto& s : list) len += s.len();
	String out(len, 0);

	// Copy the first item without the delimiter.
	char* p = out.begin();
	auto it = list.begin(), end = list.end();
	memcpy(p, it->begin(), it->len());
	p += it->len();

	// Copy each additional item with a delimiter.
	for(++it; it != end; ++it)
	{
		memcpy(p, lim, limlen);
		p += limlen;
		memcpy(p, it->begin(), it->len());
		p += it->len();
	}

	return out;
}

// ================================================================================================
// Str :: time formatting.

String Str::formatTime(double seconds, bool precise)
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

String operator + (StringRef a, char b)
{
	return Str2::cat(a.begin(), a.len(), &b, 1);
}

String operator + (char a, StringRef b)
{
	return Str2::cat(&a, 1, b.begin(), b.len());
}

String operator + (StringRef a, const char* b)
{
	return Str2::cat(a.begin(), a.len(), b, strlen(b));
}

String operator + (const char* a, StringRef b)
{
	return Str2::cat(a, strlen(a), b.begin(), b.len());
}

String operator + (StringRef a, StringRef b)
{
	return Str2::cat(a.begin(), a.len(), b.begin(), b.len());
}

String& operator += (String& a, char b)
{
	Str::append(a, b);
	return a;
}

String& operator += (String& a, const char* b)
{
	Str::append(a, b, strlen(b));
	return a;
}

String& operator += (String& a, StringRef b)
{
	Str::append(a, b.begin(), b.len());
	return a;
}

bool operator < (StringRef a, const char* b)
{
	return strcmp(a.str(), b) < 0;
}

bool operator < (const char* a, StringRef b)
{
	return strcmp(a, b.str()) < 0;
}

bool operator < (StringRef a, StringRef b)
{
	return strcmp(a.str(), b.str()) < 0;
}

bool operator > (StringRef a, const char* b)
{
	return strcmp(a.str(), b) > 0;
}

bool operator > (const char* a, StringRef b)
{
	return strcmp(a, b.str()) > 0;
}

bool operator > (StringRef a, StringRef b)
{
	return strcmp(a.str(), b.str()) > 0;
}

bool operator == (StringRef a, const char* b)
{
	return strcmp(a.str(), b) == 0;
}

bool operator == (const char* a, StringRef b)
{
	return strcmp(a, b.str()) == 0;
}

bool operator == (StringRef a, StringRef b)
{
	int len1 = a.len(), len2 = b.len();
	if(len1 != len2) return false;
	return memcmp(a.str(), b.str(), len1) == 0;
}

bool operator != (StringRef a, const char* b)
{
	return strcmp(a.str(), b) != 0;
}

bool operator != (const char* a, StringRef b)
{
	return strcmp(a, b.str()) != 0;
}

bool operator != (StringRef a, StringRef b)
{
	return !(a == b);
}

}; // namespace Vortex
