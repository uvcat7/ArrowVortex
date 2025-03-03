#include <Precomp.h>

#include <Core/Utils/Unicode.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/System/Debug.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/FontManager.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// TextStyle.

TextStyle::TextStyle()
	: fontSize(12)
	, flags(TextOptions::None)
	, textColor(Color::White)
	, shadowColor(Color::Black)
{
}

// =====================================================================================================================
// Text :: Static data.

static constexpr int UnlimitedLineWidth = INT_MAX;

// Style data.

static TextStyle* sTextStyle = nullptr;
static TextAlign sTextAlign = TextAlign::TL;

// Formatted text data.

struct TextGlyph
{
	const Glyph* glyph;
	int charIndex;
	int x;
};

struct TextMarkup
{
	enum class Type
	{
		TEXT_COLOR,
		SHADOW_COLOR,
	};
	Type type;
	int glyphIndex;
	Color color;
};

struct TextLine
{
	int beginGlyph;
	int endGlyph;
	int x;
	int y;
	int top;
	int bottom;
	int width;
};

struct TextBox
{
	int line;
	int xl, xr;
	Color color;
	bool isForeground;
};

static TextBox* sBoxBuffer = nullptr;
static int sBoxCount = 0;
static int sBoxCap = 0;

static TextGlyph* sGlyphBuffer = nullptr;
static int sGlyphCount = 0;
static int sGlyphCap = 0;

static TextLine* sLineBuffer = nullptr;
static int sLineCount = 0;
static int sLineCap = 0;

static TextMarkup* sMarkupBuffer = nullptr;
static int sMarkupCount = 0;
static int sMarkupCap = 0;

static int sTextWidth = 0;
static int sTextHeight = 0;
static int sTextNumChars = 0;

// Temporary Formatting data.

struct TemporaryFormattingData
{
	struct BoxState
	{
		bool enabled;
		Color color;
		int x;
	};

	const AV::Glyph* previousGlyph;

	FontData* baseFont;
	FontData* font;

	TextOptions flags;
	int fontSize;
	Color textColor;
	Color shadowColor;
	bool useKerning;

	BoxState fgbox;
	BoxState bgbox;

	int stringLength;
	int maxLineWidth;
	int lineTop;
	int lineBottom;
	int lineBeginGlyph;
	int lineWidth;
	int lineIndex;
	int charIndex;
	int nextCharIndex;
	int numReferences;
};

static TemporaryFormattingData sFmt;

// Temporary drawing data.

struct TemporaryDrawingData
{
	Color color = Color::White;
	Texture* texture;
	bool isShadowPass;
};

static TemporaryDrawingData sDraw;

// =====================================================================================================================
// Text :: (de)initialization.

#define ALLOC(name, n, type) { name##Cap = n; name##Buffer = (type*)malloc(n * sizeof(type)); }
#define FREE(x) { free(x); x = nullptr; }

void Text::initialize()
{
	sTextStyle = new TextStyle();

	ALLOC(sBox,    8,   TextBox);
	ALLOC(sGlyph,  512, TextGlyph);
	ALLOC(sLine,   32,  TextLine);
	ALLOC(sMarkup, 8,   TextMarkup);
}

void Text::deinitialize()
{
	Util::reset(sTextStyle);

	FREE(sBoxBuffer);
	FREE(sGlyphBuffer)
	FREE(sLineBuffer);
	FREE(sMarkupBuffer);
}

#undef ALLOC_BUFFER
#undef DEL

// =====================================================================================================================
// Text :: Style manipulation.

void Text::setStyle(const TextStyle& style)
{
	*sTextStyle = style;
}

void Text::setStyle(const TextStyle& style, TextOptions flags)
{
	*sTextStyle = style;
	sTextStyle->flags = flags;
}

void Text::setStyle(const TextStyle& style, Color textColor)
{
	*sTextStyle = style;
	sTextStyle->textColor = textColor;
}

void Text::setFontSize(int size)
{
	sTextStyle->fontSize = size;
}

void Text::setFlags(TextOptions flags)
{
	sTextStyle->flags = flags;
}

void Text::setTextColor(Color color)
{
	sTextStyle->textColor = color;
}

