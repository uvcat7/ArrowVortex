#pragma once

#include <Core/Texture.h>

namespace Vortex {

struct TextStyle;

/// Functions and enumerations related to text rendering.
namespace Text {
/// Combination of horizontal and vertical text alignments.
enum Align {
    TL,  ///< Top, left.
    TC,  ///< Top, center.
    TR,  ///< Top, right.
    ML,  ///< Middle, left.
    MC,  ///< Middle, center.
    MR,  ///< Middle, right.
    BL,  ///< Bottom, left.
    BC,  ///< Bottom, center.
    BR,  ///< Bottom, right.
};

/// Hinting modes used by FreeType for glyph rasterization.
enum Hinting {
    HINT_NORMAL,  ///< Use the font's native hinter.
    HINT_AUTO,    ///< Use FreeType's auto-hinter.
    HINT_NONE,    ///< Disable hinting entirely.
    HINT_LIGHT,   ///< Lighter hinting algorithm.
    HINT_MONO,    ///< Produces monochrome output instead of grayscale.
};

/// Flags that affect text processing/layout.
enum Flags {
    /// Read and remove markup from the text.
    MARKUP_READ = 1 << 0,

    /// Apply the effects of markup to the text.
    MARKUP_APPLY = 1 << 1,

    /// Put all text on a single line; newline characters are ignored.
    SINGLE_LINE = 1 << 2,

    /// Kerning is applied to character spacing, when available.
    KERNING = 1 << 3,

    /// Disables line wrap; text at the end of long lines is omitted and
    /// replaced with ellipses.
    ELLIPSES = 1 << 4,

    /// Enables reading, removing, and application of markup.
    MARKUP = MARKUP_READ | MARKUP_APPLY,
};

/// Returned by "GetCursorPos" to indicate the cursor position.
struct CursorPos {
    int x, y, h;
};

/// Creates a text arrangement from a string of text.
vec2i arrange(Align align, const char* text);
vec2i arrange(Align align, int maxLineWidth, const char* text);
vec2i arrange(Align align, const TextStyle& style, const char* text);
vec2i arrange(Align align, const TextStyle& style, int maxLineWidth,
              const char* text);

/// Draws the current text arrangement.
void draw(vec2i textPos);
void draw(recti textBox);

/// Returns the size of the current text arrangement.
vec2i getSize();
int getWidth();

/// Returns the bounding box of the current text arrangement.
recti getBoundingBox(vec2i textPos);
recti getBoundingBox(recti textBox);

/// Returns the string index of the character at the given cursor position in
/// the current text arrangement.
int getCharIndex(vec2i textPos, vec2i cursorPos);
int getCharIndex(recti textBox, vec2i cursorPos);

/// Returns the cursor position in front of the character at the given String
/// index in the current text arrangement.
CursorPos getCursorPos(vec2i textPos, int charIndex);
CursorPos getCursorPos(recti textBox, int charIndex);

/// Escapes all characters that could be interpreted as markup tags.
std::string escapeMarkup(const char* unformattedText);

/// Returns the new index of the given character after the transformation done
/// by "EscapeMarkup".
int getEscapedCharIndex(const char* unformattedText, int charIndex);

/// Creates a glyph image that can be embedded in text by using the {g:name}
/// markup.
bool createGlyph(const char* name, const Texture& texture, int dx = 0,
                 int dy = 0, int advance = 0);
};  // namespace Text

// Reference counted font object.
class Font {
   public:
    ~Font();
    Font();
    Font(Font&& s);
    Font(const Font& s);
    Font& operator=(Font&& s);
    Font& operator=(const Font& s);

    /// Loads a font from a TrueType/OpenType font file.
    Font(const char* path, Text::Hinting hint = Text::HINT_NORMAL);

    /// Makes sure the font stays loaded until the shutdown of goo.
    void cache() const;

    /// Makes sure all digit glyphs have the same width.
    void forceUniformDigitWidth();

    /// Returns the OpenGL handle of the current glyph texture.
    TextureHandle texture(int size, vec2i& outTexSize);

    /// Returns the font data; used internally.
    inline void* data() const { return data_; }

    /// Logs debug info on the currently loaded fonts.
    static void LogInfo();

   private:
    void* data_;  // TODO: replace with a more descriptive variable name.
};

/// Settings for text rendering.
struct TextStyle {
    TextStyle();

    /// Use this as the default settings when a new text style is constructed.
    void makeDefault();

    Font font;
    int fontSize;
    uint32_t textFlags;
    uint32_t textColor;
    uint32_t shadowColor;
};

};  // namespace Vortex
