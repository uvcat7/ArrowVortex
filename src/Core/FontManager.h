#pragma once

#include <Core/FontData.h>

namespace Vortex {

struct FontManager {
    static void create();
    static void destroy();

    static FontData* load(const std::string& path, Text::Hinting h);
    static FontData* find(const std::string& path);
    static void cache(FontData* font);

    static bool loadGlyph(const std::string& name, const Texture& texture,
                          int dx, int dy, int advance);
    static Glyph* getGlyph(const std::string& name);
    static void release(FontData* font);

    static void log();
    static void startFrame(float dt);

    static FontData* fallback();

    static const Glyph& getPlaceholderGlyph(int size);

};  // FontManager

};  // namespace Vortex
