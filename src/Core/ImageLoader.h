#pragma once

#include <Core/Texture.h>

namespace Vortex {

/// Functions and enumerations related to image loading.
namespace ImageLoader
{
	enum Format { RGBA, RGB, LUMA, LUM, ALPHA };
	struct Data { uchar* pixels; int width, height; };
	Data load(const char* path, Format targetFormat);
	void release(Data& data);
};

/// Functions related to zlib decompression.
namespace Zlib
{
	struct Data { uchar* data; int numBytes; };
	Data deflate(const uchar* compressedData, int numBytes);
	void release(Data& data);
};

}; // namespace Vortex
