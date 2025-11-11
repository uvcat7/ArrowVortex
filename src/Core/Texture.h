#pragma once

#include <Core/Core.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Vortex {

/// Reference counted texture object; keeps track of a texture handle.
class Texture {
   public:
    struct Data;

    ~Texture();
    Texture();
    Texture(Texture&& s);
    Texture(const Texture& s);
    Texture& operator=(Texture&& s);
    Texture& operator=(const Texture& s);

    /// Enumeration of supported pixel formats.
    enum Format {
        RGBA = 0,   ///< 4 channels : red, green, blue, alpha.
        LUMA = 1,   ///< 2 channels : luminance, alpha.
        LUM = 2,    ///< 1 channel  : luminance; alpha is always 1.
        ALPHA = 3,  ///< 1 channel  : alpha; luminance is always 1.
    };

    /// Creates a blank texture with the given width and height.
    Texture(int w, int h, Format fmt = RGBA);

    /// Loads a texture from an image file.
    Texture(fs::path path, bool mipmap = false, Format fmt = RGBA);

    /// Creates a texture from a buffer of [w * h * channels] pixel values.
    Texture(int w, int h, const uint8_t* pixeldata, bool mipmap = false,
            Format fmt = RGBA);

    /// Updates a region of the texture with a buffer of [w * h * channels]
    /// pixel values.
    void modify(int x, int y, int w, int h, const uint8_t* pixeldata);

    /// Sets UV wrapping mode to repeat on true, or clamp on false; default is
    /// clamp.
    void setWrapping(bool repeat);

    /// Sets the filtering mode to linear on true, or nearest on false; default
    /// is linear.
    void setFiltering(bool linear);

    /// Makes sure the texture stays loaded until the shutdown of goo.
    void cache() const;

    /// Returns the handle of the OpenGL texture object.
    TextureHandle handle() const;

    /// Returns the width and height of the texture in pixels.
    vec2i size() const;

    /// Returns the width of the texture in pixels.
    int width() const;

    /// Returns the height of the texture in pixels.
    int height() const;

    /// Returns the pixel format of the texture.
    Format format() const;

    /// Returns the texture data; used internally.
    inline void* data() const { return data_; }

    /// Logs debug info on the currently loaded textures.
    static void LogInfo();

    /// Creates a set of textures from a sprite sheet / tile sheet.
    static int createTiles(fs::path path, int tileW, int tileH, int numTiles,
                           Texture* outTiles, bool mipmap = false,
                           Format fmt = RGBA);

    /// Creates a set of textures from a sprite sheet / tile sheet.
    static int createTiles(fs::path path, int tileW, int tileH, int numTiles,
                           std::vector<Texture>& outTiles, bool mipmap = false,
                           Format fmt = RGBA);

   private:
    Data* data_;  // TODO: replace with a more descriptive variable name.
};

};  // namespace Vortex
