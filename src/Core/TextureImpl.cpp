#include <Core/TextureImpl.h>
#include <Core/ImageLoader.h>

#include <Core/Vector.h>
#include <Core/StringUtils.h>
#include <Core/MapUtils.h>
#include <Core/Shader.h>
#include <Core/Utils.h>

#include <System/Debug.h>

#include <set>
#include <map>

#include <System/OpenGL.h>

namespace Vortex {
namespace {

ImageLoader::Format TexLoadFormats[] = {ImageLoader::RGBA, ImageLoader::LUMA,
                                        ImageLoader::LUM, ImageLoader::ALPHA};

struct TextureManagerInstance {
    typedef std::map<std::string, Texture::Data*> TexMap;
    typedef Vector<Texture::Data*> TexList;

    TexMap files;
    TexList textures;
};

static TextureManagerInstance* TM = nullptr;

};  // anonymous namespace

// ================================================================================================
// Texture manager

void TextureManager::create() { TM = new TextureManagerInstance; }

void TextureManager::destroy() {
    /// We can't delete textures that still have references, so we just
    /// invalidate them.
    for (auto* tex : TM->textures) {
        if (tex->refs == 0)
            delete tex;
        else
            tex->clear();
    }
    delete TM;
    TM = nullptr;
}

Texture::Data* TextureManager::load(const char* path, Texture::Format fmt,
                                    bool mipmap) {
    // If textures are created before goo is initialized, just return null.
    if (!TM) return nullptr;

    // Combine path/format/mipmap information to form the key String.
    std::string key(path);
    key = key + ("0" + fmt);
    key = key + (mipmap ? "y" : "n");

    // Check if the image is already loaded.
    auto it = TM->files.find(key);
    if (it != TM->files.end()) {
        auto* out = it->second;
        out->refs++;
        return out;
    }

    // If not, try to load the image.
    Texture::Data* out = nullptr;
    ImageLoader::Data img = ImageLoader::load(path, TexLoadFormats[fmt]);
    if (img.pixels) {
        out = new Texture::Data(img.width, img.height, fmt, img.pixels, mipmap);
        TM->textures.push_back(out);
        TM->files[key] = out;
        ImageLoader::release(img);
    }
    return out;
}

Texture::Data* TextureManager::load(int w, int h, Texture::Format fmt,
                                    bool mipmap, const uint8_t* pixels) {
    // If textures are created before goo is initialized, just return null.
    if (!TM) return nullptr;

    // Try to create the texture.
    Texture::Data* out = new Texture::Data(w, h, fmt, pixels, mipmap);
    TM->textures.push_back(out);

    return out;
}

void TextureManager::release(Texture::Data* tex) {
    tex->refs--;
    if (tex->refs == 0 && !tex->cached) {
        // Textures can still exist after shutdown, don't assume TM is valid.
        if (TM) {
            Map::eraseVals(TM->files, tex);
            TM->textures.erase_values(tex);
        }
        delete tex;
    }
}

// ================================================================================================
// API functions from "draw.h"

static int NextPowerOfTwo(int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static const uint8_t* ConvertAlphaToLuma(Vector<uint8_t>& out,
                                         const uint8_t* in, int width,
                                         int height) {
    int numPixels = width * height;
    out.resize(numPixels * 2);
    uint8_t* dst = out.begin();
    for (auto end = in + numPixels; in != end; ++in) {
        *dst++ = 255;
        *dst++ = *in;
    }
    return out.begin();
}

// Maps a channel count to an OpenGL texture type.
static const int sFmtGL[4] = {GL_RGBA, GL_LUMINANCE_ALPHA, GL_LUMINANCE,
                              GL_LUMINANCE};
static const int sIntFmtGL[4] = {GL_RGBA8, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE8,
                                 GL_LUMINANCE8};
static const int sNumChannels[4] = {4, 2, 1, 1};

// Downsamples the source image to half size.
static void GenerateMipmapLevel(const uint8_t* src, uint8_t* dst, int w, int h,
                                Texture::Format fmt) {
    int ch = sNumChannels[fmt];
    if (ch == 4) {
        for (int y = 0; y < h; y += 2)
            for (int x = 0; x < w; x += 2, dst += ch) {
                auto a = src + (y * w + x) * ch, b = a + ch, c = a + w * ch,
                     d = c + ch;
                int asum = a[3] + b[3] + c[3] + d[3];
                if (asum > 0) {
                    for (int i = 0; i < 3; ++i) {
                        dst[i] = (a[i] * a[3] + b[i] * b[3] + c[i] * c[3] +
                                  d[i] * d[3]) /
                                 asum;
                    }
                    dst[3] = asum / 4;
                } else {
                    dst[0] = dst[1] = dst[2] = dst[3] = 0;
                }
            }
    } else if (ch == 2) {
        for (int y = 0; y < h; y += 2)
            for (int x = 0; x < w; x += 2, dst += ch) {
                auto a = src + (y * w + x) * ch, b = a + ch, c = a + w * ch,
                     d = c + ch;
                int asum = a[1] + b[1] + c[1] + d[1];
                if (asum > 0) {
                    dst[0] = (a[0] * a[1] + b[0] * b[1] + c[0] * c[1] +
                              d[0] * d[1]) /
                             asum;
                    dst[1] = asum / 4;
                } else {
                    dst[0] = dst[1] = 0;
                }
            }
    } else {
        for (int y = 0; y < h; y += 2)
            for (int x = 0; x < w; x += 2, dst += ch) {
                auto a = src + (y * w + x) * ch, b = a + ch, c = a + w * ch,
                     d = c + ch;
                dst[0] = (a[0] + b[0] + c[0] + d[0]) / 4;
            }
    }
}

// Generates additional mipmap levels for the current openGL texture.
static void GenerateMipmaps(int w, int h, const uint8_t* pixeldata,
                            Texture::Format fmt) {
    int ch = sNumChannels[fmt];
    uint8_t* tmpA = static_cast<uint8_t*>(malloc((w / 2) * (h / 2) * ch));
    uint8_t* tmpB = static_cast<uint8_t*>(malloc((w / 4) * (h / 4) * ch));

    // The first mipmap level is generated from the source buffer.
    GenerateMipmapLevel(pixeldata, tmpA, w, h, fmt);
    w /= 2, h /= 2;
    glTexImage2D(GL_TEXTURE_2D, 1, sIntFmtGL[fmt], w, h, 0, sFmtGL[fmt],
                 GL_UNSIGNED_BYTE, tmpA);

    // Subsequent mipmap levels are generated by swapping temporary buffers.
    for (int n = 2; w > 0 && h > 0; ++n) {
        GenerateMipmapLevel(tmpA, tmpB, w, h, fmt);
        w /= 2, h /= 2;
        glTexImage2D(GL_TEXTURE_2D, n, sIntFmtGL[fmt], w, h, 0, sFmtGL[fmt],
                     GL_UNSIGNED_BYTE, tmpB);
        std::swap(tmpA, tmpB);
    }
    free(tmpA);
    free(tmpB);
}

// Tries to load the texture as a power-of-two texture.
static bool PowerOfTwoTexImage2D(Texture::Format fmt, int w, int h,
                                 const uint8_t* pixels) {
    int channels = sNumChannels[fmt];
    int nw = NextPowerOfTwo(w);
    int nh = NextPowerOfTwo(h);
    if (nw == w && nh == h) return false;

    uint8_t *buffer = static_cast<uint8_t*>(malloc(nw * nh * channels)),
            *dst = buffer;
    int maxIndex = w * h - w - 1;
    int x, y;
    float sx, sy;
    float dx = w / static_cast<float>(nw);
    float dy = h / static_cast<float>(nh);
    for (y = 0, sy = 0; y < nh; ++y, sy += dy) {
        int py = static_cast<int>(sy);
        for (x = 0, sx = 0; x < nw; ++x, sx += dx, dst += channels) {
            int px = static_cast<int>(sx);

            const uint8_t* p1 =
                pixels + clamp(py * w + px, 0, maxIndex) * channels;
            const uint8_t* p2 = p1 + channels;
            const uint8_t* p3 = p1 + w * channels;
            const uint8_t* p4 = p3 + channels;

            for (int ch = 0; ch < channels; ++ch) {
                float fx1 = sx - px, fx0 = 1.0f - fx1;
                float fy1 = sy - py, fy0 = 1.0f - fy1;

                float w1 = fx0 * fy0;
                float w2 = fx1 * fy0;
                float w3 = fx0 * fy1;
                float w4 = fx1 * fy1;

                dst[ch] = static_cast<uint8_t>(p1[ch] * w1 + p2[ch] * w2 +
                                               p3[ch] * w3 + p4[ch] * w4);
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, sIntFmtGL[fmt], nw, nh, 0, sFmtGL[fmt],
                 GL_UNSIGNED_BYTE, buffer);
    VortexCheckGlError();
    free(buffer);

    return true;
}

// ================================================================================================
// Texture data.

Texture::Data::~Data() { clear(); }

Texture::Data::Data(int inW, int inH, Texture::Format inFmt,
                    const uint8_t* pixels, bool mipmap) {
    w = inW;
    h = inH;
    fmt = inFmt;
    handle = 0;

    glGenTextures(1, &handle);
    VortexCheckGlError();

    Format usedFmt = fmt;
    Vector<uint8_t> tempPixels;
    if (fmt == Texture::ALPHA && !Shader::isSupported()) {
        pixels = ConvertAlphaToLuma(tempPixels, pixels, inW, inH);
        usedFmt = Texture::LUMA;
    }

    if (handle) {
        bool unaligned = (usedFmt != Texture::RGBA && (w & 3));
        if (unaligned) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, sIntFmtGL[usedFmt], w, h, 0,
                     sFmtGL[usedFmt], GL_UNSIGNED_BYTE, pixels);

        if (VortexCheckGlError()) {
            PowerOfTwoTexImage2D(
                usedFmt, w, h,
                pixels);  // Maybe NPOT textures are not supported.
        } else {
            mipmap = (mipmap && w == h && w >= 2 && (w & (w - 1)) == 0);
            if (mipmap) {
                GenerateMipmaps(w, h, pixels, usedFmt);
                VortexCheckGlError();
            }
        }

        int minFilter = mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (unaligned) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

    rw = static_cast<float>(1.0 / w);
    rh = static_cast<float>(1.0 / h);
    refs = 1;
    mipmapped = mipmap;
    cached = false;
}

void Texture::Data::clear() {
    if (handle) glDeleteTextures(1, &handle);
    handle = 0;
    cached = false;
}

void Texture::Data::modify(int mx, int my, int mw, int mh,
                           const uint8_t* pixels) {
    if (handle && mx >= 0 && my >= 0 && mx + mw <= w && my + mh <= h) {
        Format usedFmt = fmt;
        Vector<uint8_t> tempPixels;
        if (fmt == Texture::ALPHA && !Shader::isSupported()) {
            pixels = ConvertAlphaToLuma(tempPixels, pixels, mw, mh);
            usedFmt = Texture::LUMA;
        }

        bool unaligned = (usedFmt != Texture::RGBA && ((w & 3) || (mw & 3)));
        glBindTexture(GL_TEXTURE_2D, handle);
        if (unaligned) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexSubImage2D(GL_TEXTURE_2D, 0, mx, my, mw, mh, sFmtGL[usedFmt],
                        GL_UNSIGNED_BYTE, pixels);
        VortexCheckGlError();

        if (unaligned) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::Data::increaseHeight(int newHeight) {
    if (handle) {
        Format usedFmt = fmt;
        if (fmt == Texture::ALPHA && !Shader::isSupported()) {
            usedFmt = Texture::LUMA;
        }

        int channels = sNumChannels[usedFmt];
        uint8_t* pixels =
            static_cast<uint8_t*>(malloc(w * newHeight * channels));
        memset(pixels + w * h * channels, 0, w * (newHeight - h) * channels);

        glBindTexture(GL_TEXTURE_2D, handle);
        glGetTexImage(GL_TEXTURE_2D, 0, sFmtGL[usedFmt], GL_UNSIGNED_BYTE,
                      pixels);
        glTexImage2D(GL_TEXTURE_2D, 0, sIntFmtGL[usedFmt], w, newHeight, 0,
                     sFmtGL[usedFmt], GL_UNSIGNED_BYTE, pixels);
        VortexCheckGlError();

        glBindTexture(GL_TEXTURE_2D, 0);

        h = newHeight;
        rh = 1.0f / static_cast<float> max(h, 1);
        free(pixels);
    }
}

void Texture::Data::setFiltering(bool linear) {
    if (handle) {
        glBindTexture(GL_TEXTURE_2D, handle);
        int fmin = linear ? GL_LINEAR : GL_NEAREST, fmag = fmin;
        if (mipmapped)
            fmin = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, fmin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, fmag);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::Data::setWrapping(bool repeat) {
    if (handle) {
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        repeat ? GL_REPEAT : GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        repeat ? GL_REPEAT : GL_CLAMP);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// ================================================================================================
// Texture class.

typedef TextureManager TexMan;

#define TEXDATA ((Texture::Data*)data_)

Texture::~Texture() {
    if (data_) TexMan::release(TEXDATA);
}

Texture::Texture() : data_(nullptr) {}

Texture::Texture(int w, int h, Format fmt) : data_(nullptr) {
    w = max(w, 1);
    h = max(h, 1);
    Vector<uint8_t> pixels;
    pixels.resize(w * h * sNumChannels[fmt], 0);
    data_ = TexMan::load(w, h, fmt, false, pixels.begin());
}

Texture::Texture(const char* path, bool mipmap, Format fmt) : data_(nullptr) {
    data_ = TexMan::load(path, fmt, mipmap);
}

Texture::Texture(int w, int h, const uint8_t* pixels, bool mipmap, Format fmt)
    : data_(nullptr) {
    data_ = TexMan::load(w, h, fmt, mipmap, pixels);
}

Texture::Texture(Texture&& tex) : data_(tex.data_) { tex.data_ = nullptr; }

Texture::Texture(const Texture& tex) : data_(tex.data_) {
    if (data_) TEXDATA->refs++;
}

Texture& Texture::operator=(Texture&& tex) {
    if (data_) TexMan::release(TEXDATA);
    data_ = tex.data_;
    tex.data_ = nullptr;
    return *this;
}

Texture& Texture::operator=(const Texture& tex) {
    if (data_) TexMan::release(TEXDATA);
    data_ = tex.data_;
    if (data_) TEXDATA->refs++;
    return *this;
}

void Texture::modify(int x, int y, int w, int h, const uint8_t* pixels) {
    if (data_) TEXDATA->modify(x, y, w, h, pixels);
}

void Texture::setWrapping(bool repeat) {
    if (data_) TEXDATA->setWrapping(repeat);
}

void Texture::setFiltering(bool linear) {
    if (data_) TEXDATA->setFiltering(linear);
}

void Texture::cache() const {
    if (data_) TEXDATA->cached = true;
}

TextureHandle Texture::handle() const { return data_ ? TEXDATA->handle : 0; }

vec2i Texture::size() const {
    return data_ ? vec2i{TEXDATA->w, TEXDATA->h} : vec2i{0, 0};
}

int Texture::width() const { return data_ ? TEXDATA->w : 0; }

int Texture::height() const { return data_ ? TEXDATA->h : 0; }

Texture::Format Texture::format() const {
    return data_ ? TEXDATA->fmt : Texture::RGBA;
}

void Texture::LogInfo() {
    Debug::blockBegin(Debug::INFO, "loaded textures");
    Debug::log("amount: %i\n", TM->textures.size());
    for (auto tex : TM->textures) {
        const std::string* key = Map::findKey(TM->files, tex);
        std::string path =
            key ? Str::substr(*key, 0, key->length() - 2) : "-- no path --";

        Debug::log("\npath   : %s\n", path.c_str());
        Debug::log("size   : %i x %i\n", tex->w, tex->h);
        Debug::log("refs   : %i\n", tex->refs);
        Debug::log("cached : %i\n", tex->cached);
        Debug::log("mipmap : %s\n\n", (tex->mipmapped ? "yes" : "no"));
    }
    Debug::blockEnd();
}

int Texture::createTiles(const char* path, int tileW, int tileH, int numTiles,
                         Texture* outTiles, bool mipmap, Format fmt) {
    ImageLoader::Data image = ImageLoader::load(path, TexLoadFormats[fmt]);
    int ch = sNumChannels[fmt];

    Vector<uint8_t> pixelData;
    pixelData.resize(tileW * tileH * ch, 0);
    uint8_t* dst = pixelData.begin();
    int tileIndex = 0;

    for (int y = 0; y + tileH <= image.height; y += tileH) {
        for (int x = 0; x + tileW <= image.width; x += tileW) {
            if (tileIndex < numTiles) {
                uint8_t* src = image.pixels + ((y * image.width) + x) * ch;
                for (int line = 0; line < tileH; ++line) {
                    memcpy(dst + line * tileW * ch,
                           src + line * image.width * ch, tileW * ch);
                }
                outTiles[tileIndex++] = Texture(tileW, tileH, dst, mipmap, fmt);
            }
        }
    }
    ImageLoader::release(image);

    return tileIndex;
}

int Texture::createTiles(const char* path, int tileW, int tileH, int numTiles,
                         std::vector<Texture>& outTiles, bool mipmap,
                         Format fmt) {
    ImageLoader::Data image = ImageLoader::load(path, TexLoadFormats[fmt]);
    int ch = sNumChannels[fmt];

    Vector<uint8_t> pixelData;
    pixelData.resize(tileW * tileH * ch, 0);
    uint8_t* dst = pixelData.begin();
    int tileIndex = 0;

    for (int y = 0; y + tileH <= image.height; y += tileH) {
        for (int x = 0; x + tileW <= image.width; x += tileW) {
            if (tileIndex < numTiles) {
                uint8_t* src = image.pixels + ((y * image.width) + x) * ch;
                for (int line = 0; line < tileH; ++line) {
                    memcpy(dst + line * tileW * ch,
                           src + line * image.width * ch, tileW * ch);
                }
                outTiles.emplace_back(tileW, tileH, dst, mipmap, fmt);
            }
        }
    }
    ImageLoader::release(image);

    return tileIndex;
}
};  // namespace Vortex
