#include <Precomp.h>

#include <inttypes.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Ascii.h>

#pragma warning(disable : 4996) // stricmp.

namespace AV {

using namespace std;

static bool Equals(const char* a, const char* b, size_t len, bool caseSensitive)
{
	if (caseSensitive)
		return strncmp(a, b, len) == 0;
	else
		return strnicmp(a, b, len) == 0;
}

void String::replace(string& s, const char* find, const char* replace)
{
	size_t index = 0;
	auto flen = strlen(find);
	auto rlen = strlen(replace);
	while (true)
	{
		index = s.find(find, index, flen);
		if (index == string::npos)
			break;
		s.replace(index, flen, replace, rlen);
		index += rlen;
	}
}

void String::truncate(string& s, size_t n)
{
	if (s.length() > n)
		s.resize(n);
}

// =====================================================================================================================
// Value conversion.

bool String::readInt(const char* str, int& outValue, int& outCharsRead)
{
	char* end;
	int v = strtol(str, &end, 10);
	if (v == 0 && str == end)
	{
		outCharsRead = 0;
		return false;
	}
	outCharsRead = int(end - str);
	outValue = v;
	return true;
}

bool String::readUint(const char* str, uint& outValue, int& outCharsRead)
{
	char* end;
	uint v = strtoul(str, &end, 10);
	if (v == 0 && str == end)
	{
		outCharsRead = 0;
		return false;
	}
	outCharsRead = int(end - str);
	outValue = v;
	return true;
}

bool String::readFloat(const char* str, float& outValue, int& outCharsRead)
{
	char* end;
	float v = strtof(str, &end);
	if (v == 0 && str == end)
	{
		outCharsRead = 0;
		return false;
	}
	outCharsRead = int(end - str);
	outValue = v;
	return true;
}

bool String::readDouble(const char* str, double& outValue, int& outCharsRead)
{
	char* end;
	double v = strtod(str, &end);
	if (v == 0 && str == end)
	{
		outCharsRead = 0;
		return false;
	}
	outCharsRead = int(end - str);
	outValue = v;
	return true;
}

bool String::readBool(const char* str, bool& outValue, int& outCharsRead)
{
	if (strncmp(str, "true", 4) == 0)
	{
		outValue = true;
		outCharsRead = 4;
		return true;
	}
	if (strncmp(str, "false", 5) == 0)
	{
		outValue = false;
		outCharsRead = 5;
		return true;
	}
	outCharsRead = 0;
	return false;
}

bool String::readInt(stringref s, int& out)
{
	int n;
	return String::readInt(s.data(), out, n);
}

bool String::readUint(stringref s, uint& out)
{
	int n;
	return String::readUint(s.data(), out, n);
}

bool String::readFloat(stringref s, float& out)
{
	int n;
	return String::readFloat(s.data(), out, n);
}

bool String::readDouble(stringref s, double& out)
{
	int n;
	return String::readDouble(s.data(), out, n);
}

bool String::readBool(stringref s, bool& out)
{
	int n;
	return String::readBool(s.data(), out, n);
}

int String::toInt(stringref s, int alt)
{
	int n;
	String::readInt(s.data(), alt, n);
	return alt;
}

uint String::toUint(stringref s, uint alt)
{
	int n;
	String::readUint(s.data(), alt, n);
	return alt;
}

float String::toFloat(stringref s, float alt)
{
	int n;
	String::readFloat(s.data(), alt, n);
	return alt;
}

double String::toDouble(stringref s, double alt)
{
	int n;
	String::readDouble(s.data(), alt, n);
	return alt;
}

bool String::toBool(stringref s, bool alt)
{
	int n;
	String::readBool(s.data(), alt, n);
	return alt;
}

bool String::readInts(stringref s, int* out, size_t n, char separator)
{
	int nread;
	auto p = s.data();
	for (int i = 0; i < n; ++i)
	{
		if (!String::readInt(p, out[i], nread)) return false;
		p += nread;
		while (Ascii::isSpace(*p)) ++p;
		if (*p == separator) ++p;
	}
	return true;
}

bool String::readUints(stringref s, uint* out, size_t n, char separator)
{
	int nread;
	auto p = s.data();
	for (int i = 0; i < n; ++i)
	{
		if (!String::readUint(p, out[i], nread)) return false;
		p += nread;
		while (Ascii::isSpace(*p)) ++p;
		if (*p == separator) ++p;
	}
	return true;
}

bool String::readFloats(stringref s, float* out, size_t n, char separator)
{
	int nread;
	auto p = s.data();
	for (int i = 0; i < n; ++i)
	{
		if (!String::readFloat(p, out[i], nread)) return false;
		p += nread;
		while (Ascii::isSpace(*p)) ++p;
		if (*p == separator) ++p;
	}
	return true;
}

bool String::readDoubles(stringref s, double* out, size_t n, char separator)
{
	int nread;
	auto p = s.data();
	for (int i = 0; i < n; ++i)
	{
		if (!String::readDouble(p, out[i], nread)) return false;
		p += nread;
		while (Ascii::isSpace(*p)) ++p;
		if (*p == separator) ++p;
	}
	return true;
}

bool String::readBools(stringref s, bool* out, size_t n, char separator)
{
	int nread;
	auto p = s.data();
	for (int i = 0; i < n; ++i)
	{
		if (!String::readBool(p, out[i], nread)) return false;
		p += nread;
		while (Ascii::isSpace(*p)) ++p;
		if (*p == separator) ++p;
	}
	return true;
}

// =====================================================================================================================
//  expression parsing.

static const uchar* ParseExpression(const uchar* p, double& out);

inline const uchar* SkipWs(const uchar* p)
{
	while (*p == ' ' || *p == '\t') ++p;
	return p;
}

static const uchar* ParseNumber(const uchar* p, double& out)
{
	// Digits leading up to the decimal-point.
	uint64_t sum = 0;
	for (; *p >= '0' && *p <= '9'; ++p)
	{
		sum = sum * 10 + (*p - '0');
	}
	out = (double)sum;

	// Digits after the decimal-point.
	if (*p == '.' || *p == ',')
	{
		uint64_t dec = 0;
		int digits = 0;
		for (++p; *p >= '0' && *p <= '9'; ++p, ++digits)
		{
			dec = dec * 10 + (*p - '0');
		}
		out += (double)dec / pow(10.0, digits);
	}

	// Optional exponent.
	if (*p == 'e' || *p == 'E')
	{
		++p;

		// Plus/minus sign.
		bool neg = false;
		if (*p == '-' || *p == '+')
		{
			neg = (*p++ == '-');
		}

		// Exponent digits.
		int exp = 0;
		for (; *p >= '0' && *p <= '9'; ++p)
		{
			exp = exp * 10 + (*p - '0');
		}
		if (neg) exp = -exp;

		out *= pow(10.0, exp);
	}

	return SkipWs(p);
}

static const uchar* ParseOperandWithSign(const uchar* p, double& out)
{
	char sign = *p;
	p = SkipWs(++p);
	if (*p == '(')
	{
		p = ParseExpression(SkipWs(++p), out);
		if (*p == ')') p = SkipWs(++p);
	}
	else if ((*p >= '0' && *p <= '9') || *p == '.')
	{
		p = ParseNumber(p, out);
	}
	if (sign == '-') out = -out;
	return p;
}

static const uchar* ParseMultiplicationOperand(const uchar* p, double& out)
{
	if (*p == '(')
	{
		p = ParseExpression(SkipWs(++p), out);
		if (*p == ')') p = SkipWs(++p);
	}
	else if ((*p >= '0' && *p <= '9') || *p == '.')
	{
		p = ParseNumber(p, out);
	}
	else if (*p == '+' || *p == '-')
	{
		p = ParseOperandWithSign(p, out);
	}
	return p;
}

static const uchar* ParseAdditionOperand(const uchar* p, double& out)
{
	p = ParseMultiplicationOperand(p, out);
	while (*p == '*' || *p == '/')
	{
		char op = *p;
		double rhs = 0.0;
		p = ParseMultiplicationOperand(SkipWs(++p), rhs);
		if (op == '*') out *= rhs; else out /= rhs;
	}
	return p;
}

static const uchar* ParseExpression(const uchar* p, double& out)
{
	p = ParseAdditionOperand(p, out);
	while (*p == '+' || *p == '-')
	{
		char op = *p;
		double rhs = 0.0;
		p = ParseAdditionOperand(SkipWs(++p), rhs);
		if (op == '+') out += rhs; else out -= rhs;
	}
	return p;
}

bool String::evaluateExpression(stringref s, double& out)
{
	double result = 0.0;
	auto begin = SkipWs((const uchar*)s.data());
	auto p = ParseExpression(begin, result);
	if (p > begin) out = result;
	return (p > begin);
}

// =====================================================================================================================
// modifying functions.

void String::toUpper(string& s)
{
	transform(s.begin(), s.end(), s.begin(), Ascii::toUpper);
}

void String::toLower(string& s)
{
	transform(s.begin(), s.end(), s.begin(), Ascii::toLower);
}

void String::collapseWhitespace(std::string& s)
{
	bool writeSpace = false;
	char* p = s.data();
	for (char* c = p; *c;)
	{
		if (Ascii::isSpace(*c))
		{
			*p++ = ' ';
			do { ++c; } while (Ascii::isSpace(*c));
		}
		else *p++ = *c++;
	}
	s.resize(p - s.data());
}

// =====================================================================================================================
// Information functions

size_t String::find(stringref s, char c, size_t pos)
{
	auto len = s.length();
	auto str = s.data();
	while (pos < len && str[pos] != c)
		++pos;
	return (pos < len) ? pos : string::npos;
}

size_t String::find(stringref s, const char* fnd, size_t pos)
{
	auto len = s.length();

	if (*fnd == 0 && pos <= len)
		return pos;

	auto str = s.data();
	for (size_t i = pos; i < len; ++i)
	{
		if (str[i] == *fnd)
		{
			const char* a = fnd, *b = str + i;
			while (*a && *a == *b) ++a, ++b;
			if (*a == 0) return i;
		}
	}

	return string::npos;
}

size_t String::findLast(stringref s, char c, size_t pos)
{
	auto len = s.length();
	if (len == 0)
		return string::npos;

	pos = min(pos, len - 1);
	const char* str = s.data();
	while (pos >= 0 && str[pos] != c) --pos;
	return (pos >= 0) ? pos : -1;
}

size_t String::findAnyOf(stringref s, const char* c, size_t pos)
{
	auto len = s.length();
	auto str = s.data();
	while (pos < len)
	{
		const char a = str[pos], *b = c;
		while (*b && *b != a)
			++b;
		if (*b)
			break;
		++pos;
	}
	return (pos < len) ? pos : string::npos;
}

size_t String::findLastOf(stringref s, const char* c, size_t pos)
{
	auto len = s.length();
	if (len == 0) return string::npos;
	pos = min(pos, len - 1);
	auto str = s.data();
	while (pos >= 0)
	{
		const char a = str[pos], *b = c;
		while (*b && *b != a)
			++b;
		if (*b)
			break;
		--pos;
	}
	return (pos >= 0) ? pos : -1;
}

// =====================================================================================================================
// compare functions.

bool String::endsWith(stringref s, const char* suffix, bool caseSensitive)
{
	auto len = s.length();
	auto n = strlen(suffix);
	if (len < n) return false;
	return Equals(s.data() + len - n, suffix, n, caseSensitive);
}

bool String::startsWith(stringref s, const char* prefix, bool caseSensitive)
{
	auto len = s.length();
	auto n = strlen(prefix);
	if (len < n) return false;
	return Equals(s.data(), prefix, n, caseSensitive);
}

string String::head(stringref s)
{
	auto p = s.data();
	while (*p && !Ascii::isSpace(*p)) ++p;
	return string(s.data(), p);
}

std::string String::tail(stringref s)
{
	auto p = s.data();
	while (*p && !Ascii::isSpace(*p)) ++p;
	while (Ascii::isSpace(*p)) ++p;
	return string(p);
}

bool String::equal(const char* a, const char* b)
{
	return strcmp(a, b) == 0;
}

bool String::iequal(stringref a, stringref b)
{
	return a.compare(b) == 0;
}

bool String::iequal(stringref a, const char* b)
{
	return a.compare(b) == 0;
}

bool String::iequal(const char* a, stringref b)
{
	return b.compare(a) == 0;
}

bool String::iequal(const char* a, const char* b)
{
	return stricmp(a, b) == 0;
}

// =====================================================================================================================
// formatting functions.

static constexpr int IntBuflen = 64;
static constexpr int DblBuflen = 128;

static int PrintInt(char* buf, int64_t v, int minDig, bool hex)
{
	int len;
	if (minDig <= 0)
	{
		len = _snprintf(buf, IntBuflen, hex ? ("%" PRIX64) : ("%" PRIi64), v);
	}
	else
	{
		len = _snprintf(buf, IntBuflen, hex ? ("%0*" PRIX64) : ("%0*" PRIi64), minDig, v);
	}
	return (len < 0) ? DblBuflen : len;
}

static int PrintUint(char* buf, uint64_t v, int minDig, bool hex)
{
	int len;
	if (minDig <= 0)
	{
		len = _snprintf(buf, IntBuflen, hex ? ("%" PRIX64) : ("%" PRIu64), v);
	}
	else
	{
		len = _snprintf(buf, IntBuflen, hex ? ("%0*" PRIX64) : ("%0*" PRIu64), minDig, v);
	}
	return (len < 0) ? IntBuflen : len;
}

static int PrintDouble(char* buf, double v, int minDec, int maxDec)
{
	minDec = min(max(minDec, 0), 16);
	maxDec = min(max(minDec, maxDec), 16);

	// Print the value.
	int len = _snprintf(buf, DblBuflen, "%.*f", maxDec, v);
	if (len < 0) len = DblBuflen;

	// Cap the number of decimal digits.
	int decimalPoint = len - 1;
	while (decimalPoint >= 0 && buf[decimalPoint] != '.') --decimalPoint;
	if (decimalPoint >= 0)
	{
		int decimalDigits = len - decimalPoint - 1;

		// Remove trailing zeroes.
		while (decimalDigits > minDec && buf[len - 1] == '0')
		{
			--len, --decimalDigits;
		}

		// Remove the decimal point if there are no decimal digits left.
		if (decimalDigits == 0 && decimalPoint >= 0)
		{
			--len;
		}
	}

	return len;
}

static const char* BoolOptions[] = {"false", "FALSE", "true", "TRUE"};

string String::fromBool(bool v, bool upperCase)
{
	return string(BoolOptions[v * 2 + upperCase]);
}

string String::fromInt(int64_t v, int minDig, bool hex)
{
	char buf[IntBuflen];
	return string(buf, PrintInt(buf, v, minDig, hex));
}

string String::fromUint(uint64_t v, int minDig, bool hex)
{
	char buf[IntBuflen];
	return string(buf, PrintUint(buf, v, minDig, hex));
}

string String::fromFloat(float v, int minDec, int maxDec)
{
	char buf[DblBuflen];
	return string(buf, PrintDouble(buf, v, minDec, maxDec));
}

string String::fromDouble(double v, int minDec, int maxDec)
{
	char buf[DblBuflen];
	return string(buf, PrintDouble(buf, v, minDec, maxDec));
}

void String::appendBool(string& s, bool v, bool upperCase)
{
	s.append(BoolOptions[v * 2 + upperCase]);
}

void String::appendInt(string& s, int64_t v, int minDig, bool hex)
{
	char buf[IntBuflen];
	s.append(buf, PrintInt(buf, v, minDig, hex));
}

void String::appendUint(string& s, uint64_t v, int minDig, bool hex)
{
	char buf[IntBuflen];
	s.append(buf, PrintUint(buf, v, minDig, hex));
}

void String::appendFloat(string& s, float v, int minDec, int maxDec)
{
	char buf[DblBuflen];
	s.append(buf, PrintDouble(buf, v, minDec, maxDec));
}

void String::appendDouble(string& s, double v, int minDec, int maxDec)
{
	char buf[DblBuflen];
	s.append(buf, PrintDouble(buf, v, minDec, maxDec));
}

// =====================================================================================================================
// string splitting and joining.

vector<string> String::split(stringref s)
{
	vector<string> result;
	const char* p = s.data();
	while (Ascii::isSpace(*p)) ++p;
	while (*p)
	{
		const char* start = p;
		while (*p && !Ascii::isSpace(*p)) ++p;
		result.emplace_back(start, p - start);
		while (Ascii::isSpace(*p)) ++p;
	}
	return result;
}

vector<string> String::split(stringref s, const char* delimiter, bool trim, bool skipEmpty)
{
	vector<string> result;
	size_t a = 0;
	auto data = s.data();
	auto slen = s.length();
	auto dlen = strlen(delimiter);
	do
	{
		auto b = s.find(delimiter, a);
		if (b == string::npos) b = slen;
		if (trim)
		{
			while (a != b && Ascii::isSpace(data[a + 0])) ++a;
			while (a != b && Ascii::isSpace(data[b - 1])) --b;
			if (a != b || !skipEmpty)
				result.emplace_back(data + a, data + b);
		}
		else
		{
			if (!skipEmpty || !all_of(data + a, data + b, Ascii::isSpace))
				result.emplace_back(data + a, data + b);
		}
		a = b + dlen;
	}
	while (a < slen);
	return result;
}

bool String::splitOnce(stringref s, std::string& outLeft, std::string& outRight)
{
	auto a = s.data();
	while (Ascii::isSpace(*a)) ++a;
	while (*a && !Ascii::isSpace(*a)) ++a;
	if (*a == 0) return false;
	auto b = a;
	while (*b && Ascii::isSpace(*b)) ++b;
	if (*b == 0) return false;
	outLeft.assign(s.data(), a);
	outRight.assign(b, s.data() + s.length());
	return true;
}

bool String::splitOnce(stringref s, std::string& outLeft, std::string& outRight, const char* delimiter, bool trim)
{
	auto splitPos = s.find(delimiter, 0);
	if (splitPos == string::npos)
		return false;

	auto a = s.data();
	auto b = a + splitPos;
	auto c = b + strlen(delimiter);
	auto d = a + s.length();

	if (trim)
	{
		while (b > a && Ascii::isSpace(b[-1])) --b;
		while (d > c && Ascii::isSpace(d[-1])) --d;
	}

	outLeft.assign(a, b);
	outRight.assign(c, d);
	return true;
}

string String::join(const vector<string>& list, const char* delimiter)
{
	if (list.empty())
		return string();

	// Determine the total string length and allocate the output string.
	auto delimiterLength = strlen(delimiter);
	size_t len = (list.size() - 1) * delimiterLength;
	for (auto& s : list)
		len += s.length();

	string result(len, '\0');

	// Copy the first item without the delimiter.
	char* p = &result[0];
	auto it = list.begin(), end = list.end();
	memcpy(p, it->data(), it->length());
	p += it->length();

	// Copy each additional item with a delimiter.
	for (++it; it != end; ++it)
	{
		memcpy(p, delimiter, delimiterLength);
		p += delimiterLength;
		memcpy(p, it->data(), it->length());
		p += it->length();
	}

	return result;
}

// =====================================================================================================================
// startTime formatting.

string String::formatTime(double seconds, bool showMilliseconds)
{
	bool negative = false;
	if (seconds < 0.0)
	{
		negative = true;
		seconds = -seconds;
	}

	int64_t t = (int64_t)(seconds * 1000.0);
	int64_t min = t / (60 * 1000);
	t -= min * (60 * 1000);
	int64_t sec = t / 1000;

	auto str = format("{:02d}:{:02d}.", min, sec);

	if (showMilliseconds)
	{
		t -= sec * 1000;
		appendInt(str, (int)t, 3);
	}
	else
	{
		t -= sec * 1000;
		t /= 100;
		appendInt(str, (int)t);
	}

	if (negative)
	{
		str.insert(str.begin(), '-');
	}

	return str;
}

} // namespace AV
