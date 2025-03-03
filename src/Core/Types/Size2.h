#pragma once

namespace AV {

// Represents a 2D size as width and height.
struct Size2
{
	// The width.
	int w;

	// The height.
	int h;

	// Creates a size of zero width and zero height.
	constexpr Size2() : w(0), h(0) {}

	// Creates a size of width <w> and height <h>.
	constexpr Size2(int w, int h) : w(w), h(h) {}
};

} // namespace AV