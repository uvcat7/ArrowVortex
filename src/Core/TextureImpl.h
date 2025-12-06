#pragma once

#include <Core/Texture.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Vortex {

struct Texture::Data {
    ~Data();
    Data(int w, int h, Texture::Format fmt, const uint8_t* pixels, bool mipmap);

    void clear();
    void modify(int x, int y, int w, int h, const uint8_t* pixels);
    void increaseHeight(int newHeight);
    void setFiltering(bool linear);
    void setWrapping(bool repeat);

    uint32_t handle;
    int w, h;
    float rw, rh;
    int refs;
    Texture::Format fmt;
    bool mipmapped;
    bool cached;
};

struct TextureManager {
    static void create();
    static void destroy();

    static Texture::Data* load(fs::path path, Texture::Format fmt, bool mipmap);
    static Texture::Data* load(int w, int h, Texture::Format fmt, bool mipmap,
                               const uint8_t* pixels);

    static void release(Texture::Data* tex);
};

};  // namespace Vortex
