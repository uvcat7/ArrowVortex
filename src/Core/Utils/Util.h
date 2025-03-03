#pragma once

#include <Precomp.h>

namespace AV {
namespace Util {

#define TT template <typename T>

// Deletes a pointer and sets it to null.
TT void reset(T*& p)
{
	delete p;
	p = nullptr;
}

// Returns the value of <x> restricted to the range <min, max>.
TT constexpr const T& clamp(const T& x, const T& min, const T& max)
{
	return (x < min) ? min : ((max < x) ? max : x);
}

// Swaps the values of <high> and <low> if <high> is less than <low>.
TT constexpr void sort(T& low, T& high)
{
	if (high < low)
		std::swap(low, high);
}

// Returns the result of linear interpolation between <begin> and <end> with factor <t>.
TT constexpr T lerp(T begin, T end, T t)
{
	return begin + (end - begin) * t;
}

// Allows string literals to be used as template parameters.
template <size_t N>
struct StringLiteral
{
	constexpr StringLiteral(const char(&s)[N]) { std::copy_n(s,N,value); }
    char value[N];
};

// Reverse iteration.

TT struct reverseIterationWrapper { T& src; };
TT auto begin(reverseIterationWrapper<T> w) { return std::rbegin(w.src); }
TT auto end(reverseIterationWrapper<T> w) { return std::rend(w.src); }

// Returns a wrapper that iterates the input in reverse.
TT reverseIterationWrapper<T> reverseIterate(T&& iterable) { return { iterable }; }

#undef TT

} // namespace Util
} // namespace AV