#include <Core/FontManager.h>
#include <Core/TextLayout.h>
#include <Core/TextDraw.h>
#include <Core/Renderer.h>
#include <Core/Utils.h>
#include <Core/Vector.h>
#include <Core/StringUtils.h>

#include <cctype>
#include <stdint.h>

namespace Vortex {
namespace {

enum { NO_MAX_LINE_WIDTH = -1 };

static LLayout* LD = nullptr;

};  // anonymous namespace

// ================================================================================================
// Utility functions

const uint8_t utf8TrailingBytes[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
};

const uint32_t utf8MultibyteResidu[6] = {0x0,       0x3080,     0xE2080,
                                         0x3C82080, 0xFA082080, 0x82082080};

// ================================================================================================
// Markup functions

static void SetLineMetrics();
static const Glyph* GetGlyph(int charcode);

#define ID1(a) ((uint64_t)a)
#define ID2(a, b) ((ID1(a) << 8) | b)
#define ID3(a, b, c) ((ID2(a, b) << 8) | c)
#define ID4(a, b, c, d) ((ID3(a, b, c) << 8) | d)
#define ID5(a, b, c, d, e) ((ID4(a, b, c, d) << 8) | e)

static double ReadNumber(const uint8_t* p, int len) {
    bool negative = false;
    double out = 0.0;
    int i = 0;

    // Skip whitespace.
    for (; i < len && isspace(p[i]); ++i);

    // Leading plus or minus.
    if (p[i] == '-') {
        negative = true;
        ++i;
    } else if (p[i] == '+') {
        ++i;
    }

    // Integer part.
    for (; i < len && p[i] >= '0' && p[i] <= '9'; ++i) {
        out *= 10.0;
        out += static_cast<double>(p[i] - '0');
    }

    // Fractional part.
    if (p[i] == '.') {
        double mul = 0.1;
        for (++i; i < len && p[i] >= '0' && p[i] <= '9'; ++i) {
            out += static_cast<double>(p[i] - '0') * mul;
            mul *= 0.1;
        }
    }

    // Negate value if there is a leading minus.
    if (negative) out *= -1;

    return out;
}

static int HexDigit(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static int ReadHexColor(uint8_t* out, const uint8_t* p, int n) {
    int numComponents = 0;
    switch (n) {
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

static void ReadMarkupColor(uint32_t& out, const uint8_t* param, int len,
                            uint32_t base) {
    uint8_t channels[4];
    switch (ReadHexColor(channels, param, len)) {
        case 2:
            out = RGBAtoColor32(channels[0], channels[1], channels[2],
                                channels[3]);
            break;
        case 1:
            out = RGBAtoColor32(channels[0], channels[1], channels[2], 0) |
                  (base & kColor32_AlphaMask);
            break;
        case 0:
            out = base;
            break;
    };
}

static void ReadTextColor(const uint8_t* param, int len) {
    ReadMarkupColor(LD->textColor, param, len, LD->baseTextColor);
    auto& item = LD->markup.append();
    item.type = LMarkup::SET_TEXT_COLOR;
    item.glyphIndex = LD->glyphs.size();
    item.setTextColor = LD->textColor;
}

static void ReadShadowColor(const uint8_t* param, int len) {
    ReadMarkupColor(LD->shadowColor, param, len, LD->baseShadowColor);
    auto& item = LD->markup.append();
    item.type = LMarkup::SET_SHADOW_COLOR;
    item.glyphIndex = LD->glyphs.size();
    item.setShadowColor = LD->shadowColor;
}

static void ReadFontSize(const uint8_t* param, int len) {
    int fontSize = static_cast<int>(ReadNumber(param, len) + 0.5);

    if (len == 0) {
        fontSize = LD->baseFontSize;
    } else if (param[len - 1] == '%') {
        fontSize =
            static_cast<int>(ReadNumber(param, len) *
                                 static_cast<double>(LD->baseFontSize) / 100.0 +
                             0.5);
    } else {
        fontSize = static_cast<int>(ReadNumber(param, len) + 0.5);
    }
    LD->fontSize = min(max(1, fontSize), 256);
    SetLineMetrics();
}

static void ReadFontChange(const uint8_t* param, int len) {
    // Resolve the path of the font file (could be an asset name).
    std::string path(reinterpret_cast<const char*>(param), len);

    // Look for the font in the font manager.
    FontData* font = FontManager::find(path);
    LD->font = font ? font : LD->baseFont;

    SetLineMetrics();
}

static void ReadFgColor(const uint8_t* param, int len) {
    if (LD->fgQuad.enabled) {
        auto& quad = LD->fgQuads.append();
        quad.color = LD->fgQuad.color;
        quad.line = LD->lineIndex;
        quad.x = LD->fgQuad.x;
        quad.w = LD->lineW - LD->fgQuad.x;
        LD->fgQuad.enabled = false;
    }
    union {
        uint32_t color;
        uint8_t channels[4];
    };
    if (ReadHexColor(channels, param, len)) {
        LD->fgQuad.enabled = true;
        LD->fgQuad.color = color;
        LD->fgQuad.x = LD->lineW;
    }
}

static void ReadBgColor(const uint8_t* param, int len) {
    if (LD->bgQuad.enabled) {
        auto& quad = LD->bgQuads.append();
        quad.color = LD->bgQuad.color;
        quad.line = LD->lineIndex;
        quad.x = LD->bgQuad.x;
        quad.w = LD->lineW - LD->bgQuad.x;
        LD->bgQuad.enabled = false;
    }
    union {
        uint32_t color;
        uint8_t channels[4];
    };
    if (ReadHexColor(channels, param, len)) {
        LD->bgQuad.enabled = true;
        LD->bgQuad.color = color;
        LD->bgQuad.x = LD->lineW;
    }
}

static const Glyph* ReadMarkup(const uint8_t* str) {
    // Keep reading tags until a glyph is returned.
    const uint8_t* p = str + LD->nextCharIndex;
    while (p[0] == '{') {
        LD->charIndex = LD->nextCharIndex;

        // A leading slash indicates undo.
        bool undo = (p[1] == '/');
        if (undo) ++p;

        // Read the tag name.
        const uint8_t* name = ++p;
        while (*p && *p != '}' && *p != ':') ++p;
        int nameLen = p - name;
        if (*p == ':') ++p;

        // Read the tag parameters.
        const uint8_t* param = p;
        while (*p && *p != '}') ++p;
        int paramLen = p - param;
        if (*p == '}') ++p;

        // Advance the read position to the tag end.
        LD->nextCharIndex = p - str;

        // Turn the name into a 64-bit identifier.
        uint64_t id = 0;
        for (int i = 0; i < nameLen; ++i) {
            id = (id << 8) | toupper(name[i]);
        }

        // Perform the action associated with the identifier.
        switch (id) {
            case ID1('N'):
                return GetGlyph('\n');
            case ID1('T'):
                return GetGlyph('\t');
            case ID1('F'):
                ReadFontChange(param, paramLen);
                break;
            case ID1('G'):
                return FontManager::getGlyph(std::string(
                    reinterpret_cast<const char*>(param), paramLen));
            case ID2('L', 'B'):
                return GetGlyph('{');
            case ID2('R', 'B'):
                return GetGlyph('}');
            case ID2('F', 'C'):
                ReadFgColor(param, paramLen);
                break;
            case ID2('B', 'C'):
                ReadBgColor(param, paramLen);
                break;
            case ID2('S', 'C'):
                ReadShadowColor(param, paramLen);
                break;
            case ID2('T', 'C'):
                ReadTextColor(param, paramLen);
                break;
            case ID2('T', 'S'):
                ReadFontSize(param, paramLen);
                break;
        };
    }

    // The markup tags did not return a glyph, continue reading.
    return nullptr;
}

// ================================================================================================
// Text layout functions

static const Glyph* GetGlyph(int charcode) {
    const Glyph* glyph = nullptr;

    // First check if it is already available in the glyph cache of the current
    // font.
    if (LD->font) glyph = LD->font->getGlyph(LD->fontSize, charcode);

    // If the current LD->font does not support the glyph, try the fallback
    // fonts.
    if (!glyph) {
        FontData* fallback = FontManager::fallback();
        while (fallback && !glyph) {
            glyph = fallback->getGlyph(LD->fontSize, charcode);
            fallback = fallback->next;
        }
    }

    // None of the fallback fonts support the character, return a placeholder
    // glyph.
    if (!glyph) glyph = &FontManager::getPlaceholderGlyph(LD->fontSize);

    return glyph;
}

static void SetLineMetrics() {
    LD->lineTop = min(LD->lineTop, -LD->fontSize);
    LD->lineBottom = max(LD->lineBottom, LD->fontSize / 4);
}

static const Glyph* ReadGlyph(const uint8_t* str) {
    int charcode = str[LD->nextCharIndex];

    // Check if the current character is the start of a markup tag.
    if (charcode == '{' && (LD->flags & Text::MARKUP_READ)) {
        auto* glyph = ReadMarkup(str);
        if (glyph) return glyph;
        charcode = str[LD->nextCharIndex];
    }

    // Check if we reached the end of the string.
    if (charcode == 0) return nullptr;

    // Advance to the next character.
    LD->charIndex = LD->nextCharIndex;
    ++LD->nextCharIndex;

    // Keep reading if the current character is multibyte UTF-8.
    if (charcode >= 0x80) {
        const uint8_t* p = str + LD->nextCharIndex;
        int tb = utf8TrailingBytes[charcode];
        for (int i = 0; i < tb && *p; ++i, ++p) charcode = (charcode << 6) + *p;
        charcode -= utf8MultibyteResidu[tb];
        LD->nextCharIndex = p - str;
    }

    return GetGlyph(charcode);
}

static void AddEllipsesToLine() {
    const Glyph* ellipsesGlyph = GetGlyph('.');
    int ellipsesW = ellipsesGlyph->advance * 3;
    int maxLineW = max(0, LD->maxLineW - ellipsesW);

    // In order to do ellipses, the line must contain atleast one glyph.
    if (LD->lineBeginGlyph == LD->glyphs.size()) return;
    auto* glyphs = LD->glyphs.begin();
    int firstGlyph = LD->lineBeginGlyph;
    int lastGlyph = LD->glyphs.size() - 1;
    int lineEndCharIndex = glyphs[firstGlyph].charIndex;

    // Omit all glyphs that are past the maximum line width.
    while (lastGlyph >= firstGlyph &&
           glyphs[lastGlyph].x + glyphs[lastGlyph].glyph->advance > maxLineW) {
        lineEndCharIndex = glyphs[lastGlyph].charIndex;
        --lastGlyph;
    }

    // We don't put ellipses after whitespace, so omit trailing whitespace as
    // well.
    while (lastGlyph >= firstGlyph && glyphs[lastGlyph].glyph->isWhitespace) {
        lineEndCharIndex = glyphs[lastGlyph].charIndex;
        --lastGlyph;
    }

    // Determine the new line width.
    if (lastGlyph >= 0) {
        LD->lineW = glyphs[lastGlyph].x + glyphs[lastGlyph].glyph->advance;
    } else {
        LD->lineW = 0;
    }

    // Clamp foreground/background quads to the new line width.
    for (auto& quad : LD->fgQuads) {
        if (quad.line == LD->lineIndex) {
            quad.w = min(quad.w, max(0, LD->lineW - quad.x));
        }
    }
    for (auto& quad : LD->bgQuads) {
        if (quad.line == LD->lineIndex) {
            quad.w = min(quad.w, max(0, LD->lineW - quad.x));
        }
    }

    // Append three ellipses glyphs after the last glyph.
    LD->glyphs.resize(lastGlyph + 1, LGlyph());
    if (ellipsesW < LD->maxLineW) {
        for (int i = 0; i < 3; ++i) {
            auto& item = LD->glyphs.append();
            item.glyph = ellipsesGlyph;
            item.x = LD->lineW;
            item.charIndex = lineEndCharIndex;
            LD->lineW += ellipsesGlyph->advance;
        }
    }

    // Make sure all markup from the omitted part is applied after the ellipses.
    int postEllipsesIndex = LD->glyphs.size();
    auto* markup = LD->markup.begin();
    int markupIndex = LD->markup.size() - 1;
    while (markupIndex >= 0 && markup[markupIndex].glyphIndex > lastGlyph) {
        markup[markupIndex].glyphIndex = postEllipsesIndex;
        --markupIndex;
    }
}

static void FinishCurrentLine(bool last) {
    // Finish the current foreground quad.
    if (LD->fgQuad.enabled && LD->lineW > LD->fgQuad.x) {
        auto& quad = LD->fgQuads.append();
        quad.color = LD->fgQuad.color;
        quad.line = LD->lineIndex;
        quad.x = LD->fgQuad.x;
        quad.w = LD->lineW - LD->fgQuad.x;
        LD->fgQuad.x = 0;
    }

    // Finish the current background quad.
    if (LD->fgQuad.enabled && LD->lineW > LD->fgQuad.x) {
        auto& quad = LD->bgQuads.append();
        quad.color = LD->fgQuad.color;
        quad.line = LD->lineIndex;
        quad.x = LD->fgQuad.x;
        quad.w = LD->lineW - LD->fgQuad.x;
        LD->fgQuad.x = 0;
    }

    // Add ellipses if necessary.
    if ((LD->flags & Text::ELLIPSES) && LD->lineW > LD->maxLineW) {
        AddEllipsesToLine();
    }

    // Calculate the y-value of the baseline.
    int lineY = -LD->lineTop;
    if (LD->lines.size()) {
        auto& previousLine = LD->lines.back();
        lineY += previousLine.y + previousLine.bottom;
    }

    // Store the current line info.
    auto& line = LD->lines.append();
    line.beginGlyph = LD->lineBeginGlyph;
    line.endGlyph = LD->glyphs.size();
    line.x = 0;
    line.y = lineY;
    line.w = LD->lineW;
    line.top = LD->lineTop;
    line.bottom = LD->lineBottom;

    // Update the size of the text area.
    LD->textW = max(LD->textW, LD->lineW);
    LD->textH = lineY + LD->lineBottom;

    // advance to the next line.
    LD->lineBeginGlyph = LD->glyphs.size();
    LD->previousGlyph = nullptr;
    LD->lineW = 0;
    ++LD->lineIndex;
}

static void ClearLayout() {
    LD->charIndex = 0;
    LD->nextCharIndex = 0;
    LD->previousGlyph = nullptr;

    LD->fgQuads.clear();
    LD->fgQuad.enabled = false;
    LD->bgQuads.clear();
    LD->fgQuad.enabled = false;

    LD->lines.clear();
    LD->glyphs.clear();
    LD->markup.clear();

    LD->lineIndex = 0;
    LD->lineBeginGlyph = 0;

    LD->lineW = 0;
    LD->lineTop = 0;
    LD->lineBottom = 0;

    LD->textW = 0;
    LD->textH = 0;
}

static void CreateLayout(const char* str) {
    // Make a list of every line of glyphs.
    bool isMultiline = !(LD->flags & Text::SINGLE_LINE);
    while (true) {
        auto glyph = ReadGlyph(reinterpret_cast<const uint8_t*>(str));
        if (!glyph) break;

        // Apply glyph advance.
        if ((LD->flags & Text::KERNING) && LD->previousGlyph) {
            if (glyph->index && LD->previousGlyph->index) {
                int dx = glyph->font->getKerning(LD->previousGlyph, glyph);
                LD->lineW += dx;
            }
        }

        // Insert the glyph in the list.
        auto& item = LD->glyphs.append();
        item.glyph = glyph;
        item.x = LD->lineW;
        item.charIndex = LD->charIndex;

        // Check if we are forced to break the line and continue on a new line.
        if (glyph->isNewline && isMultiline) {
            FinishCurrentLine(false);
        } else {
            LD->previousGlyph = glyph;
            LD->lineW += glyph->advance;
        }
    }

    // Finally, add left over characters to the last line.
    if (LD->glyphs.size() > LD->lineBeginGlyph && LD->previousGlyph) {
        FinishCurrentLine(true);
    }
}

static void AlignText() {
    // Apply horizontal alignments (left = 0, center = 1, right = 2).
    static const int alignH[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    switch (alignH[LD->align]) {
        case 1:
            for (auto& line : LD->lines) line.x -= line.w / 2;
            break;
        case 2:
            for (auto& line : LD->lines) line.x -= line.w;
            break;
    };

    // Apply vertical alignments (top = 0, middle = 1, bottom = 2).
    static const int alignV[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    switch (alignV[LD->align]) {
        case 1:
            for (auto& line : LD->lines) line.y -= LD->textH / 2;
            break;
        case 2:
            for (auto& line : LD->lines) line.y -= LD->textH;
            break;
    };
}

static vec2i ArrangeText(const TextStyle& style, int maxLineWidth,
                         Text::Align align, const char* str) {
    LD->style = style;
    LD->flags = style.textFlags;
    LD->align = align;

    LD->font = LD->baseFont = static_cast<FontData*>(style.font.data());
    LD->fontSize = LD->baseFontSize = style.fontSize;

    LD->baseTextColor = LD->textColor = style.textColor;
    LD->baseShadowColor = LD->shadowColor = style.shadowColor;

    LD->maxLineW = maxLineWidth;

    if (maxLineWidth == NO_MAX_LINE_WIDTH) {
        LD->flags &= ~Text::ELLIPSES;
    }

    LD->stringLength = strlen(str);

    ClearLayout();
    SetLineMetrics();
    CreateLayout(str);
    AlignText();

    return {LD->textW, LD->textH};
}

// ================================================================================================
// TextStyle

TextStyle::TextStyle()
    : fontSize(12),
      textFlags(0),
      textColor(Colors::white),
      shadowColor(Colors::black) {
    if (LD) *this = LD->defaultStyle;
}

void TextStyle::makeDefault() {
    if (LD) LD->defaultStyle = *this;
}

// ================================================================================================
// TextLayout API functions.

void TextLayout::create() {
    LD = new LLayout;
    LD->fgQuad.enabled = false;
    LD->fgQuad.enabled = false;
}

void TextLayout::destroy() {
    delete LD;
    LD = nullptr;
}

void TextLayout::endFrame() { LD->lines.clear(); }

LLayout& TextLayout::get() { return *LD; }

vec2i TextLayout::getTextPos(recti r) {
    int x = r.x, y = r.y, w = r.w, h = r.h;
    switch (LD->align) {
        case Text::TL:
            return {x, y};
        case Text::TC:
            return {x + w / 2, y};
        case Text::TR:
            return {x + w, y};
        case Text::ML:
            return {x, y + h / 2};
        case Text::MC:
            return {x + w / 2, y + h / 2};
        case Text::MR:
            return {x + w, y + h / 2};
        case Text::BL:
            return {x, y + h};
        case Text::BC:
            return {x + w / 2, y + h};
        case Text::BR:
            return {x + w, y + h};
    };
    return {x, y};
}

recti TextLayout::getTextBB(vec2i p) {
    int x = p.x, y = p.y, w = LD->textW, h = LD->textH;
    switch (LD->align) {
        case Text::TL:
            return {x, y, w, h};
        case Text::TC:
            return {x - w / 2, y, w, h};
        case Text::TR:
            return {x - w, y, w, h};
        case Text::ML:
            return {x, y - h / 2, w, h};
        case Text::MC:
            return {x - w / 2, y - h / 2, w, h};
        case Text::MR:
            return {x - w, y - h / 2, w, h};
        case Text::BL:
            return {x, y - h, w, h};
        case Text::BC:
            return {x - w / 2, y - h, w, h};
        case Text::BR:
            return {x - w, y - h, w, h};
    };
    return {x, y, w, h};
}

// ================================================================================================
// Text API functions.

vec2i Text::arrange(Text::Align align, const char* text) {
    return ArrangeText(LD->defaultStyle, NO_MAX_LINE_WIDTH, align, text);
}

vec2i Text::arrange(Text::Align align, int maxLineWidth, const char* text) {
    return ArrangeText(LD->defaultStyle, max(maxLineWidth, 0), align, text);
}

vec2i Text::arrange(Text::Align align, const TextStyle& style,
                    const char* text) {
    return ArrangeText(style, NO_MAX_LINE_WIDTH, align, text);
}

vec2i Text::arrange(Text::Align align, const TextStyle& style, int maxLineWidth,
                    const char* text) {
    return ArrangeText(style, max(maxLineWidth, 0), align, text);
}

vec2i Text::getSize() { return {LD->textW, LD->textH}; }

int Text::getWidth() { return LD->textW; }

recti Text::getBoundingBox(recti textBox) {
    return TextLayout::getTextBB(TextLayout::getTextPos(textBox));
}

recti Text::getBoundingBox(vec2i textPos) {
    return TextLayout::getTextBB(textPos);
}

int Text::getCharIndex(recti textBox, vec2i cursorPos) {
    return getCharIndex(TextLayout::getTextPos(textBox), cursorPos);
}

int Text::getCharIndex(vec2i textPos, vec2i cursorPos) {
    const LLine* line = LD->lines.begin();
    const LLine* lineEnd = LD->lines.end();

    cursorPos.x -= textPos.x;
    cursorPos.y -= textPos.y;

    // If there are no glyphs, we return the start of the string.
    if (line == lineEnd) return 0;

    // If charPos is above the first line, we return the start of the string.
    if (cursorPos.y < line->y + line->top) return 0;

    // Advance to the first line which overlaps with the y-position.
    while (line != lineEnd && cursorPos.y > line->y + line->bottom) ++line;

    // If charPos is below the final line, we return the end of the string.
    if (line == lineEnd) return LD->stringLength;

    // Otherwise, we look for the closest character on the current line.
    int lastPos = line->beginGlyph;
    auto* begin = LD->glyphs.begin() + line->beginGlyph;
    auto* end = LD->glyphs.begin() + line->endGlyph;
    for (auto item = begin; item != end; ++item) {
        int glyphCenterX = line->x + item->x + item->glyph->advance / 2;
        if (glyphCenterX > cursorPos.x) return item->charIndex;
        lastPos = item->charIndex;
    }

    // If we reach this point the x-position is after the end of the line.
    return LD->stringLength;
}

Text::CursorPos Text::getCursorPos(recti textBox, int charIndex) {
    return getCursorPos(TextLayout::getTextPos(textBox), charIndex);
}

Text::CursorPos Text::getCursorPos(vec2i textPos, int charIndex) {
    // If there are no glyphs, we return immediately.
    if (LD->lines.empty()) {
        vec2i pos =
            TextLayout::getTextPos({0, 0, 0, LD->lineBottom - LD->lineTop});
        return {textPos.x, textPos.y - pos.y, LD->lineBottom - LD->lineTop};
    }

    // Return the rect of the first glyph on or after index.
    for (auto& line : LD->lines) {
        auto* begin = LD->glyphs.begin() + line.beginGlyph;
        auto* end = LD->glyphs.begin() + line.endGlyph;
        for (auto item = begin; item != end; ++item) {
            if (item->charIndex >= charIndex) {
                int x = textPos.x + line.x + item->x;
                int y = textPos.y + line.y + line.top;
                return {x, y, line.bottom - line.top};
            }
        }
    }

    // The index is after the last glyph on the last line.
    const auto& line = LD->lines.back();
    return {textPos.x + line.x + line.w, textPos.y + line.y + line.top,
            line.bottom - line.top};
}

std::string Text::escapeMarkup(const char* str) {
    std::string out;
    for (; *str; ++str) {
        if (*str == '{') {
            Str::append(out, "{lb}", 4);
        } else {
            Str::append(out, *str);
        }
    }
    return out;
}

int Text::getEscapedCharIndex(const char* str, int index) {
    int offset = index;
    for (int i = 0; i < index && str[i]; ++i) {
        if (str[i] == '{') offset += 3;
    }
    return offset;
}

};  // namespace Vortex