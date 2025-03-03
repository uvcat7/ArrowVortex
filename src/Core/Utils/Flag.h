#pragma once

#include <Core/Utils/Enum.h>

namespace AV {

// Performs a bitwise AND and returns whether the result is non-zero.
template <IsFlagsType T>
constexpr bool operator & (T a, T b)
{
	static_assert(sizeof(T) <= sizeof(uint32_t));
	return (uint32_t(a) & uint32_t(b)) != 0;
}

// Performs a bitwise OR and returns the result.
template <IsFlagsType T>
constexpr T operator | (T a, T b)
{
	static_assert(sizeof(T) <= sizeof(uint32_t));
	return (T)(uint32_t(a) | uint32_t(b));
}

// Performs a bitwise XOR and returns the result.
template <IsFlagsType T>
constexpr T operator ^ (T a, T b)
{
	static_assert(sizeof(T) <= sizeof(uint32_t));
	return (T)(uint32_t(a) ^ uint32_t(b));
}

// Performs a bitwise OR and assigns the result.
template <IsFlagsType T>
constexpr void operator |= (T& a, T b)
{
	a = a | b;
}

// Performs a bitwise XOR and assigns the result.
template <IsFlagsType T>
constexpr void operator ^= (T& a, T b)
{
	a = a ^ b;
}

namespace Flag {

// Sets the mask bits in v to the given state (1 for true, 0 for false).
template <IsFlagsType T>
constexpr void set(T& v, T mask, bool state)
{
	v = (T)(state ? uint32_t(v) | uint32_t(mask) : uint32_t(v) & ~uint32_t(mask));
}

// Sets the mask bits in v to zero.
template <IsFlagsType T>
constexpr void reset(T& v, T mask)
{
	v = (T)(uint32_t(v) & ~uint32_t(mask));
}

// Returns true if all of the mask bits in v are 1, false otherwise.
template <IsFlagsType T>
constexpr bool all(T v, T mask)
{
	return (uint32_t(v) & uint32_t(mask)) == uint32_t(mask);
}

} // namespace Flag
} // namespace AV