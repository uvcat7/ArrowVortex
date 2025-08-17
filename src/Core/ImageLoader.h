#pragma once

#include <Core/Texture.h>

namespace Vortex {

/// Functions and enumerations related to image loading.
namespace ImageLoader {
enum Format { RGBA, RGB, LUMA, LUM, ALPHA };
struct Data {
    uint8_t* pixels;
    int width, height;
};
Data load(const char* path, Format targetFormat);
void release(Data& data);
};  // namespace ImageLoader

/// Functions related to zlib decompression.
namespace Zlib {
struct Data {
    uint8_t* data;
    int numBytes;
};
Data deflate(const uint8_t* compressedData, int numBytes);
void release(Data& data);
};  // namespace Zlib

};  // namespace Vortex
