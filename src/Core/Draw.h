#pragma once

#include <Core/Texture.h>
#include <Core/Renderer.h>

namespace Vortex {

// ================================================================================================
// Color utilities.

constexpr uint32_t kColor32_RedMask = 0xFF;
constexpr uint32_t kColor32_GreenMask = 0xFF00;
constexpr uint32_t kColor32_BlueMask = 0xFF0000;
constexpr uint32_t kColor32_AlphaMask = 0xFF000000;

constexpr int kColor32_RedShift = 0;
constexpr int kColor32_GreenShift = 8;
constexpr int kColor32_BlueShift = 16;
constexpr int kColor32_AlphaShift = 24;

// Inline function to create a uint32_t from 8-bit RGBA values.
inline uint32_t RGBAtoColor32(int r, int g, int b, int a) {
    return static_cast<uint32_t>(
        ((a << kColor32_AlphaShift) & kColor32_AlphaMask) |
        ((b << kColor32_BlueShift) & kColor32_BlueMask) |
        ((g << kColor32_GreenShift) & kColor32_GreenMask) |
        ((r << kColor32_RedShift) & kColor32_RedMask));
}

inline uint32_t Color32(int r, int g, int b, int a = 255) {
    return RGBAtoColor32(r, g, b, a);
}

inline uint32_t Color32(int lum, int a = 255) {
    return RGBAtoColor32(lum, lum, lum, a);
}

inline uint32_t Color32a(uint32_t rgb, int a) {
    return (rgb & ~kColor32_AlphaMask) |
           ((a << kColor32_AlphaShift) & kColor32_AlphaMask);
}

// Common colors (float RGBA).
struct Colorsf {
    static const colorf white, black, blank;
};

// Common colors (8-bit RGBA).
struct Colors {
    static const uint32_t white, black, blank;
};

// ================================================================================================
// Drawing objects.

// A 3-part texture with a middle section that stretches horizontally.
struct TileBar {
    enum Flags { VERTICAL, FLIP_H, FLIP_V };

    TileBar();

    void draw(recti rect, uint32_t color = Colors::white, int flags = 0) const;

    void draw(QuadBatchTC* out, recti rect, uint32_t color = Colors::white,
              int flags = 0) const;

    Texture texture;
    areaf uvs;
    int border;
};

// A 9-part texture with a middle section that stretches horizontally and
// vertically.
struct TileRect {
    enum Flags { VERTICAL, FLIP_H, FLIP_V };

    TileRect();

    void draw(recti rect, uint32_t color = Colors::white, int flags = 0) const;

    void draw(QuadBatchTC* out, recti rect, uint32_t color = Colors::white,
              int flags = 0) const;

    Texture texture;
    areaf uvs;
    int border;
};

// A horizontal texture of two tile rects, 1st with round corners, 2nd with
// square corners.
struct TileRect2 {
    enum Flags { VERTICAL, FLIP_H, FLIP_V };

    enum Rounding {
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

    void draw(recti rect, int rounding = ALL, uint32_t color = Colors::white,
              int flags = 0) const;

    void draw(QuadBatchTC* out, recti rect, int rounding = ALL,
              uint32_t color = Colors::white, int flags = 0) const;

    Texture texture;
    int border;
};

// Functions and enumerations related to 2D drawing.
namespace Draw {
/// Enumeration of flags that affect drawing.
enum Flags {
    FLIP_H = 1 << 0,  ///< Flip the image horizontally.
    FLIP_V = 1 << 1,  ///< Flip the image vertically.
    ROT_90 = 1 << 2,  ///< Rotate the image 90 degrees clockwise.
};

/// Draws a filled rectangle.
void fill(recti r, uint32_t c);
void fill(QuadBatchC* batch, recti r, uint32_t c);
void fill(recti r, uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br,
          bool hsv);
void fill(recti r, uint32_t c, TextureHandle t,
          Texture::Format fmt = Texture::RGBA);
void fill(recti r, uint32_t c, TextureHandle t, areaf uvs,
          Texture::Format fmt = Texture::RGBA);

/// Draws a rectangle outline.
void outline(recti r, uint32_t c);

/// Draws a rectangle with round corners of radius 4.
void roundedBox(recti r, uint32_t c);

/// Draws a sprite.
void sprite(const Texture& t, vec2i pos, int flags = 0);
void sprite(const Texture& t, vec2i pos, uint32_t c, int flags = 0);
};  // namespace Draw

};  // namespace Vortex
