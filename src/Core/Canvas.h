#pragma once

#include <Core/Texture.h>

namespace Vortex {

class Canvas {
   public:
    ~Canvas();

    /// colorfblending modes for canvas drawing operations.
    enum BlendMode {
        BM_NONE,   ///< No blending.
        BM_ALPHA,  ///< Alpha blending.
        BM_ADD,    ///< Additive blending.
    };

    /// Constructs an empty canvas
    Canvas();

    /// Constructs a canvas with the given width and height in pixels.
    Canvas(int width, int height, float lum = 0.0f);

    /// Copies the contents of another canvas; this is a deep copy.
    Canvas(const Canvas& other);

    /// Sets a mask that discards modifications to pixels outside the given
    /// area.
    void setMask(int l, int t, int r, int b);

    /// Sets the line thickness that is used for drawing outline shapes.
    void setOutline(float size);

    /// Setting a value greater than zero will draw shapes with an outer glow.
    void setOuterGlow(float size);

    /// Setting a value greater than zero will draw outline shapes with an inner
    /// glow.
    void setInnerGlow(float size);

    /// Enables or disables fill; when fill is disabled, shapes are drawn as
    /// outlines.
    void setFill(bool enabled);

    /// Set the color blending mode that is used in drawing operations.
    void setBlendMode(BlendMode bm);

    /// Sets the drawing color to luminance l.
    void setColor(float lum, float alpha = 1.0f);

    /// Sets the drawing color to c.
    void setColor(const colorf& color);

    /// Sets the drawing color to a vertical gradient that interpolates from top
    /// to bottom.
    void setColor(const colorf& top, const colorf& bottom);

    /// Sets the drawing color to a gradient that interpolates between four
    /// corners. The colors correspond to the top-left, top-right, bottom-left
    /// and bottom-right corners.
    void setColor(const colorf& tl, const colorf& tr, const colorf& bl,
                  const colorf& br);

    /// Fills the entire canvas with the given RGBA colorfvalues.
    void clear(float lum = 0);

    /// Draws a line from point (x1, y1) to point (x2, y2) with the given
    /// thickness.
    void line(float x1, float y1, float x2, float y2, float thickness);

    /// Draws a circle at point (x, y) with the given radius.
    void circle(float x, float y, float radius);

    /// Draws a rectangle with (x1, y1) as top-left and (x2, y2) as bottom-right
    /// coordinate. Setting radius to a value greater than zero will round the
    /// corners of the rectangle.
    void box(float x1, float y1, float x2, float y2, float radius = 0.f);
    void box(int x1, int y1, int x2, int y2, float radius = 0.f);

    /// Draws a polygon with the given vertex positions.
    void polygon(const float* x, const float* y, int vertexCount);

    /// Creates a texture from the current canvas image.
    Texture createTexture(bool mipmap = false) const;

    /// Copies the contents of another canvas; this is a deep copy.
    Canvas& operator=(const Canvas& other);

    /// Returns a pointer to the pixel array, 4 values per pixel, RGBA order.
    float* pixels() { return canvas_data_; }

    int width() const {
        return canvas_width_;
    }  ///< Returns the number of horizontal pixels.
    int height() const {
        return canvas_height_;
    }  ///< Returns the number of vertical pixels.

   private:
    struct Data;
    float* canvas_data_;
    int canvas_width_, canvas_height_;
    Data* data_;  // TODO: replace with a more descriptive variable name.
};

};  // namespace Vortex
