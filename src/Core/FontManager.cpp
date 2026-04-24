#include <Core/FontManager.h>

#include <Core/Texture.h>
#include <Core/Utils.h>
#include <Core/MapUtils.h>

#include <System/Debug.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_GLYPH_H

namespace Vortex {

namespace {

struct FontManagerInstance {
    typedef std::map<std::string, FontData*> FontMap;
    typedef std::map<std::string, Glyph> GlyphMap;

    FontMap fonts;
    void* ftLib;
    FontData* fallback;
    Glyph placeholderGlyphs[8];
    GlyphMap specialGlyphs;
};

FontManagerInstance* FM;

};  // anonymous namespace

// ================================================================================================
// Dummy glyphs

static void CreatePlaceholderGlyphs(Glyph* out) {
    static const int padding = 2;

    static const int w[8] = {2, 4, 6, 8, 10, 14, 18, 22};
    static const int h[8] = {4, 8, 12, 16, 20, 28, 36, 44};
    static const int b[8] = {1, 1, 1, 2, 2, 3, 3, 4};
    static const int bufferSize = (22 + padding) * (44 + padding);

    uint8_t tmp[bufferSize];
    for (int i = 0; i < 8; ++i) {
        memset(tmp, 0, bufferSize);
        int boxW = w[i] + padding;
        int boxH = h[i] + padding;
        for (int y = 0; y < h[i]; ++y) {
            for (int x = 0; x < w[i]; ++x) {
                tmp[(y + 1) * boxW + x + 1] = 255;
            }
        }
        for (int y = b[i]; y < b[i] * 4; ++y) {
            for (int x = b[i]; x < w[i] - b[i]; ++x) {
                tmp[(y + 1) * boxW + x + 1] = 0;
            }
        }

        Glyph& glyph = out[i];
        memset(&glyph, 0, sizeof(Glyph));

        float rw = 1.0f / static_cast<float>(boxW);
        float rh = 1.0f / static_cast<float>(boxH);

        glyph.hasPixels = 1;
        glyph.hasAlphaTex = 1;
        glyph.tex =
            TextureManager::load(boxW, boxH, Texture::ALPHA, false, tmp);
        glyph.uvs = {rw, rh, 1.0f - rw, 1.0f - rh};
        glyph.box = {0, 0, boxW, boxH};
        glyph.ofs = {b[i], -h[i], b[i] + w[i], 0};
        glyph.advance = w[i] + b[i] / 2 + 1;
    }
}

// ================================================================================================
// FontManager

void FontManager::create() {
    FM = new FontManagerInstance;

    FT_Library lib;
    FT_Init_FreeType(&lib);

    FM->ftLib = lib;
    FM->fallback = nullptr;

    CreatePlaceholderGlyphs(FM->placeholderGlyphs);
}

void FontManager::destroy() {
    for (auto& glyph : FM->placeholderGlyphs) {
        TextureManager::release(glyph.tex);
    }

    /// We can't delete fonts that still have references, so we just invalidate
    /// them.
    for (auto& font : FM->fonts) {
        if (font.second->refs == 0) {
            delete font.second;
        } else {
            font.second->clear();
        }
    }

    FT_Done_FreeType(static_cast<FT_Library>(FM->ftLib));

    delete FM;
    FM = nullptr;
}

void FontManager::release(FontData* font) {
    font->refs--;
    if (font->refs == 0 && !font->cached) {
        // Fonts can still exist after shutdown, don't assume FM is valid.
        if (FM) {
            FM->fonts.erase(font->path);
        }
        delete font;
    }
}

FontData* FontManager::find(const std::string& path) {
    auto it = Map::findVal(FM->fonts, path);
    return it ? *it : nullptr;
}

FontData* FontManager::load(const std::string& path, Text::Hinting hinting) {
    // If fonts are created before goo is initialized, just return null.
    if (!FM) return nullptr;

    // Check if the font is already loaded.
    auto it = Map::findVal(FM->fonts, path);
    if (it) {
        auto* font = *it;
        font->refs++;
        return font;
    }

    // If not, try to load the font.
    FT_Face face;
    int result =
        FT_New_Face(static_cast<FT_Library>(FM->ftLib), path.c_str(), 0, &face);
    if (result == FT_Err_Ok) {
        FontData* font = new FontData(face, path.c_str(), hinting);
        FM->fonts[path] = font;
        return font;
    } else {
        Debug::blockBegin(Debug::ERROR, "could not load font file");
        Debug::log("file: %s\n", path.c_str());
        switch (result) {
            case FT_Err_Cannot_Open_Resource:
                Debug::log("reason: file not found\n");
                break;
            case FT_Err_Unknown_File_Format:
                Debug::log("reason: unknown file format\n");
                break;
            case FT_Err_Invalid_File_Format:
                Debug::log("reason: invalid file format\n");
                break;
            default:
                Debug::log("ft-error: %i\n", result);
        };
        Debug::blockEnd();
    }

    return nullptr;
}

void FontManager::cache(FontData* font) {
    if (font && !font->cached) {
        font->cached = true;
        FontData** last = &FM->fallback;
        while (*last) last = &((*last)->next);
        *last = font;
    }
}

bool FontManager::loadGlyph(const std::string& name, const Texture& texture,
                            int dx, int dy, int advance) {
    auto tex = static_cast<Texture::Data*>(texture.data());
    if (tex) {
        texture.cache();

        Glyph glyph;
        memset(&glyph, 0, sizeof(Glyph));
        glyph.hasAlphaTex = (tex->fmt == Texture::ALPHA);
        glyph.hasPixels = 1;
        glyph.uvs = {0, 0, 1, 1};
        glyph.box = {0, 0, tex->w, tex->h};
        glyph.ofs = {dx, dy - tex->h, dx + tex->w, dy};
        glyph.tex = tex;
        glyph.advance = advance ? advance : (tex->w + 1);

        FM->specialGlyphs.insert({name, glyph});

        return true;
    }
    return false;
}

Glyph* FontManager::getGlyph(const std::string& name) {
    return Map::findVal(FM->specialGlyphs, name);
}

void FontManager::log() {
    Debug::blockBegin(Debug::INFO, "loaded fonts");
    Debug::log("amount: %i\n", FM->fonts.size());
    for (auto& font : FM->fonts) {
        auto& path = font.second->path;
        if (path.empty()) path = "-- no path --";

        Debug::log("\npath: %s\n", path.c_str());
        Debug::log("refs: %i\n", font.second->refs);
        Debug::log("cached: %i\n", font.second->cached);
    }
    Debug::blockEnd();
}

void FontManager::startFrame(float dt) {
    for (const auto& it : FM->fonts) {
        it.second->update(dt);
    }
}

const Glyph& FontManager::getPlaceholderGlyph(int size) {
    int i = max(0, min(size / 8, 7));
    return FM->placeholderGlyphs[i];
}

FontData* FontManager::fallback() { return FM->fallback; }

// ================================================================================================
// API functions from "draw.h"

void Font::cache() const { FontManager::cache(static_cast<FontData*>(data_)); }

void Font::LogInfo() { FontManager::log(); }

bool Text::createGlyph(const char* id, const Texture& texture, int dx, int dy,
                       int advance) {
    return FontManager::loadGlyph(id, texture, dx, dy, advance);
}

};  // namespace Vortex
