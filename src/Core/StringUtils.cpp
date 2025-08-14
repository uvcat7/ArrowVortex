#include <Core/StringUtils.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <algorithm>

namespace Vortex {

inline int min(int a, int b) { return (a > b) ? b : a; }
inline int max(int a, int b) { return (a < b) ? b : a; }

inline char ToUpper(char c) { return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c; }
inline char ToLower(char c) { return (c >= 'A' && c <= 'Z') ? (c | 0x20) : c; }

// ================================================================================================
// Str :: append functions.

void Str::append(std::string& s, char c)
{
	s.append(1, c);
}

void Str::append(std::string& s, const std::string& str)
{
	s.append(str);
}

void Str::append(std::string& s, const char* str)
{
	s.append(str);
}

void Str::append(std::string& s, const char* str, size_t n)
{
	if(n > 0)
	{
		s.append(str, n);
	}
}

// ================================================================================================
// Str :: insert functions.

void Str::insert(std::string& s, size_t pos, char c)
{
	if (pos >= s.length()) {
		s.append(1, c);
	}
	else {
		s.insert(s.begin() + pos, c);
	}
}

void Str::insert(std::string& s, size_t pos, const std::string& str)
{
	insert(s, pos, str.c_str(), str.length());
}

void Str::insert(std::string& s, size_t pos, const char* str)
{
	insert(s, pos, str, strlen(str));
}

void Str::insert(std::string& s, size_t pos, const char* str, size_t n)
{

	if (pos >= s.length()) {
		s.append(str, n);
	}
	else if (pos + n > s.length()) {
		s = s.substr(0, pos) + std::string(str, n) + s.substr(pos);
	}
	else {
		s.insert(pos, str, n);
	}
}

// ================================================================================================
// Str :: resize functions.

void Str::truncate(std::string& s, size_t n)
{
	if(n < s.length())
	{
		if(n <= 0)
		{
			s.clear();
		}
		else
		{
			s = s.substr(0, n);
		}
	}
}

void Str::extend(std::string& s, size_t n, char c)
{
	const auto len = s.length();
	if(n > len)
	{
		s.append(n - len, c);
	}
}

void Str::resize(std::string& s, size_t n, char c)
{
	if(n > s.length())
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

	if (v == 0 && s.empty()) return alt;
	alt = v;
	return alt;
}

bool Str::read(const std::string& s, int* out)
{
	try {
		int v = static_cast<int>(std::stol(s));
		*out = v;
		return true;
	} catch (...) {
		// probably want to log the error here
		return false;
	}
}

bool Str::read(const std::string& s, uint32_t* out)
{
	try {
		uint32_t v = std::stoul(s);
		*out = v;
		return true;
	}
	catch (...) {
		// probably want to log the error here
		return false;
	}
}

bool Str::read(const std::string& s, float* out)
{
	try {
		float v = std::stof(s);
		*out = v;
		return true;
	}
	catch (...) {
		// probably want to log the error here
		return false;
	}
}

bool Str::read(const std::string& s, double* out)
{
	try {
		double v = std::stod(s);
		*out = v;
		return true;
	}
	catch (...) {
		// probably want to log the error here
		return false;
	}
}

bool Str::read(const std::string& s, bool* out)
{
	if (icompare(s, "true") == 0 || icompare(s, "yes") == 0)
	{
		*out = true;
		return true;
	}
	else if (icompare(s, "false") == 0 || icompare(s, "no") == 0)
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
	// from the front
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
		return !IsWhiteSpace(c);
	}));

	// from the back
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
		return !IsWhiteSpace(c);
	}).base(), s.end());
}

void Str::simplify(std::string& s)
{
	trim(s);
	
	// remove duplicates in the string, but only adjacent duplicate spaces
	// get an iterator for that
	auto it = std::unique(s.begin(), s.end(), [](char a, char b) {
		return a == b && IsWhiteSpace(a);
	});
	// and then erase everything in the iterator
	s.erase(it, s.end());
}

