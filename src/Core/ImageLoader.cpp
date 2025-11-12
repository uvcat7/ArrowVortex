#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <Core/ImageLoader.h>

#include <System/File.h>
#include <System/Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>  // ptrdiff_t on osx
#include <fstream>

using namespace Vortex;

namespace Vortex {

// ================================================================================================
// ImageLoader :: API functions.

ImageLoader::Data ImageLoader::load(fs::path path, ImageLoader::Format fmt) {
    int channels = 0, w = 0, h = 0;
    static const int sFmtChannels[] = {4, 3, 2, 1, 1};
    auto desiredChannels = sFmtChannels[fmt];
    ImageLoader::Data out = {nullptr, 0, 0};
    auto path_str = pathToUtf8(path);
    stbi_uc *pixels = stbi_load(path_str.c_str(), &w, &h, &channels, 0);
    if (pixels && w > 0 && h > 0) {
        // stb is missing the ability to convert to alpha-only format from 2 and
        // 4 channels, so we do it ourselves here.
        if (fmt == ImageLoader::ALPHA && (channels == 2 || channels == 4)) {
            out.pixels =
                reinterpret_cast<uint8_t *>(malloc(w * h * desiredChannels));
            if (!out.pixels) {
                Debug::blockBegin(Debug::WARNING,
                                  "malloc failed on loading image");
                Debug::log("file: %s\n", path_str.c_str());
                Debug::blockEnd();
                stbi_image_free(pixels);
                return out;
            }
            for (auto j = 0; j < w * h; ++j) {
                unsigned char *src = pixels + j * channels;
                unsigned char *dest = out.pixels + j * desiredChannels;
                if (channels == 4) {
                    dest[0] = src[3];
                } else {
                    dest[0] = src[1];
                }
            }
        } else {
            out.pixels =
                stbi__convert_format(pixels, channels, desiredChannels, w, h);
        }
        out.width = w, out.height = h;
    } else {
        Debug::blockBegin(Debug::WARNING, "could not load image");
        Debug::log("file: %s\n", path_str.c_str());
        if (stbi__g_failure_reason) {
            Debug::log("reason: %s\n", stbi__g_failure_reason);
        }
        Debug::blockEnd();
    }
    return out;
}

void ImageLoader::release(ImageLoader::Data &data) {
    stbi_image_free(data.pixels);
}

Zlib::Data Zlib::deflate(const stbi_uc *data, int inSize) {
    int numBytes = 0;
    Zlib::Data out = {nullptr, 0};
    out.data = reinterpret_cast<stbi_uc *>(stbi_zlib_decode_malloc(
        reinterpret_cast<const char *>(data), inSize, &numBytes));
    if (out.data) out.numBytes = numBytes;
    return out;
}

void Zlib::release(Zlib::Data &data) { free(data.data); }

};  // namespace Vortex
