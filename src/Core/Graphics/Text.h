#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/Utils/Enum.h>

#include <Core/Types/Rect.h>

#include <Core/Graphics/Color.h>
#include <Core/Graphics/Font.h>
#include <Core/Graphics/Text.h>

namespace AV {

struct FormattedText;

// Combination of horizontal and vertical text alignments.
enum class TextAlign
{
	TL, // Top, left.
	TC, // Top, center.
	TR, // Top, right.
	ML, // Middle, left.
	MC, // Middle, center.
	MR, // Middle, right.
	BL, // Bottom, left.
	BC, // Bottom, center.
	BR, // Bottom, right.
};

// Flags that affect text processing/layout.
enum class TextOptions
{
	// None.
	None = 0,

	// Read and remove markup from the text.
	ReadMarkup = 1 << 0,

	// Apply the effects of markup to the text.
	ApplyMarkup = 1 << 1,

	// Put all text on a single line; newline characters are ignored.
	SingleLine = 1 << 2,

	// Kerning is applied to character spacing, when available.
	Kerning = 1 << 3,

	// Disables line wrap; text at the end of long lines is omitted and replaced with ellipses.
	Ellipses = 1 << 4,

	// Enables reading, removing, and application of markup.
	Markup = ReadMarkup | ApplyMarkup,
};

template <>
constexpr bool IsFlags<TextOptions> = true;

// Contains style and formatting settings that can be used to arrange text.
class TextStyle
{
public:
	TextStyle();

	// The font that is used to arrange/draw the text.
	Font font;

	// The size of the characters. 
	int fontSize;

	// A combination of flags that determine aspects of the layout and/or appearance of the text.
	TextOptions flags;

	// The color of the characters.
	Color textColor;

	// The color of the shadow behind the characters.
	Color shadowColor;
};

// Functions related to text rendering.
namespace Text
{
	void initialize();
	void deinitialize();

	// Returned by "getCursorPos" to indicate the cursor position.
	struct CursorPos { int x, y, h; };
	
// Style settings:
	
	// Sets the style that is used to arrange text.
	void setStyle(const TextStyle& style);
	
	// Sets the style that is used to arrange text.
	void setStyle(const TextStyle& style, TextOptions flags);
	
	// Sets the style that is used to arrange text.
	void setStyle(const TextStyle& style, Color textColor);
	
	// Sets the font size of the style that is used to format text.
	void setFontSize(int size);
	
	// Sets the flags that are used to format text.
	void setFlags(TextOptions flags);
	
	// Sets the text color that is used to format text.
	void setTextColor(Color color);
	
	// Sets the shadow color that is used to format text.
	void setShadowColor(Color color);
	
// Formatting:
	
	// Format text according to the current style and given alignment.
	void format(TextAlign align, stringref text);
	
	// Format text according to the current style and given alignment.
	void format(TextAlign align, const char* text);
	
	// Format text according to the current style, given alignment and maximum line width.
	void format(TextAlign align, int maxLineWidth, stringref text);
	
	// Format text according to the current style, given alignment and maximum line width.
	void format(TextAlign align, int maxLineWidth, const char* text);
	
// Info functions:
	
	// Returns the with of the formatted text.
	int getWidth();
	
	// Returns the height of the formatted text.
	int getHeight();
	
	// Returns the position inside rect that corresponds to the current alignment.
	// For example: TL corresponds to (0,0) and BR corresponds to (w,h).
	Vec2 getTextPos(Rect rect);
	
	// Returns the bounding box of the text arrangement when drawn at the given position.
	Rect getBounds(Vec2 pos);
	
	// Returns the bounding box of the text arrangement when drawn inside the given region.
	Rect getBounds(Rect region);
	
	// Returns the string index of the character at the given cursor position in the current text arrangement.
	int getCharIndex(Vec2 textPos, Vec2 cursorPos);
	
	// Returns the string index of the character at the given cursor position in the current text arrangement.
	int getCharIndex(Rect textRegion, Vec2 cursorPos);
	
	// Returns the cursor position in front of the character at the given string index in the current text arrangement.
	CursorPos getCursorPos(Vec2 textPos, int charIndex);
	
	// Returns the cursor position in front of the character at the given string index in the current text arrangement.
	CursorPos getCursorPos(Rect textRegion, int charIndex);
	
// Text drawing:
	
	// Draws the current formatted text at the given position.
	void draw(int x, int y);
	
	// Draws the current formatted text at the given position.
	void draw(Vec2 pos);
	
	// Draws the current formatted text inside the given region.
	void draw(Rect region);
	
// Markup utilities:
	
	// Escapes all characters that could be interpreted as markup.
	std::string escapeMarkup(const char* unformattedText);
	
	// Returns the new index of the given character after the transformation done by the escape markup function.
	int getEscapedCharIndex(const char* unformattedText, int charIndex);
};

} // namespace AV
