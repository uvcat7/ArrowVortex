#pragma once

namespace AV {

// Represents a 2D vector with float coordinates.
struct Vec2f
{
	// The x-coordinate.
	float x;

	// The y-coordinate.
	float y;

	// Creates a zero vector.
	constexpr Vec2f() : x(0), y(0) {}

	// Creates a vector with the given <x> and <y> coordinates.
	constexpr Vec2f(float x, float y) : x(x), y(y) {}

	// Per-component addition.
	constexpr Vec2f operator + (const Vec2f& o) const { return { x + o.x, y + o.y }; }

	// Per-component subtraction.
	constexpr Vec2f operator - (const Vec2f& o) const { return { x - o.x, y - o.y }; }

	// Returns whether or not all components are equal.
	constexpr bool operator == (const Vec2f& o) const { return x == o.x && y == o.y; }
};

} // namespace AV