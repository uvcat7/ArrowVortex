#pragma once

#include <Core/Types/Vec2.h>
#include <Core/Types/Size2.h>

namespace AV {

// Represents an 2D rectangle as left, top, right, and bottom integer coordinates.
struct Rect
{
	// The x-position of the left side.
	int l;

	// The y-position of the top side.
	int t;

	// The x-position of right side.
	int r;

	// The y-position of the bottom side.
	int b;

	// Creates a rectangle with all zero coordinates.
	constexpr Rect() : l(0), t(0), r(0), b(0) {}

	// Creates a rectangle with the given coordinates.
	constexpr Rect(int l, int t, int r, int b) : l(l), t(t), r(r), b(b) {}

	// Creates a rectangle with the given center position and size.
	inline Rect(Vec2 pos, Size2 size)
		: l(pos.x - size.w / 2)
		, t(pos.y - size.h / 2)
		, r(pos.x + size.w / 2)
		, b(pos.y + size.h / 2)
	{
	}

	// Returns the width of the rect.
	constexpr int w() const { return r - l; }

	// Returns the height of the rect
	constexpr int h() const { return b - t; };

	// Returns the width and height of the rect.
	constexpr Size2 size() const { return {r - l, b - t}; }

	// Returns the x-position of the center of the rect.
	constexpr int cx() const { return (l + r) / 2; }

	// Returns the y-position of the center of the rect.
	constexpr int cy() const { return (t + b) / 2; }

	// Returns the top left corner position of the rect.
	constexpr Vec2 tl() const { return {l, t}; }

	// Returns the top right corner position of the rect.
	constexpr Vec2 tr() const { return {r, t}; }

	// Returns the bottom left corner position of the rect.
	constexpr Vec2 bl() const { return {l, b}; }

	// Returns the bottom right corner position of the rect.
	constexpr Vec2 br() const { return {r, b}; }

	// Returns the center position of the rect.
	constexpr Vec2 center() const { return { cx(), cy() }; }

	// Returns the top side center position of the rect.
	constexpr Vec2 centerT() const { return {cx(), t}; }

	// Returns the bottom side center position of the rect.
	constexpr Vec2 centerB() const { return {cx(), b}; }

	// Returns the left side center position of the rect.
	constexpr Vec2 centerL() const { return {l, cy()}; }

	// Returns the right side center position of the rect.
	constexpr Vec2 centerR() const { return {r, cy()}; }

	// Returns a vertical slice of the left side of the rect of width <w>.
	constexpr Rect sliceL(int w) const { return {l, t, l + w, b}; }

	// Returns a vertical slice of the center of the rect of width <w>.
	constexpr Rect sliceV(int w) const { int x = cx() - w/2; return {x, t, x + w, b}; }

	// Returns a vertical slice of the right side of the rect of width <w>.
	constexpr Rect sliceR(int w) const { return {r - w, t, r, b}; }

	// Returns a horizontal slice of the top side of the rect of height <h>.
	constexpr Rect sliceT(int h) const { return {l, t, r, t + h}; }

	// Returns a horizontal slice of the middle of the rect of height <h>.
	constexpr Rect sliceH(int h) const { int y = cy() - h/2; return {l, y, r, y + h}; }

	// Returns a horizontal slice of the bottom side of the rect of height <h>.
	constexpr Rect sliceB(int h) const { return {l, b - h, r, b}; }

	// Returns a region from the top left corner of the rect with width <w> and height <h>.
	constexpr Rect cornerTL(int w, int h) const { return {l, t, l + w, t + h}; }

	// Returns a region from the top right corner of the rect with width <w> and height <h>.
	constexpr Rect cornerTR(int w, int h) const { return {r - w, t, r, t + h}; }

	// Returns a region from the bottom left corner of the rect with width <w> and height <h>
	constexpr Rect cornerBL(int w, int h) const { return {l, b - h, l + w, b}; }

	// Returns a region from the bottom right corner of the rect with width <w> and height <h>.
	constexpr Rect cornerBR(int w, int h) const { return {r - w, b - h, r, b}; }

