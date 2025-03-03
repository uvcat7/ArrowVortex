#pragma once

#include <Core/FontData.h>

namespace Vortex {

struct FontManager {

static void create();
static void destroy();

static FontData* load(StringRef path, Text::Hinting h);
static FontData* find(StringRef path);
static void cache(FontData* font);

static bool loadGlyph(StringRef name, const Texture& texture, int dx, int dy, int advance);
static Glyph* getGlyph(StringRef name);
static void release(FontData* font);

static void log();
static void startFrame(float dt);

static FontData* fallback();

static const Glyph& getPlaceholderGlyph(int size);

}; // FontManager

}; // namespace Vortex
