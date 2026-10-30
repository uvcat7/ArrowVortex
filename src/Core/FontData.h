#pragma once

#include <Core/Draw.h>
#include <Core/Text.h>
#include <Core/TextureImpl.h>

#include <unordered_map>
#include <set>
#include <map>

namespace Vortex {

typedef int FontSize;
typedef int Codepoint;

struct FontData;

struct Glyph {
    uint32_t hasPixels : 1;
    uint32_t hasAlphaTex : 1;
    uint32_t isWhitespace : 1;
    uint32_t isNewline : 1;
    uint32_t dummy : 28;
    int advance;
    areai ofs;
    recti box;
    areaf uvs;
    int index;
    uint32_t charcode;
    FontData* font;
    Texture::Data* tex;
    float timeSinceLastUse;
    Glyph* next;
};

struct GlyphAreaCompare {
    bool operator()(const Glyph* a, const Glyph* b) const {
        return a->box.w * a->box.h < b->box.w * b->box.h;
    }
};

struct GlyphCache {
    Texture::Data* tex;
    int shelfX, shelfY, shelfH;
    Glyph *shelfStart, *shelfEnd;
    std::unordered_map<Codepoint, Glyph*> glyphs;
    std::multiset<Glyph*, GlyphAreaCompare> unusedGlyphs;
};

struct FontData {
    FontData(void* ftface, const char* path, Text::Hinting h);
    ~FontData();

    void clear();
    void update(float dt);
    void setSize(FontSize s);
    const Glyph* getGlyph(FontSize s, Codepoint c);

    int getKerning(const Glyph* left, const Glyph* right) const;
    bool hasKerning() const;

    TextureHandle getActiveTexture(int size, vec2i& outTexSize);

    std::map<FontSize, GlyphCache*> caches;
    GlyphCache* currentCache;
    int currentSize;
    void* ftface;
    std::string path;
    FontData* next;
    int refs;
    int loadflags;
    bool cached;
};

};  // namespace Vortex
