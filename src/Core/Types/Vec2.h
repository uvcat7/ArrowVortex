#pragma once

namespace AV {

// Represents a 2D vector with integer coordinates.
struct Vec2
{
	// The x-coordinate.
	int x;

	// The y-coordinate.
	int y;

	// Creates a zero vector.
	constexpr Vec2() : x(0), y(0) {}

	// Creates a vector with the given <x> and <y> coordinates.
	constexpr Vec2(int x, int y) : x(x), y(y) {}

	// Returns a copy of the vector that is offset by the given x-delta.
	constexpr Vec2 offsetX(int dx) const { return {x + dx, y}; }

	// Returns a copy of the vector that is offset by the given y-delta.
	constexpr Vec2 offsetY(int dy) const { return {x, y + dy}; }

	// Returns a copy of the vector that is offset by the given x-delta and y-delta.
	constexpr Vec2 offset(int dx, int dy) const { return { x, y + dy }; }

	// Per-component addition.
	constexpr Vec2 operator + (const Vec2& o) const { return {x + o.x, y + o.y}; }

	// Per-component subtraction.
	constexpr Vec2 operator - (const Vec2& o) const { return {x - o.x, y - o.y}; }

	// Returns whether or not all components are equal.
	constexpr bool operator == (const Vec2& o) const { return x == o.x && y == o.y; }
};

} // namespace AV