#pragma once

#include <Core/Types/Vec2.h>
#include <Core/Types/Size2.h>

namespace AV {

// Represents a 2D rectangle as left, top, right, and bottom floating point coordinates.
struct Rectf
{
	// The x-position of the left side.
	float l;

	// The y-position of the top side.
	float t;

	// The x-position of right side.
	float r;

	// The y-position of the bottom side.
	float b;

	// Creates a rectangle with all zero coordinates.
	inline Rectf() : l(0), t(0), r(0), b(0) {}

	// Creates a rectangle with the given coordinates.
	inline Rectf(float l, float t, float r, float b) : l(l), t(t), r(r), b(b) {}
};

} // namespace AV