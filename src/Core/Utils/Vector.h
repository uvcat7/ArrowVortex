#pragma once

#include <Precomp.h>

namespace AV {
namespace Vector {

#define TT template <typename T>

// Resizes to <n> elements, assigning <val> to added elements.
TT inline void resize(vector<T>& v, T val, size_t n)
{
	auto s = v.size();
	v.resize(n);
	for (auto a = v.begin() + s, b = v.end(); a != b; ++a)
		*a = val;
}

// Destroys all elements in the vector and emtpies the vector.
TT inline void destroyElements(vector<T*>& v)
{
	for (T* v : v) delete v;
	v.clear();
}

// Returns the index of the first element on or after pos that equals value.
// If no matching element is found, the vector size is returned.
TT inline int find(const vector<T>& v, const T& value, int pos = 0)
{
	auto size = (int)v.size();
	auto data = v.begin();
	while (pos < size && data[pos] != value) ++pos;
	return pos;
}

// Returns true if the vector contains one or more elements that equal value.
TT inline bool contains(const vector<T>& v, const T& value)
{
	return find(v, value) != v.size();
}

// Removes all elements that are equal to value from the vector.
TT inline void eraseValues(vector<T>& v, const T& value)
{
	auto data = v.begin();
	for (int i = (int)v.size() - 1; i >= 0; --i)
	{
		if (data[i] == value)
			v.erase(v.begin() + i);
	}
}

// Erases the last <n> elements of the vector.
TT inline void pop(vector<T>& v, int n = 1)
{
	v.resize(v.size() - n);
}

// Makes sure vector <v> has at most <n> elements.
TT inline void truncate(vector<T>& v, int n)
{
	if ((int)v.size() > n)
		v.resize(n);
}

// Makes sure vector <v> has at least <n> elements.
TT inline void grow(vector<T>& v, int n)
{
	if ((int)v.size() < n)
		v.resize(n);
}

// Makes sure vector <v> has at least <n> elements.
TT inline void grow(vector<T>& v, int n, const T& val)
{
	if ((int)v.size() < n)
		v.resize(n, val);
}

#undef TT

} // namespace Vector
} // namespace AV