void Text::setShadowColor(Color color)
{
	sTextStyle->shadowColor = color;
}

// =====================================================================================================================
// Text :: Formatting, buffer operations.

#define FOR_EACH_LINE(it)\
	for (auto it = sLineBuffer, lineBufferEnd = sLineBuffer + sLineCount; it != lineBufferEnd; ++it)

#define FOR_EACH_BOX(it)\
	for (auto it = sBoxBuffer, boxBufferEnd = sBoxBuffer + sBoxCount; it != boxBufferEnd; ++it)

static TextBox& PushBox()
{
	if (sBoxCount == sBoxCap)
		sBoxBuffer = (TextBox*)realloc(sBoxBuffer, (sBoxCap <<= 1) * sizeof(TextBox));
	return sBoxBuffer[sBoxCount++];
}

static TextGlyph& PushGlyph()
{
	if (sGlyphCount == sGlyphCap)
		sGlyphBuffer = (TextGlyph*)realloc(sGlyphBuffer, (sGlyphCap <<= 1) * sizeof(TextGlyph));
	return sGlyphBuffer[sGlyphCount++];
}

static TextLine& PushLine()
{
	if (sLineCount == sLineCap)
		sLineBuffer = (TextLine*)realloc(sLineBuffer, (sLineCap <<= 1) * sizeof(TextLine));
	return sLineBuffer[sLineCount++];
}

static TextMarkup& PushMarkup()
{
	if (sMarkupCount == sMarkupCap)
		sMarkupBuffer = (TextMarkup*)realloc(sMarkupBuffer, (sMarkupCap <<= 1) * sizeof(TextMarkup));
	return sMarkupBuffer[sMarkupCount++];
}

// =====================================================================================================================
// Text :: Formatting, markup parsing.

#define TAG1(a)       ((uint64_t)a)
#define TAG2(a, b)    ((TAG1(a) << 8) | b)
#define TAG3(a, b, c) ((TAG2(a, b) << 8) | c)

static void SetLineMetrics()
{
	sFmt.lineTop = min(sFmt.lineTop, -sFmt.fontSize);
	sFmt.lineBottom = max(sFmt.lineBottom, sFmt.fontSize / 4);
}

static const Glyph* GetGlyph(int codepoint);

static double ReadNumber(const uchar* p, size_t len)
{
	bool negative = false;
	double out = 0.0;
	int i = 0;

	// Skip whitespace.
	for (; i < len && isspace(p[i]); ++i);

	// Leading plus or minus.
	if (p[i] == '-')
	{
		negative = true;
		++i;
	}
	else if (p[i] == '+')
	{
		++i;
	}

	// Integer part.
	for (; i < len && p[i] >= '0' && p[i] <= '9'; ++i)
	{
		out *= 10.0;
		out += (double)(p[i] - '0');
	}

	// Fractional part.
	if (p[i] == '.')
	{
		double mul = 0.1;
		for (++i; i < len && p[i] >= '0' && p[i] <= '9'; ++i)
		{
			out += (double)(p[i] - '0') * mul;
			mul *= 0.1;
		}
	}

	// Negate value if there is a leading minus.
	if (negative) out *= -1;

	return out;
}

