#pragma once

#include <Core/Common/NonCopyable.h>

#include <Core/Graphics/Colorf.h>
#include <Core/Graphics/Texture.h>

namespace AV {

// Provides a pixel canvas that can be converted to a texture.
class Canvas : NonCopyable
{
public:
	~Canvas();

	// color blending modes for canvas drawing operations.
	enum class BlendMode
	{
		None,  // No blending.
		Alpha, // Alpha blending.
		Add,   // Additive blending.
	};

	// Constructs an empty canvas
	Canvas();

	// Constructs a canvas of <width> by <height> pixels and fills every pixel with
	// a grayscale value based on luminance <lum>. Default is pure black.
	Canvas(int width, int height, float lum = 0.0f);

	// Sets a mask that discards modifications to pixels outside the given area.
	void setMask(int left, int top, int right, int bottom);

	// Sets the line thickness that is used for drawing outline shapes.
	void setOutline(float size);

	// Setting a value greater than zero will draw shapes with an outer glow.
	void setOuterGlow(float size);

	// Setting a value greater than zero will draw outline shapes with an inner glow.
	void setInnerGlow(float size);

	// Enables or disables fill; when fill is disabled, shapes are drawn as outlines.
	void setFill(bool enabled);

	// Set the color blending mode that is used in drawing operations.
	void setBlendMode(BlendMode bm);

	// Sets the drawing color to luminance <lum> and alpha <a>.
	void setColor(float lum, float a = 1.0f);

	// Sets the drawing color to <color>.
	void setColor(const Colorf& color);

	// Sets the drawing color to a vertical gradient that interpolates from <top> to <bottom>.
	void setColor(const Colorf& top, const Colorf& bottom);

	// Sets the drawing color to a gradient that interpolates between four corners.
	// The colors correspond to the top-left, top-right, bottom-left and bottom-right corners.
	void setColor(const Colorf& tl, const Colorf& tr, const Colorf& bl, const Colorf& br);

	// Fills the entire canvas with the given luminance value.
	void clear(float lum = 0);

	// Draws a line from point <x1, y1> to point <x2, y2> with the given thickness.
	void line(float x1, float y1, float x2, float y2, float thickness);

	// Draws a circle at point <x, y> with the given radius.
	void circle(float x, float y, float radius);

	// Draws a rectangle with <x1, y1> as top-left and <x2, y2> as bottom-right coordinate.
	// Setting radius to a value greater than zero will round the corners of the rectangle.
	void box(float x1, float y1, float x2, float y2, float radius = 0.f);

	// Draws a rectangle with <x1, y1> as top-left and <x2, y2> as bottom-right coordinate.
	// Setting radius to a value greater than zero will round the corners of the rectangle.
	void box(int x1, int y1, int x2, int y2, float radius = 0.f);

	// Draws a polygon with the given vertex positions.
	void polygon(const float* x, const float* y, int vertexCount);

	// Creates a texture from the current canvas image.
	unique_ptr<Texture> createTexture(const char* name, bool mipmap = false) const;

	// Returns a pointer to the pixel array, 4 values per pixel, RGBA order.
	float* pixels() { return myPixels; }

	// Returns the number of horizontal pixels.
	int width() const { return myWidth; }

	// Returns the number of vertical pixels.
	int height() const { return myHeight; }

private:
	struct Settings;
	float* myPixels;
	int myWidth;
	int myHeight;
	Settings* mySettings;
};

} // namespace AV