void Str::erase(std::string& s, size_t pos, size_t n)
{
	auto len = s.length();
	if(pos < 0)	{ n += pos, pos = 0; }
	if(pos == 0 && n >= len)
	{
		s.clear();
	}
	else if(pos < len && n > 0)
	{
		n = min(n, len - pos);
		s.erase(s.begin() + pos, s.begin() + pos + n);
	}
}

void Str::pop_back(std::string& s)
{
	auto len = s.length();
	if(len)
	{
		s.pop_back();
	}
}

void Str::replace(std::string& s, char find, char replace)
{
	std::replace(s.begin(), s.end(), find, replace);
}

void Str::replace(std::string& s, const char* fnd, const char* rep)
{
	if(*fnd == 0) return;

	auto pos = find(s, fnd);
	if(pos == std::string::npos) return;
	const auto replen = strlen(rep);
	const auto fndlen = strlen(fnd);

	while (pos != std::string::npos) {
		s.erase(s.begin() + pos, s.begin() + pos + fndlen);
		insert(s, pos, rep);
		pos = find(s, fnd);
	}
}

void Str::toUpper(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ToUpper);
}

void Str::toLower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ToLower);
}

// ================================================================================================
// Information functions

std::string Str::substr(const std::string& s, size_t pos, size_t n)
{
	auto len = s.length();
	if(pos < 0) { n += pos, pos = 0; }
	if(pos == 0 && n >= len)
	{
		return s;
	}
	else if(pos < len && n > 0)
	{
		n = min(n, len - pos);
		return s.substr(pos, n);
	}
	return {};
}

size_t Str::nextChar(const std::string& s, size_t pos)
{
	auto len = s.length();
	if(pos >= len) return std::string::npos;
	do { ++pos; } while(pos < len && (s.at(pos) & 0xC0) == 0x80);
	return pos;
}

size_t Str::prevChar(const std::string& s, size_t pos)
{
	if(pos <= 0) return -1;
	do { --pos; } while(pos >= 0 && (s.at(pos) & 0xC0) == 0x80);
	return pos;
}

bool Str::isUnicode(const std::string& s)
{
	for(char c : s) { if(c & 0x80) return true; }
	return false;
}

size_t Str::find(const std::string& s, char c, size_t pos)
{
	auto len = s.length();
	pos = max(pos, 0);
	pos = s.find(c, pos);
	return (pos < len) ? pos : std::string::npos;
}

size_t Str::find(const std::string& s, const char* str, size_t pos)
{
	pos = max(pos, 0);
	auto len = s.length();

	if(*str == 0 && pos <= len)
		return pos;

	return s.find(str, pos);
}

size_t Str::findLast(const std::string& s, char c, size_t pos)
{
	auto len = s.length();
	pos = min(pos, len - 1);
	pos = s.find_last_of(c, pos);

	if (pos == std::string::npos) {
		pos = -1;
	}

	return (pos >= 0) ? pos : -1;
}

size_t Str::findAnyOf(const std::string& s, const char* c, size_t pos)
{
	auto len = s.length();
	pos = max(pos, 0);
	while(pos < len)
	{
		const char a = s.at(pos), *b = c;
		while(*b && *b != a) ++b;
		if(*b) break;
		++pos;
	}
	return (pos < len) ? pos : std::string::npos;
}

size_t Str::findLastOf(const std::string& s, const char* c, size_t pos)
{
	auto len = s.length();
	pos = min(pos, len - 1);
	while(pos >= 0)
	{
		const char a = s.at(pos), *b = c;
		while(*b && *b != a) ++b;
		if(*b) break;
		--pos;
	}
	return (pos >= 0) ? pos : -1;
}

// ================================================================================================
// Str :: compare functions.

static int fastnocasecmp(const char* pStrA, const char* pStrB, size_t lim = 0) {
	char a, b;
	if (lim > 0) {
		do {
			a = ToLower(*(pStrA++));
			b = ToLower(*(pStrB++));
		} while ((--lim > 0) && (a) && a == b);
	}
	else {
		do {
			a = ToLower(*(pStrA++));
			b = ToLower(*(pStrB++));
		} while ((a) && a == b);
	}
	return static_cast<int>(a - b);
}