static int HexDigit(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static int ReadHexColor(uchar* out, const uchar* p, size_t n)
{
	int numComponents = 0;
	switch (n)
	{
	case 8:
		out[3] = HexDigit(p[6]) * 16 + HexDigit(p[7]);
		++numComponents;
	case 6:
		out[0] = HexDigit(p[0]) * 16 + HexDigit(p[1]);
		out[1] = HexDigit(p[2]) * 16 + HexDigit(p[3]);
		out[2] = HexDigit(p[4]) * 16 + HexDigit(p[5]);
		++numComponents;
		break;
	case 4:
		out[3] = HexDigit(p[3]) * 17;
		++numComponents;
	case 3:
		out[0] = HexDigit(p[0]) * 17;
		out[1] = HexDigit(p[1]) * 17;
		out[2] = HexDigit(p[2]) * 17;
		++numComponents;
		break;
	};
	return numComponents;
}

static void ReadMarkupColor(Color& out, const uchar* param, size_t len, Color base)
{
	uchar channels[4];
	switch (ReadHexColor(channels, param, len))
	{
	case 2:
		out = Color(channels[0], channels[1], channels[2], channels[3]);
		break;
	case 1:
		out = Color(channels[0], channels[1], channels[2], base.alpha());
		break;
	case 0:
		out = base;
		break;
	};
}

static void ReadTextColor(const uchar* param, size_t len)
{
	ReadMarkupColor(sFmt.textColor, param, len, sTextStyle->textColor);
	auto& item = PushMarkup();
	item.type = TextMarkup::Type::TEXT_COLOR;
	item.glyphIndex = sGlyphCount;
	item.color = sFmt.textColor;
}

static void ReadShadowColor(const uchar* param, size_t len)
{
	ReadMarkupColor(sFmt.shadowColor, param, len, sTextStyle->shadowColor);
	auto& item = PushMarkup();
	item.type = TextMarkup::Type::SHADOW_COLOR;
	item.glyphIndex = sGlyphCount;
	item.color = sFmt.shadowColor;
}

static void ReadFontSize(const uchar* param, size_t len)
{
	int fontSize = int(ReadNumber(param, len) + 0.5);

	if (len == 0)
	{
		fontSize = sTextStyle->fontSize;
	}
	else if (param[len - 1] == '%')
	{
		double baseSize = (double)sTextStyle->fontSize / 100.0;
		fontSize = int(ReadNumber(param, len) * baseSize + 0.5);
	}
	else
	{
		fontSize = int(ReadNumber(param, len) + 0.5);
	}
	sFmt.fontSize = clamp(fontSize, 1, 256);
	SetLineMetrics();
}

static void ReadFgColor(const uchar* param, size_t len)
{
	if (sFmt.fgbox.enabled)
	{
		auto& box = PushBox();
		box.isForeground = true;
		box.color = sFmt.fgbox.color;
		box.line = sFmt.lineIndex;
		box.xl = sFmt.fgbox.x;
		box.xr = sFmt.lineWidth;
		sFmt.fgbox.enabled = false;
	}
	uchar rgba[4];
	if (ReadHexColor(rgba, param, len))
	{
		sFmt.fgbox.enabled = true;
		sFmt.fgbox.color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
		sFmt.fgbox.x = sFmt.lineWidth;
	}
}

static void ReadBgColor(const uchar* param, size_t len)
{
	if (sFmt.bgbox.enabled)
	{
		auto& box = PushBox();
		box.isForeground = false;
		box.color = sFmt.bgbox.color;
		box.line = sFmt.lineIndex;
		box.xl = sFmt.bgbox.x;
		box.xr = sFmt.lineWidth;
		sFmt.bgbox.enabled = false;
	}
	uchar rgba[4];
	if (ReadHexColor(rgba, param, len))
	{
		sFmt.bgbox.enabled = true;
		sFmt.bgbox.color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
		sFmt.bgbox.x = sFmt.lineWidth;
	}
}

static const Glyph* ReadMarkup(const uchar* str)
{
	// Keep reading tags until a glyph is returned.
	const uchar* p = str + sFmt.nextCharIndex;
	while (p[0] == '{')
	{
		sFmt.charIndex = sFmt.nextCharIndex;

		// A leading slash indicates undo.
		bool undo = (p[1] == '/');
		if (undo) ++p;

		// Read the tag name.
		const uchar* name = ++p;
		while (*p && *p != '}' && *p != ':') ++p;
		auto nameLen = p - name;
		if (*p == ':') ++p;

		// Read the tag parameters.
		const uchar* param = p;
		while (*p && *p != '}') ++p;
		auto paramLen = p - param;
		if (*p == '}') ++p;

		// Advance the read position to the tag end.
		sFmt.nextCharIndex = int(p - str);

		// Turn the name into a 64-bit identifier.
		uint64_t id = 0;
		for (int i = 0; i < nameLen; ++i)
		{
			id = (id << 8) | toupper(name[i]);
		}

		// Perform the action associated with the identifier.
		switch (id)
		{
		case TAG1('N'):
			return GetGlyph('\n');
		case TAG1('T'):
			return GetGlyph('\t');
		case TAG1('G'):
			return FontManager::getCustomGlyph(string((const char*)param, paramLen));
		case TAG2('L', 'B'):
			return GetGlyph('{');
		case TAG2('R', 'B'):
			return GetGlyph('}');
		case TAG2('F', 'C'):
			ReadFgColor(param, paramLen); break;
		case TAG2('B', 'C'):
			ReadBgColor(param, paramLen); break;
		case TAG2('S', 'C'):
			ReadShadowColor(param, paramLen); break;
		case TAG2('T', 'C'):
			ReadTextColor(param, paramLen); break;
		case TAG2('T', 'S'):
			ReadFontSize(param, paramLen); break;
		};
	}

	// The markup tags did not return a glyph, continue reading.
	return nullptr;
}

// =====================================================================================================================
// Text :: Formatting, glyph and line building.

static const Glyph* GetGlyph(int codepoint)
{
	const Glyph* glyph = nullptr;

	// First check if it is already available in the glyph cache of the current font.
	if (sFmt.font) glyph = sFmt.font->getGlyph(sFmt.fontSize, codepoint);

	// If not, return a placeholder glyph.
	if (!glyph) glyph = &FontManager::getPlaceholderGlyph(sFmt.fontSize);

	return glyph;
}

static const Glyph* ReadNextGlyph(const uchar* str)
{
	int codepoint = str[sFmt.nextCharIndex];

	// Check if the current character is the start of a markup tag.
	if (codepoint == '{' && (sFmt.flags & TextOptions::ReadMarkup))
	{
		auto* glyph = ReadMarkup(str);
		if (glyph) return glyph;
		codepoint = str[sFmt.nextCharIndex];
	}
	if (codepoint == 0) return nullptr;

	// Advance to the next character.
	sFmt.charIndex = sFmt.nextCharIndex;
	++sFmt.nextCharIndex;

	// Keep reading if the current character is multibyte UTF-8.
	if (codepoint >= 0x80)
	{
		const uchar* p = str + sFmt.nextCharIndex;
		int tb = Unicode::utf8TrailingBytes[codepoint];
		for (int i = 0; i < tb && *p; ++i, ++p) codepoint = (codepoint << 6) + *p;
		codepoint -= Unicode::utf8MultibyteResidu[tb];
		sFmt.nextCharIndex = int(p - str);
	}

	return GetGlyph(codepoint);
}

static void AddEllipsesToLine()
{
	const Glyph* ellipsesGlyph = GetGlyph('.');
	int ellipsesW = ellipsesGlyph->advance * 3;
	int maxLineW = max(0, sFmt.maxLineWidth - ellipsesW);

	// In order to do ellipses, the line must contain atleast one glyph.
	if (sFmt.lineBeginGlyph == sGlyphCount) return;
	int firstGlyph = sFmt.lineBeginGlyph;
	int lastGlyph = sGlyphCount - 1;
	int lineEndCharIndex = sGlyphBuffer[firstGlyph].charIndex;

	// Omit all glyphs that are past the maximum line width.
	while (lastGlyph >= firstGlyph && sGlyphBuffer[lastGlyph].x + sGlyphBuffer[lastGlyph].glyph->advance > maxLineW)
	{
		lineEndCharIndex = sGlyphBuffer[lastGlyph].charIndex;
		--lastGlyph;
	}

	// We don't put ellipses after whitespace, so omit trailing whitespace as well.
	while (lastGlyph >= firstGlyph && sGlyphBuffer[lastGlyph].glyph->isWhitespace)
	{
		lineEndCharIndex = sGlyphBuffer[lastGlyph].charIndex;
		--lastGlyph;
	}

	// Determine the new line width.
	if (lastGlyph >= 0)
	{
		sFmt.lineWidth = sGlyphBuffer[lastGlyph].x + sGlyphBuffer[lastGlyph].glyph->advance;
	}
	else
	{
		sFmt.lineWidth = 0;
	}

	// Clamp boxes to the new line width.
	for (auto box = sBoxBuffer, end = sBoxBuffer + sBoxCount; box != end; ++box) 
	{
		if (box->line == sFmt.lineIndex)
			box->xr = min(box->xr, box->xl + sFmt.lineWidth);
	}

	// Append three ellipses glyphs after the last glyph.
	sGlyphCount = lastGlyph + 1;
	if (ellipsesW < sFmt.maxLineWidth)
	{
		for (int i = 0; i < 3; ++i)
		{
			auto& item = PushGlyph();
			item.glyph = ellipsesGlyph;
			item.x = sFmt.lineWidth;
			item.charIndex = lineEndCharIndex;
			sFmt.lineWidth += ellipsesGlyph->advance;
		}
	}

	// Make sure all markup from the omitted part is applied after the ellipses.
	int postEllipsesIndex = sGlyphCount;
	int markupIndex = sMarkupCount - 1;
	while (markupIndex >= 0 && sMarkupBuffer[markupIndex].glyphIndex > lastGlyph)
	{
		sMarkupBuffer[markupIndex].glyphIndex = postEllipsesIndex;
		--markupIndex;
	}
}

static void FinishCurrentLine(bool last)
{
	// Finish the current foreground box.
	if (sFmt.fgbox.enabled && sFmt.lineWidth > sFmt.fgbox.x)
	{
		auto& box = PushBox();
		box.isForeground = true;
		box.color = sFmt.fgbox.color;
		box.line = sFmt.lineIndex;
		box.xl = sFmt.fgbox.x;
		box.xr = sFmt.lineWidth;
		sFmt.fgbox.x = 0;
	}

	// Finish the current background box.
	if (sFmt.fgbox.enabled && sFmt.lineWidth > sFmt.fgbox.x)
	{
		auto& box = PushBox();
		box.isForeground = false;
		box.color = sFmt.fgbox.color;
		box.line = sFmt.lineIndex;
		box.xl = sFmt.fgbox.x;
		box.xr = sFmt.lineWidth;
		sFmt.fgbox.x = 0;
	}

	// Add ellipses if necessary.
	if ((sFmt.flags & TextOptions::Ellipses) && sFmt.lineWidth > sFmt.maxLineWidth)
	{
		AddEllipsesToLine();
	}

	// Calculate the y-value of the baseline.
	int lineY = -sFmt.lineTop;
	if (sLineCount > 0)
	{
		auto& previousLine = sLineBuffer[sLineCount - 1];
		lineY += previousLine.y + previousLine.bottom;
	}

	// Store the current line info.
	auto& line = PushLine();
	line.beginGlyph = sFmt.lineBeginGlyph;
	line.endGlyph = sGlyphCount;
	line.x = 0;
	line.y = lineY;
	line.width = sFmt.lineWidth;
	line.top = sFmt.lineTop;
	line.bottom = sFmt.lineBottom;

	// Update the size of the text area.
	sTextWidth = max(sTextWidth, sFmt.lineWidth);
	sTextHeight = lineY + sFmt.lineBottom;

	// advance to the next line.
	sFmt.lineBeginGlyph = sGlyphCount;
	sFmt.previousGlyph = nullptr;
	sFmt.lineWidth = 0;
	++sFmt.lineIndex;
}

static void PrepareArrange(int maxLineWidth)
{
	sFmt.flags = sTextStyle->flags;
	sFmt.font = sTextStyle->font;
	sFmt.fontSize = sTextStyle->fontSize;
	sFmt.textColor = sTextStyle->textColor;
	sFmt.shadowColor = sTextStyle->shadowColor;

	sFmt.charIndex = 0;
	sFmt.nextCharIndex = 0;
	sFmt.previousGlyph = nullptr;

	sFmt.bgbox.enabled = false;
	sFmt.fgbox.enabled = false;

	sFmt.lineIndex = 0;
	sFmt.lineBeginGlyph = 0;

	sFmt.lineWidth = 0;
	sFmt.lineTop = 0;
	sFmt.lineBottom = 0;

	sFmt.maxLineWidth = max(0, maxLineWidth);
	if (maxLineWidth == UnlimitedLineWidth)
		Flag::reset(sFmt.flags, TextOptions::Ellipses);
}

static void CreateLayout(const uchar* str)
{
	// Make a list of every line of glyphs.
	bool isMultiline = !(sFmt.flags & TextOptions::SingleLine);
	while (true)
	{
		auto glyph = ReadNextGlyph(str);
		if (!glyph) break;

		// Apply glyph advance.
		if ((sFmt.flags & TextOptions::Kerning) && sFmt.previousGlyph)
		{
			if (glyph->charIndex && sFmt.previousGlyph->charIndex)
			{
				int dx = sFmt.font->getKerning(sFmt.previousGlyph, glyph);
				sFmt.lineWidth += dx;
			}
		}

		// Insert the glyph in the list.
		auto& item = PushGlyph();
		item.glyph = glyph;
		item.x = sFmt.lineWidth;
		item.charIndex = sFmt.charIndex;

		// Check if we are forced to break the line and continue on a new line.
		if (glyph->isNewline && isMultiline)
		{
			FinishCurrentLine(false);
		}
		else
		{
			sFmt.previousGlyph = glyph;
			sFmt.lineWidth += glyph->advance;
		}
	}

	// Finally, add left over characters to the last line.
	if (sGlyphCount > sFmt.lineBeginGlyph&& sFmt.previousGlyph)
	{
		FinishCurrentLine(true);
	}
}

static void AlignText()
{
	// Apply horizontal alignments (left = 0, center = 1, right = 2).
	static const int AlignH[9] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
	switch (AlignH[(int)sTextAlign])
	{
	case 1:
		FOR_EACH_LINE(line)
			line->x -= line->width / 2;
		break;
	case 2:
		FOR_EACH_LINE(line)
			line->x -= line->width;
		break;
	};

	// Apply vertical alignments (top = 0, middle = 1, bottom = 2).
	static const int AlignV[9] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
	switch (AlignV[(int)sTextAlign])
	{
	case 1:
		FOR_EACH_LINE(line)
			line->y -= sTextHeight / 2;
		break;
	case 2:
		FOR_EACH_LINE(line)
			line->y -= sTextHeight;
		break;
	};
}

// =====================================================================================================================
// Text :: Formatting.

void Text::format(TextAlign align, const char* text)
{
	format(align, UnlimitedLineWidth, text);
}


void Text::format(TextAlign align, stringref text)
{
	format(align, UnlimitedLineWidth, text.data());
}

void Text::format(TextAlign align, int maxLineWidth, stringref text)
{
	format(align, maxLineWidth, text.data());
}

void Text::format(TextAlign align, int maxLineWidth, const char* text)
{
	sTextWidth = 0;
	sTextHeight = 0;

	sBoxCount = 0;
	sGlyphCount = 0;
	sLineCount = 0;
	sMarkupCount = 0;

	sTextAlign = align;
	sTextNumChars = (int)strlen(text);

	PrepareArrange(maxLineWidth);
	SetLineMetrics();
	CreateLayout((const uchar*)text);
	AlignText();
}

// =====================================================================================================================
// Text :: Info functions.

int Text::getWidth()
{
	return sTextWidth;
}

int Text::getHeight()
{
	return sTextHeight;
}

Vec2 Text::getTextPos(Rect r)
{
	switch (sTextAlign)
	{
	case TextAlign::TL: return r.tl();
	case TextAlign::TC: return r.centerT();
	case TextAlign::TR: return r.tr();
	case TextAlign::ML: return r.centerL();
	case TextAlign::MC: return r.center();
	case TextAlign::MR: return r.centerR();
	case TextAlign::BL: return r.bl();
	case TextAlign::BC: return r.centerB();
	case TextAlign::BR: return r.br();
	};
	return r.center();
}

Rect Text::getBounds(Vec2 p)
{
	int x = p.x;
	int y = p.y;
	int w = sTextWidth;
	int h = sTextHeight;
	switch (sTextAlign)
	{
	case TextAlign::TL: return { x, y, w, h };
	case TextAlign::TC: return { x - w / 2, y, w, h };
	case TextAlign::TR: return { x - w, y, w, h };
	case TextAlign::ML: return { x, y - h / 2, w, h };
	case TextAlign::MC: return { x - w / 2, y - h / 2, w, h };
	case TextAlign::MR: return { x - w, y - h / 2, w, h };
	case TextAlign::BL: return { x, y - h, w, h };
	case TextAlign::BC: return { x - w / 2, y - h, w, h };
	case TextAlign::BR: return { x - w, y - h, w, h };
	};
	return { x, y, w, h };
}

Rect Text::getBounds(Rect box)
{
	return getBounds(getTextPos(box));
}

int Text::getCharIndex(Rect textRegion, Vec2 cursorPos)
{
	return getCharIndex(getTextPos(textRegion), cursorPos);
}

int Text::getCharIndex(Vec2 textPos, Vec2 cursorPos)
{
	const auto* line = sLineBuffer;
	const auto* lineEnd = sLineBuffer + sLineCount;

	cursorPos.x -= textPos.x;
	cursorPos.y -= textPos.y;

	// If there are no glyphs, we return the start of the string.
	if (line == lineEnd)
		return 0;

	// If charPos is above the first line, we return the start of the string.
	if (cursorPos.y < line->y + line->top)
		return 0;

	// Advance to the first line which overlaps with the y-position.
	while (line != lineEnd && cursorPos.y > line->y + line->bottom)
		++line;

	// If charPos is below the final line, we return the end of the string.
	if (line == lineEnd)
		return sTextNumChars;

	// Otherwise, we look for the closest character on the current line.
	auto* begin = sGlyphBuffer + line->beginGlyph;
	auto* end = sGlyphBuffer + line->endGlyph;
	for (auto item = begin; item != end; ++item)
	{
		int glyphX = line->x + item->x + item->glyph->advance / 2;
		if (glyphX > cursorPos.x) return item->charIndex;
	}

	// If we reach this point the x-position is after the end of the line.
	return sTextNumChars;
}

Text::CursorPos Text::getCursorPos(Rect textRegion, int charIndex)
{
	return getCursorPos(getTextPos(textRegion), charIndex);
}

Text::CursorPos Text::getCursorPos(Vec2 textPos, int charIndex)
{
	// If there are no glyphs, we return immediately.
	if (sLineCount == 0)
	{
		int textH = sTextStyle->fontSize * 5 / 4;
		Vec2 pos = getTextPos(Rect(0, 0, 0, textH));
		return { textPos.x, textPos.y - pos.y, textH };
	}

	// Return the rect of the first glyph on or after index.
	FOR_EACH_LINE(line)
	{
		auto* begin = sGlyphBuffer + line->beginGlyph;
		auto* end = sGlyphBuffer + line->endGlyph;
		for (auto item = begin; item != end; ++item)
		{
			if (item->charIndex >= charIndex)
			{
				int x = textPos.x + line->x + item->x;
				int y = textPos.y + line->y + line->top;
				return { x, y, line->bottom - line->top };
			}
		}
	}

	// The index is after the last glyph on the last line.
	const auto& line = sLineBuffer[sLineCount - 1];
	return {
		textPos.x + line.x + line.width,
		textPos.y + line.y + line.top,
		line.bottom - line.top
	};
}

// =====================================================================================================================
// Text :: Text drawing.

static void SetCurrentTexture(const AV::Glyph& g)
{
	auto texture = g.texture;
	sDraw.texture = texture;

	Renderer::flushBatch();
	Renderer::bindTextureAndShader(texture);
}

static void PushGlyph(int x, int y, const AV::Glyph& g)
{
	if (g.texture != sDraw.texture)
		SetCurrentTexture(g);

	Renderer::pushQuad(x + g.box.l, y + g.box.t, x + g.box.r, y + g.box.b, g.uvs);
}

// Applies the effect of a markup tag during glyph rendering.
static void ApplyMarkup(const TextMarkup& markup)
{
	switch (markup.type)
	{
	case TextMarkup::Type::TEXT_COLOR:
		if (!sDraw.isShadowPass)
		{
			Renderer::flushBatch();
			Renderer::setColor(markup.color);
		}
		break;
	case TextMarkup::Type::SHADOW_COLOR:
		if (sDraw.isShadowPass)
		{
			Renderer::flushBatch();
			Renderer::setColor(markup.color);
		}
		break;
	}
}

static void DrawGlyphs(Vec2 textPos)
{
	int markupIndex = 0, numMarkup = sMarkupCount;
	FOR_EACH_LINE(line)
	{
		int glyphIndex = line->beginGlyph, lineEnd = line->endGlyph;
		while (glyphIndex != lineEnd)
		{
			// Apply markup until the current glyph index is reached.
			while (markupIndex < numMarkup && sMarkupBuffer[markupIndex].glyphIndex <= glyphIndex)
			{
				ApplyMarkup(sMarkupBuffer[markupIndex]);
				++markupIndex;
			}

			// Find out how many glyph we can draw, until either the next markup or the line end.
			int drawEnd = lineEnd;
			if (markupIndex < numMarkup && sMarkupBuffer[markupIndex].glyphIndex < lineEnd)
			{
				drawEnd = sMarkupBuffer[markupIndex].glyphIndex;
			}

			// Draw said glyphs.
			for (; glyphIndex < drawEnd; ++glyphIndex)
			{
				auto& item = sGlyphBuffer[glyphIndex];
				if (item.glyph->hasTexture)
				{
					PushGlyph(textPos.x + line->x + item.x, textPos.y + line->y, *item.glyph);
				}
			}
		}
	}
	Renderer::flushBatch();
}

void Text::draw(Vec2 pos)
{
	sDraw.texture = nullptr;

	Renderer::resetColor();

	// Count boxes;
	bool hasForegroundBoxes = false;
	bool hasBackgroundBoxes = false;
	FOR_EACH_BOX(box)
	{
		hasForegroundBoxes |= box->isForeground;
		hasBackgroundBoxes |= !box->isForeground;
	}

	// Draw background boxes.
	if (hasBackgroundBoxes)
	{
		Renderer::bindShader(Renderer::Shader::Color);
		Renderer::startBatch(Renderer::FillType::ColoredQuads);
		FOR_EACH_BOX(box)
		{
			if (box->isForeground) continue;
			auto& line = sLineBuffer[box->line];
			int x = pos.x + line.x;
			int y = pos.y + line.y;
			Renderer::pushQuad(x + box->xl, y + line.top, x + box->xr, y + line.bottom, box->color);
		}
		Renderer::flushBatch();
	}

	// Set the intial properties.
	sDraw.texture = nullptr;
	Renderer::startBatch(Renderer::FillType::TexturedQuads);

	// Render the text.
	Renderer::setColor(sTextStyle->shadowColor);
	sDraw.isShadowPass = true;
	DrawGlyphs(pos);
	Renderer::setColor(sTextStyle->textColor);
	sDraw.isShadowPass = false;
	DrawGlyphs(pos);

	// Draw foreground boxes.
	if (hasForegroundBoxes)
	{
		Renderer::resetColor();
		Renderer::bindShader(Renderer::Shader::Color);
		Renderer::startBatch(Renderer::FillType::ColoredQuads);
		FOR_EACH_BOX(box)
		{
			if (!box->isForeground) continue;
			auto& line = sLineBuffer[box->line];
			int x = pos.x + line.x;
			int y = pos.y + line.y;
			Renderer::pushQuad(x + box->xl, y + line.top, x + box->xr, y + line.bottom, box->color);
		}
		Renderer::flushBatch();
	}
}

void Text::draw(int x, int y)
{
	draw(Vec2(x, y));
}

void Text::draw(Rect region)
{
	draw(getTextPos(region));
}

// =====================================================================================================================
// Text :: Markup utilities.

string Text::escapeMarkup(const char* text)
{
	string out;
	for (; *text; ++text)
	{
		if (*text == '{')
		{
			out.append("{lb}", 4);
		}
		else
		{
			out.push_back(*text);
		}
	}
	return out;
}

int Text::getEscapedCharIndex(const char* text, int index)
{
	int offset = index;
	for (int i = 0; i < index && text[i]; ++i)
	{
		if (text[i] == '{') offset += 3;
	}
	return offset;
}

} // namespace AV
