#pragma once

#include <Core/System/File.h>

namespace AV {

// Pixel formats used for loading/saving images.
enum class PixelFormat
{
	RGBA,  // 4 channels : red, green, blue, alpha.
	RGB,   // 3 channels : red, green, blue; alpha is treated as 255.
	LumA,  // 2 channels : luminance, alpha.
	Lum,   // 1 channel  : luminance; alpha is treated 255.
	Alpha, // 1 channel  : alpha; luminance is treated as 255.

	Count
};

// Functions related to image processing.
namespace Image
{
	// Contains the output data of an image operation.
	struct Data { uchar* pixels; int width, height; };
	
	// Returns the number of values each pixel contains for the given pixel format.
	int numChannels(PixelFormat format);
	
	// Loads an image from file and converts it to the requested output format.
	Data load(const FilePath& path, PixelFormat outputFormat);
	
	// Converts an image from the given input format to the requested output format.
	Data convert(const uchar* pixels, int width, int height, int stride, PixelFormat inputFormat, PixelFormat outputFormat);
	
	// Resizes an image to the requested target width and height.
	Data resize(const uchar* pixels, int width, int height, int stride, PixelFormat format, int targetWidth, int targetHeight);
	
	// Frees the memory allocated for the pixels of the output data from one of the image operations.
	void release(uchar* pixels);
};
	
// Functions related to zlib decompression.
namespace Zlib
{
	// Decompresses a block of zlib compressed memory.
	bool deflate(const void* compressedData, size_t compressedSize, void* uncompressedData, size_t uncompressedSize);
};

} // namespace AV
