#pragma once

#include <Core/Common/NonCopyable.h>

#include <Core/Graphics/Color.h>
#include <Core/Graphics/Image.h>
#include <Core/Graphics/Texture.h>

namespace AV {

namespace Vk { struct Image; }

// Enumeration of supported filtering modes.
enum class TextureFilter
{
	Nearest,             // Nearest pixel.
	Linear,              // Linear interpolation between pixels.
	LinearMipmapNearest, // Linear interpolation between pixels, nearest mipmap level.
	LinearMipmapLinear,  // Linear interpolation between pixels and mipmap levels.

	Count
};

// Enumeration of supported pixel formats.
enum class TextureFormat
{
	RGBA,  // 4 channels : red, green, blue, alpha.
	LumA,  // 2 channels : luminance, alpha.
	Lum,   // 1 channel  : luminance; alpha is treated 255.
	Alpha, // 1 channel  : alpha; luminance is treated as 255.

	Count
};

// Instance of a texture, contains the actual texture data.
class Texture : NonCopyable
{
public:
	~Texture();

	// Creates a blank texture.
	Texture();

	// Creates a texture with the given data.
	Texture(Texture&& texture) noexcept;

	// Loads an image from file and if necessary, converts it to the target format.
	Texture(
		const FilePath& path,
		TextureFormat format = TextureFormat::RGBA,
		Color transparentFill = Color::Blank);

	// Creates a blank texture of size <width, height> and the given format.
	Texture(
		int width,
		int height,
		TextureFormat format = TextureFormat::RGBA);

	// Creates a texture of size <width, height> from a pixel buffer.
	// The number of color values read per pixel is based on the pixel format.
	Texture(
		const uchar* pixels,
		int width,
		int height,
		PixelFormat pixelFormat,
		TextureFormat format = TextureFormat::RGBA);

	// Creates a texture of size <width, height> from a pixel buffer of size <stride, height>.
	// The number of color values read per pixel is based on the pixel format.
	// For each row in the buffer, only the pixels up to <width> are copied to the texture.
	Texture(
		const uchar* pixels,
		int width,
		int height,
		int stride,
		PixelFormat pixelFormat,
		TextureFormat format = TextureFormat::RGBA);

	// Creates multiple textures from a single image file that contains a grid of tiles.
	static int createTiles(
		const FilePath& path,
		int tileWidth,
		int tileHeight,
		int numTiles,
		Texture* outTiles,
		TextureFormat format = TextureFormat::RGBA);

	// Returns the pixel format that matches the given texture format.
	static PixelFormat getPixelFormat(TextureFormat format);

	// Updates a region of the texture from a buffer of pixels.
	// The number of color values read per pixel is based on the pixel format.
	void modify(const uchar* pixels, int x, int y, int width, int height, int stride, PixelFormat pixelFormat);

	// Reads pixel data from the texture to the output buffer.
	// The number of color values read per pixel is based on the pixel format.
	void readPixels(uchar* buffer, int bufferSize) const;

	// Sets UV wrapping mode to repeat on true, or clamp on false; default is clamp.
	void setWrapping(bool repeat);

	// Adjusts the height of the texture. Existing pixel data is kept intact.
	void adjustHeight(int height);

	// Returns the width of the texture in pixels.
	inline int width() const { return myWidth; }

	// Returns the height of the texture in pixels.
	inline int height() const { return myHeight; }

	// Returns the format of the texture.
	inline TextureFormat format() const { return myFormat; }

	// Returns whether the texture has a valid image.
	inline bool valid() const {	return (bool)myImage; }

	// Returns the underlying Vulkan image.
	inline Vk::Image* image() const { return myImage.get();	}

	// Takes ownership of the data from given texture.
	void operator = (Texture&&) noexcept;

private:
	unique_ptr<Vk::Image> myImage;
	int myWidth;
	int myHeight;
	TextureFormat myFormat;
};

} // namespace AV