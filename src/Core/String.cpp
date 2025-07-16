#include <Core/String.h>

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdexcept>

namespace Vortex {

// ================================================================================================
// Reference management functions
// cap[0] : capacity, cap[1] : length

static const int metaSize = sizeof(int) * 2;
static char metaData[metaSize + sizeof(char)] = {};
static char* nullstr = (char*)(metaData + metaSize);

void StrAlloc(char*& s, int n)
{
	int* cap = static_cast<int*>(malloc(metaSize + n + 1));
	if (!cap) {
		throw std::bad_alloc();
	}
	cap[0] = n; // capacity
	cap[1] = n; // length
	s = reinterpret_cast<char*>(cap + 2);
	s[n] = '\0'; // null-terminate
}

void StrRealloc(char*& s, int n)
{
	if(s != nullstr)
	{
		int* cap = reinterpret_cast<int*>(s) - 2;
		if (cap[0] < n)
		{
			int newCapacity = cap[0] * 2;
			if (newCapacity < n) {
				newCapacity = n;
			}
			cap = static_cast<int*>(realloc(cap, metaSize + newCapacity + 1));
			if (!cap) {
				throw std::bad_alloc();
			}
			cap[0] = newCapacity;
			s = reinterpret_cast<char*>(cap + 2);
		}
		cap[1] = n; // length
		s[n] = '\0'; // null-terminate
	}
	else
	{
		StrAlloc(s, n);
	}
}

void StrFree(char* s)
{
	if(s != nullstr)
	{
		free((int*)s - 2);
	}
}

void StrSetLen(char* s, int n)
{
	reinterpret_cast<int*>(s)[-1] = n; // length
	s[n] = '\0'; // null-terminate
}

const int String::npos = INT_MAX;

// ================================================================================================
// Constructors and destructor

String::~String()
{
	StrFree(string_);
}

String::String()
	: string_(nullstr)
{
}

String::String(String&& s)
	: string_(s.string_)
{
	s.string_ = nullstr;
}

String::String(StringRef s)
{
	int n = s.len();
	StrAlloc(string_, n);
	memcpy(string_, s.string_, n + 1);
}

String::String(int n, char c)
{
	StrAlloc(string_, n);
	memset(string_, c, n);
	string_[n] = 0;
}

String::String(const char* s)
{
	int n = strlen(s);
	StrAlloc(string_, n);
	memcpy(string_, s, n + 1);
}

String::String(const char* s, int n)
{
	StrAlloc(string_, n);
	memcpy(string_, s, n);
	string_[n] = 0;
}

String& String::operator = (String&& s)
{
	swap(s);
	return *this;
}

String& String::operator = (StringRef s)
{
	int n = s.len();
	StrRealloc(string_, n);
	memcpy(string_, s.string_, n + 1);
	return *this;
}

void String::swap(String& s)
{
	char* tmp = string_;
	string_ = s.string_;
	s.string_ = tmp;
}

void String::release()
{
	StrFree(string_);
	string_ = nullstr;
}

void String::clear()
{
	StrSetLen(string_, 0);
	string_[0] = 0;
}

}; // namespace Vortex
