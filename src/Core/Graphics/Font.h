#pragma once

#include <Core/System/File.h>

namespace AV {

// Hinting modes used by FreeType for glyph rasterization.
enum class FontHint
{
	Auto,   // Use FreeType's auto-hinter.
	Normal, // Use the font's native hinter.
	Light,  // Lighter hinting algorithm.
	None,   // Disable hinting entirely.
	Mono,   // Produces monochrome output instead of grayscale.
};

// Reference to a shared instance of font data.
class Font
{
public:
	~Font();
	Font();
	Font(Font&&);
	Font(const Font&);

	Font& operator = (Font&&);
	Font& operator = (const Font&);

	// Loads a font from a TrueType or OpenType font file.
	Font(const FilePath& path, FontHint hinting = FontHint::Normal);

	// Keeps the font data loaded, even if reference count reaches zero.
	void cache() const;

	// Returns the underlying font data.
	inline FontData* source() const { return mySource; }

	// Returns the underlying font data.
	inline operator FontData* () const { return mySource; }

private:
	FontData* mySource;
};

} // namespace AV
