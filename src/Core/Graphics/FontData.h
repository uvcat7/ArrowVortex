#pragma once

#include <Precomp.h>

#include <Core/Common/NonCopyable.h>

#include <Core/Types/Rect.h>
#include <Core/Types/Rect.h>

#include <Core/Graphics/Font.h>
#include <Core/Graphics/Texture.h>

namespace AV {

struct FontFace;
struct GlyphSlot;

// Contains data of a single glyph.
struct Glyph
{
	Glyph();

	uint hasTexture : 1;
	uint hasAlphaTex : 1;
	uint isWhitespace : 1;
	uint isNewline : 1;
	uint isInUse : 1;
	uint dummy : 27;

	// How much the cursor x-position should advance when moving to the next character.
	int advance;

	// The position and size of the glyph when drawn, relative to the cursor position.
	Rect box;

	// The position and size of the glyph slot on the glyph page.
	Rect slot;

	// Texture coordinates of the glyph on the glyph page texture.
	float uvs[8];

	// Index used by FreeType to identify the glyph.
	int charIndex;

	// Unicode codepoint of the character that the glyph represents.
	int codepoint;

	// Texture of the glyph page that contains the glyph.
	Texture* texture;

	// The next glyph on the shelf, or null if it is the last glyph on the shelf.
	Glyph* next;

	// Timestamp of the most recent startTime the glyph was used.
	float lastUseTime;
};

// Contains a texture with glyphs of a specific font size.
class GlyphPage
{
public:
	~GlyphPage();
	GlyphPage(FontData* owner, int fontSize);

	void increaseHeight(int minimumHeight);
	Rect getEmptySlot(int width, int height);
	Glyph* addGlyph(GlyphSlot* slot);
	Glyph* getGlyph(int codepoint);

	void optimize();
	bool isEmpty() const;

	inline const Texture& texture() const { return myTexture; }

private:
	int myFontSize;
	FontData* myOwner;
	Texture myTexture;
	int myShelfX;
	int myShelfY;
	int myShelfH;
	Glyph* myShelfBegin;
	Glyph* myShelfEnd;
	std::unordered_map<int, Glyph*> myGlyphs;
	vector<Glyph*> myEmptySlots;
};

// Instance of a font, contains the actual font data.
class FontData : NonCopyable
{
public:
	~FontData();
	FontData(FontFace* face, FontHint hinting);

	// Releases unused glyphs.
	void optimize();

	// Returns the glyph page for the given font size.
	GlyphPage* getPage(int fontSize);

	// Returns the glyph for the given codepoint at the given fontsize.
	Glyph* getGlyph(int fontSize, int codepoint);

	// Returns the kerning delta in pixels between the two glyphs.
	int getKerning(const Glyph* left, const Glyph* right) const;

	// Returns the load flags.
	inline int loadFlags() const { return myLoadFlags; }

	// Returns the font face.
	inline FontFace* face() const { return myFace; }

private:
	std::unordered_map<int, GlyphPage*> myPages;
	GlyphPage* myActivePage;
	int myActiveFontSize;
	FontFace* myFace;
	int myLoadFlags;
};

} // namespace AV
