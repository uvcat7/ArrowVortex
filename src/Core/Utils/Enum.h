#pragma once

#include <Precomp.h>

namespace AV {

#define TT template <typename T>

// If a template specialization sets this to true, flag operations will be available for the enum.
TT constexpr bool IsFlags = false;

// Matches all types for which IsFlags<T> is true.
TT concept IsFlagsType = IsFlags<T>;

namespace Enum {

// Converts the given enum value to a string.
// Note: must be implemented explicitly for the enum in a source file.
TT const char* toString(T value) requires std::is_enum_v<T>;

// Converts the given string to an enum value.
// Note: must be implemented explicitly for the enum in a source file.
TT std::optional<T> fromString(stringref str) requires std::is_enum_v<T>;

// Adds an amount to an enum value via casting to int.
TT constexpr T add(T value, int amount)
{
	static_assert(std::is_enum<T>::value);
	return T(int(value) + amount);
}

// Increments the given value by 1.
TT constexpr T next(T value) { return add(value, 1); }

// Decrements the given value by 1.
TT constexpr T previous(T value) { return add(value, -1); }

// Increments the given value by 1, wrapped within range [0, count).
TT constexpr T nextWrapped(T value, T count)
{
	static_assert(std::is_enum<T>::value);
	return T((int(value) + 1) % int(count));
}

// Decrements the given value by 1, wrapped within range [0, count).
TT constexpr T previousWrapped(T value, T count)
{
	static_assert(std::is_enum<T>::value);
	return T((int(value) + int(count) - 1) % int(count));
}

#undef TT

} // namespace Enum
} // namespace AV