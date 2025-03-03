#pragma once

#include <Core/Types/Vec2.h>
#include <Core/Types/Rect.h>
#include <Core/Types/Rectf.h>

#include <Core/Utils/Enum.h>

#include <Core/Graphics/Color.h>
#include <Core/Graphics/Texture.h>
#include <Core/Graphics/Renderer.h>

namespace AV {

// =====================================================================================================================
// Drawable objects.

// A drawable sprite.
struct Sprite
{
	// Enumeration of drawing orientations.
	enum class Orientation
	{
		Normal,
		FlipH,
		FlipV,
		Rot90,
		Rot180,
		Rot270,
	};

	// Enumeration of fill styles.
	enum class Fill
	{
		Center,    // Do not scale and center inside the target rect.
		Stretch,   // Scale non-uniformly and fill the target rect.
		Letterbox, // Scale uniformly and fit inside the target rect.
		Crop,      // Scale uniformly and fill the entire target rect.
	};

	Sprite();
	Sprite(unique_ptr<Texture>&& texture);
	Sprite(const shared_ptr<Texture>& texture);

	void draw(Vec2 pos, Color col = Color::White, Orientation orientation = Orientation::Normal) const;
	void draw(Rect rect, Color col = Color::White, Orientation orientation = Orientation::Normal, Fill fill = Fill::Center) const;

	shared_ptr<Texture> texture; // The texture containing the sprite.
	Rectf uvs; // The texture region corresponding to the sprite.
};

// A three-part texture with a middle section that stretches horizontally.
struct TileBar
{
	// Enumeration of flags that can be used in draw operations.
	enum class Flags
	{
		None     = 0,
		Vertical = 1 << 0,
		FlipH    = 1 << 1,
		FlipV    = 1 << 2,
	};

	TileBar();

	void draw(Rect rect, Color color = Color::White, Flags flags = Flags::None) const;
	void drawBatched(Rect rect, Color color = Color::White, Flags flags = Flags::None) const;

	shared_ptr<Texture> texture; // The texture containing the tile bar image.
	Rectf uvs; // The texture region of the tile bar image.
	int border; // Determines how many pixels wide the edges are.
};

template <>
constexpr bool IsFlags<TileBar::Flags> = true;

// A nine-part texture with a middle section that stretches horizontally and vertically.
struct TileRect
{
	// Enumeration of flags that can be used in draw operations.
	enum class Flags
	{
		None     = 0,
		Vertical = 1 << 0,
		FlipH    = 1 << 1,
		FlipV    = 1 << 2,
	};

	TileRect();

	// Draws the tile rect directly.
	void draw(Rect rect, Color color = Color::White, Flags flags = Flags::None) const;
	
	// Outputs the tile rect to the given quad batch.
	void drawBatched(Rect rect, Color color = Color::White, Flags flags = Flags::None) const;

	shared_ptr<Texture> texture; // The texture containing the tile rect image.
	Rectf uvs; // The texture region of the tile rect image.
	int border; //  Determines how many pixels wide the edges are.
};

template <>
constexpr bool IsFlags<TileRect::Flags> = true;

// A combination of 2 tile rects arranged horizontally (round corners, square corners).
struct TileRect2
{
	// Enumeration of flags that can be used in draw operations.
	enum class Flags
	{
		None     = 0,
		Vertical = 1 << 0,
		FlipH    = 1 << 1,
		FlipV    = 1 << 2,
	};

	// Enumeration of flags that determine which corners are rounded.
	enum class Rounding
	{
		TL = 1 << 0,
		TR = 1 << 1,
		BL = 1 << 2,
		BR = 1 << 3,

		L = TL | BL,
		T = TL | TR,
		R = TR | BR,
		B = BL | BR,

		All = TL | BL | TR | BR,
	};

	// Draws the tile rect directly.
	void draw(Rect rect, Rounding rounding = Rounding::All, Color color = Color::White, Flags flags = Flags::None) const;

	// Outputs the tile rect to the given quad batch.
	void drawBatched(Rect rect, Rounding rounding = Rounding::All, Color color = Color::White, Flags flags = Flags::None) const;

	shared_ptr<Texture> texture; // The texture containing the 2 tile rect images.
	int border;	// Determines how many pixels wide the edges are.
};

template <>
constexpr bool IsFlags<TileRect2::Flags> = true;

template <>
constexpr bool IsFlags<TileRect2::Rounding> = true;

// A drawable sprite with UVs.
class SpriteEx
{
public:
	// Initializes multiple sprites at the same startTime from a spritesheet.
	static void fromTileset(SpriteEx* sprites, int num, int cols, Row rows, int spriteW, int spriteH);

	SpriteEx();
	SpriteEx(float w, float h);

	void setUvs(Rectf uvs);
	void setUvs(const float uvs[8]);

	void drawBatched(float scale, float x, float y) const;
	void drawBatched(float scale, float x, float yt, float yb) const;
	void drawBatched(float scale, float x, float y, uchar alpha) const;
	void drawBatched(float scale, float x, float y, Color color) const;

	inline float dx() const { return myDx; }
	inline float dy() const { return myDy; }
	inline const float* uvs() const { return myUvs; }

private:
	float myDx;
	float myDy;
	float myUvs[8];
};

// Functions and enumerations related to 2D drawing.
namespace Draw {

// Draws a filled rectangle.
void fill(Rect rect, Color color);
void fill(Rectf rect, Color color);
void fill(Rect rect, Color tl, Color tr, Color bl, Color br, bool hsv);
void fill(Rect rect, Color color, const Texture& texture);
void fill(Rect rect, Color color, const Texture& texture, Rectf uvs);
void fill(Rect rect, Color color, const Texture& texture, const float uvs[8]);

// Draws a rectangle outline.
void outline(Rect rect, Color color);

// Draws a rectangle with round corners of radius 4.
void roundedBox(Rect rect, Color color);

} // namespace Draw
} // namespace AV