	// Returns a copy of the rect with all sides reduced b margin <m>.
	constexpr Rect shrink(int m) const { return {l + m, t + m, r - m, b - m}; }

	// Returns a copy of the rect with all horizontal sides reduced b margin <mx> and vertical sides b <my>.
	constexpr Rect shrink(int mx, int my) const { return {l + mx, t + my, r - mx, b - my}; }

	// Returns a copy of the rect with each side reduced b its corresponding margin (left, top, bottom right).
	constexpr Rect shrink(int ml, int mt, int mr, int mb) const { return {l + ml, t + mt, r - mr, b - mb}; }

	// Returns a copy of the rect with all horizontal sides reduced b margin <mx>.
	constexpr Rect shrinkX(int mx) const { return {l + mx, t, r - mx, b}; }

	// Returns a copy of the rect with each horizontal side reduced b the corresponding margin (left, right).
	constexpr Rect shrinkX(int ml, int mr) const { return {l + ml, t, r - mr, b}; }

	// Returns a copy of the rect with all vertical sides reduced b margin <my>.
	constexpr Rect shrinkY(int my) const { return {l, t + my, r, b - my}; }

	// Returns a copy of the rect with each vertical side reduced b the corresponding margin (top, bottom).
	constexpr Rect shrinkY(int mt, int mb) const { return {l, t + mt, r, b - mb}; }

	// Returns a copy of the rect with the left side reduced b margin <mx>.
	constexpr Rect shrinkL(int mx) const { return {l + mx, t, r, b}; }

	// Returns a copy of the rect with the right side reduced b margin <mx>.
	constexpr Rect shrinkR(int mx) const { return {l, t, r - mx, b}; }

	// Returns a copy of the rect with the left side reduced b margin <mx>.
	constexpr Rect shrinkT(int my) const { return {l, t + my, r, b}; }

	// Returns a copy of the rect with the right side reduced b margin <mx>.
	constexpr Rect shrinkB(int my) const { return {l, t, r, b - my}; }

	// Returns a copy of the rect with all sides expanded b margin <m>.
	constexpr Rect expand(int m) const { return {l - m, t - m, r + m, b + m}; }

	// Returns a copy of the rect with all horizontal sides expanded b margin <mx> and vertical sides b <my>.
	constexpr Rect expand(int mx, int my) const { return {l - mx, t - my, r + mx, b + my}; }

	// Returns a copy of the rect with each side expanded b a margin (left, top, right, bottom).
	constexpr Rect expand(int ml, int mt, int mr, int mb) const { return {l - ml, t - mt, r + mr, b + mb}; }

	// Returns a copy of the rect with the given left x-position and width.
	constexpr Rect withXandWidth(int x, int width) const  { return {x, t, x + width, b}; }

	// Returns a copy of the rect with the given top y-position and height.
	constexpr Rect withYandHeight(int y, int height) const { return {l, y, r, y + height}; }

	// Returns a copy of the rect that is offset b the given x-delta.
	constexpr Rect offsetX(int dx) const { return {l + dx, t, r + dx, b}; }

	// Returns a copy of the rect that is offset b the given y-delta.
	constexpr Rect offsetY(int dy) const { return {l, t + dy, r, b + dy}; }

	// Returns a copy of the rect that is offset b the given x-delta and y-delta.
	constexpr Rect offset(int dx, int dy) const { return {l + dx, t + dy, r + dx, b + dy}; }

	// Returns true if the rect contains point (p.x, p.y), false otherwise.
	constexpr bool contains(const Vec2& p) const { return p.x >= l && p.y >= t && p.x < r && p.y < b; }

	// Returns true if the rect contains point (px, py), false otherwise.
	constexpr bool contains(int x, int y) const { return x >= l && y >= t && x < r && y < b; }

	// Returns true if the rect intersects the given rectangle.
	constexpr bool overlaps(const Rect& o) const { return l < o.r && r > o.l && t < o.b && b > o.t; }

	// Returns a copy of the rect translated b the given offset vector.
	constexpr Rect operator + (const Vec2& v) const { return {l + v.x, t + v.y, r + v.x, b + v.y}; }
};

} // namespace AV