static bool Equals(const char* a, const char* b, size_t len, bool caseSensitive)
{
	if(caseSensitive)
	{
		return strncmp(a, b, len) == 0;
	}
	else
	{
		return fastnocasecmp(a, b, len) == 0;
	}
}

bool Str::endsWith(const std::string& s, const char* suffix, bool useCase)
{
	auto len = s.length(), n = strlen(suffix);
	if(len >= n) return Equals(s.data() + len - n, suffix, n, useCase);
	return false;
}

bool Str::startsWith(const std::string& s, const char* prefix, bool useCase)
{
	auto len = s.length(), n = strlen(prefix);
	if(len >= n) return Equals(s.data(), prefix, n, useCase);
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
	return fastnocasecmp(a.c_str(), b.c_str());
}

int Str::icompare(const std::string& a, const char* b)
{
	return fastnocasecmp(a.c_str(), b);
}

int Str::icompare(const char* a, const char* b)
{
	return fastnocasecmp(a, b);
}

bool Str::equal(const std::string& a, const std::string& b)
{
	return compare(a, b) == 0;
}

bool Str::equal(const std::string& a, const char* b)
{
	return compare(a, b) == 0;
}

bool Str::equal(const char* a, const char* b)
{
	return compare(a, b) == 0;
}

bool Str::iequal(const std::string& a, const std::string& b)
{
	return icompare(a, b) == 0;
}

bool Str::iequal(const std::string& a, const char* b)
{
	return icompare(a, b) == 0;
}

bool Str::iequal(const char* a, const char* b)
{
	return icompare(a, b) == 0;
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

Fmt& Fmt::arg(const char* s, size_t n)
{
	auto fmtLen = str.length();

	// find the lowest marker position.
	auto markerPos = fmtLen;
	auto markerLen = 0;
	if(fmtLen > 0)
	{
		size_t lowestMarker = 100;
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
	str.erase(str.begin() + markerPos, str.begin() + markerPos + markerLen);
	insert(str, markerPos, s, n);

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
	if (s.empty()) {
		return out;
	}

	auto it = s.begin();
	while(IsWhiteSpace(*it) && it != s.end()) ++it;
	while(it != s.end())
	{
		auto cur = it;
		while(cur != s.end() && !IsWhiteSpace(*cur)) ++cur;
		out.push_back(std::string(it, cur));
		while(it != s.end() && IsWhiteSpace(*it)) ++it;
	}
	return out;
}

Vector<std::string> Str::split(const std::string& s, const char* lim, bool trim, bool skip)
{
	Vector<std::string> out;
	auto limlen = strlen(lim);
	auto slen = s.length() - limlen;
	size_t start = 0;
	for(size_t i = 0; i <= slen;)
	{
		if(memcmp(s.data() + i, lim, limlen) == 0)
		{
			std::string sub = Str::substr(s, start, i - start);
			if(trim) Str::trim(sub);
			if(sub.length() || !skip) out.push_back(sub);
			i += limlen, start = i;
		}
		else ++i;
	}
	std::string sub = Str::substr(s, start, std::string::npos);
	if(trim) Str::trim(sub);
	if(sub.length() || !skip) out.push_back(sub);
	return out;
}

std::string Str::join(const Vector<std::string>& list, const char* lim)
{
	if(list.empty()) return {};

	// Determine the total String length and allocate the output string.
	auto limlen = strlen(lim);
	auto len = (list.size() - 1) * limlen;
	for(auto& s : list) len += s.length();
	std::string out;
	out.reserve(len);

	// Copy the first item without the delimiter.
	auto it = list.begin(), end = list.end();
	out.append(*it);

	// Copy each additional item with a delimiter.
	for(++it; it != end; ++it)
	{
		out.append(lim);
		out.append(*it);
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

}; // namespace Vortex
