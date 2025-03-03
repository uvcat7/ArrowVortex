#pragma once

#include <Core/Texture.h>
#include <Core/Renderer.h>

namespace Vortex {

// ================================================================================================
// Color utilities.

#define COLOR32_RMASK 0xFF
#define COLOR32_GMASK 0xFF00
#define COLOR32_BMASK 0xFF0000
#define COLOR32_AMASK 0xFF000000

#define COLOR32_RSHIFT 0
#define COLOR32_GSHIFT 8
#define COLOR32_BSHIFT 16
#define COLOR32_ASHIFT 24

// Macro to create a color32 from 8-bit RGBA values.
#define COLOR32(r, g, b, a)\
(color32)(((a)<<COLOR32_ASHIFT)|((b)<<COLOR32_BSHIFT)|((g)<<COLOR32_GSHIFT)|((r)<<COLOR32_RSHIFT))

inline color32 Color32(int r, int g, int b, int a = 255)
{
	return COLOR32(r, g, b, a);
}

inline color32 Color32(int lum, int a = 255)
{
	return COLOR32(lum, lum, lum, a);
}

inline color32 Color32a(color32 rgb, int a)
{
	return (rgb & ~COLOR32_AMASK) | (a << COLOR32_ASHIFT);
}

// Common colors (float RGBA).
struct Colorsf { static const colorf white, black, blank; };

// Common colors (8-bit RGBA).
struct Colors { static const color32 white, black, blank; };

// ================================================================================================
// Drawing objects.

// A 3-part texture with a middle section that stretches horizontally.
struct TileBar
{
	enum Flags { VERTICAL, FLIP_H, FLIP_V };

	TileBar();

	void draw(recti rect,
		color32 color = Colors::white, int flags = 0) const;

	void draw(QuadBatchTC* out, recti rect,
		color32 color = Colors::white, int flags = 0) const;

	Texture texture;
	areaf uvs;
	int border;
};

// A 9-part texture with a middle section that stretches horizontally and vertically.
struct TileRect
{
	enum Flags { VERTICAL, FLIP_H, FLIP_V };

	TileRect();

	void draw(recti rect,
		color32 color = Colors::white, int flags = 0) const;

	void draw(QuadBatchTC* out, recti rect,
		color32 color = Colors::white, int flags = 0) const;

	Texture texture;
	areaf uvs;
	int border;
};

// A horizontal texture of two tile rects, 1st with round corners, 2nd with square corners.
struct TileRect2
{
	enum Flags { VERTICAL, FLIP_H, FLIP_V };

	enum Rounding
	{
		TL = 0x1,
		TR = 0x2,
		BL = 0x4,
		BR = 0x8,

		L = TL | BL,
		T = TL | TR,
		R = TR | BR,
		B = BL | BR,

		ALL = TL | BL | TR | BR,
	};

	void draw(recti rect,
		int rounding = ALL, color32 color = Colors::white, int flags = 0) const;

	void draw(QuadBatchTC* out, recti rect,
		int rounding = ALL, color32 color = Colors::white, int flags = 0) const;

	Texture texture;
	int border;
};

// Functions and enumerations related to 2D drawing.
namespace Draw
{
	/// Enumeration of flags that affect drawing.
	enum Flags
	{
		FLIP_H = 1 << 0, ///< Flip the image horizontally.
		FLIP_V = 1 << 1, ///< Flip the image vertically.
		ROT_90 = 1 << 2, ///< Rotate the image 90 degrees clockwise.
	};

	/// Draws a filled rectangle.
	void fill(recti r, color32 c);
	void fill(QuadBatchC* batch, recti r, color32 c);
	void fill(recti r, color32 tl, color32 tr, color32 bl, color32 br, bool hsv);
	void fill(recti r, color32 c, TextureHandle t, Texture::Format fmt = Texture::RGBA);
	void fill(recti r, color32 c, TextureHandle t, areaf uvs, Texture::Format fmt = Texture::RGBA);

	/// Draws a rectangle outline.
	void outline(recti r, color32 c);

	/// Draws a rectangle with round corners of radius 4.
	void roundedBox(recti r, color32 c);

	/// Draws a sprite.
	void sprite(const Texture& t, vec2i pos, int flags = 0);
	void sprite(const Texture& t, vec2i pos, color32 c, int flags = 0);
};

}; // namespace Vortex
