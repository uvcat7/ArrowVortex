#include <Core/String.h>

#include <string.h>
#include <limits.h>
#include <stdlib.h>

namespace Vortex {

// ================================================================================================
// Reference management functions
// cap[0] : capacity, cap[1] : length

static const int metaSize = sizeof(int) * 2;
static char metaData[metaSize + sizeof(char)] = {};
static char* nullstr = (char*)(metaData + metaSize);

void StrAlloc(char*& s, int n)
{
	int* cap = (int*)malloc(metaSize + n + 1);
	cap[0] = cap[1] = n;
	s = (char*)(cap + 2);
}

void StrRealloc(char*& s, int n)
{
	if(s != nullstr)
	{
		int* cap = (int*)s - 2;
		if(*cap < n)
		{
			*cap = *cap << 1;
			if(*cap < n) *cap = n;
			cap = (int*)realloc(cap, metaSize + *cap + 1);
			s = (char*)(cap + 2);
		}
		cap[1] = n;
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
	*((int*)s - 1) = n;
}

const int String::npos = INT_MAX;

// ================================================================================================
// Constructors and destructor

String::~String()
{
	StrFree(myStr);
}

String::String()
	: myStr(nullstr)
{
}

String::String(String&& s)
	: myStr(s.myStr)
{
	s.myStr = nullstr;
}

String::String(StringRef s)
{
	int n = s.len();
	StrAlloc(myStr, n);
	memcpy(myStr, s.myStr, n + 1);
}

String::String(int n, char c)
{
	StrAlloc(myStr, n);
	memset(myStr, c, n);
	myStr[n] = 0;
}

String::String(const char* s)
{
	int n = strlen(s);
	StrAlloc(myStr, n);
	memcpy(myStr, s, n + 1);
}

String::String(const char* s, int n)
{
	StrAlloc(myStr, n);
	memcpy(myStr, s, n);
	myStr[n] = 0;
}

String& String::operator = (String&& s)
{
	swap(s);
	return *this;
}

String& String::operator = (StringRef s)
{
	int n = s.len();
	StrRealloc(myStr, n);
	memcpy(myStr, s.myStr, n + 1);
	return *this;
}

void String::swap(String& s)
{
	char* tmp = myStr;
	myStr = s.myStr;
	s.myStr = tmp;
}

void String::release()
{
	StrFree(myStr);
	myStr = nullstr;
}

void String::clear()
{
	StrSetLen(myStr, 0);
	myStr[0] = 0;
}

}; // namespace Vortex
