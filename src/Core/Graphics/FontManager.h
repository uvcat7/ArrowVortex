#pragma once

#include <Precomp.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>
#include <Core/Graphics/FontData.h>

namespace AV {
namespace FontManager {

void initialize();
void deinitialize();

// Font data that is managed as an entry in the font manager.
class Entry : public FontData
{
protected:
	Entry(FontFace*, FontHint);
};

// Loads a font from file and assigns it the given hinting mode.
Entry* load(const FilePath& path, FontHint hinting);

// Makes sure the font entry stays loaded, even if reference count reaches zero.
void cache(Entry* entry);

// Increments the reference count of the given font entry.
void addReference(Entry* entry);

// Decrements the reference count of the given font entry.
void release(Entry* entry);

// Adds a custom glyph that can be embedded in text through markup. For example, a custom
// glyph named "foo" can be drawn by embedding the markup "{G:foo}" in the target string.
bool addCustomGlyph(stringref name, unique_ptr<Texture>& texture, int dx, int dy, int advance);

// Returns the custom glyph associated with <name>, or null if none was found.
Glyph* getCustomGlyph(stringref name);

// Releases glyphs that have not been recently used from glyph pages of loaded fonts.
void optimizeGlyphPages();

// Returns a placeholder glyph that roughly matches the requested font size.
const Glyph& getPlaceholderGlyph(int fontSize);

// Reports statistics on the current font usage.
void debugInfo();

} // namespace FontManager
} // namespace AV
