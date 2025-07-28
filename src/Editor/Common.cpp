#include <Editor/Common.h>

#include <stdio.h>
#include <stdarg.h>

#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <System/Debug.h>
#include <System/System.h>

#include <Editor/TextOverlay.h>

namespace Vortex {

// ================================================================================================
// Clipboard functions.

static const int a85shift[4] = {24, 16, 8, 0};
static const uint32_t a85div[5] = {85 * 85 * 85 * 85, 85 * 85 * 85, 85 * 85, 85,
                                   1};

static void Ascii85enc(std::string& out, const uint8_t* in, int size) {
    // Convert to 32-bit integers (big endian) and pad with zeroes.
    Vector<uint32_t> blocks((size + 3) / 4, 0);
    for (int i = 0; i < size; ++i) {
        blocks[i / 4] += in[i] << a85shift[i & 3];
    }

    // Encode 32-bit integers to base 85.
    for (int i = 0; i != blocks.size(); ++i) {
        if (blocks[i] == 0 && i != blocks.size() - 1) {
            Str::append(out, 'z');
        } else
            for (int j = 0; j != 5; ++j) {
                Str::append(out, 33 + (blocks[i] / a85div[j]) % 85);
            }
    }

    // Remove padded bytes from output.
    int padding = blocks.size() * 4 - size;
    Str::truncate(out, out.length() - padding);
}

static void Ascii85dec(Vector<uint8_t>& out, const char* in) {
    // Convert from base 85 to 32-bit integers.
    int padding = 0;
    while (*in) {
        if (*in == 'z') {
            out.resize(out.size() + 4, 0);
            ++in;
        } else {
            int v = 0, i = 0;
            for (; i != 5 && *in; ++i) {
                v = v * 85 + (*in++) - 33;
            }
            for (; i != 5; ++i, ++padding) {
                v = v * 85 + 'u' - 33;
            }
            for (i = 0; i != 4; ++i) {
                out.push_back((v >> a85shift[i]) & 0xFF);
            }
        }
    }

    // Remove padded bytes from output.
    out.resize(out.size() - padding);
}

bool HasClipboardData(const std::string& tag) {
    std::string text = gSystem->getClipboardText();
    std::string prefix = "ArrowVortex:" + tag + ":";
    return Str::startsWith(text, prefix.c_str());
}

void SetClipboardData(const std::string& tag, const uint8_t* data, int size) {
    std::string text = "ArrowVortex:" + tag + ":";
    Ascii85enc(text, data, size);
    gSystem->setClipboardText(text);
}

Vector<uint8_t> GetClipboardData(const std::string& tag) {
    Vector<uint8_t> buffer;
    std::string text = gSystem->getClipboardText();
    std::string prefix = "ArrowVortex:" + tag + ":";
    if (Str::startsWith(text, prefix.c_str())) {
        Ascii85dec(buffer, text.data() + prefix.length());
    }
    return buffer;
}

// ================================================================================================
// Utility functions.

uint32_t ToColor(Difficulty dt) {
    switch (dt) {
        case DIFF_BEGINNER:
            return RGBAtoColor32(16, 222, 255, 255);
        case DIFF_EASY:
            return RGBAtoColor32(99, 220, 99, 255);
        case DIFF_MEDIUM:
            return RGBAtoColor32(255, 228, 98, 255);
        case DIFF_HARD:
            return RGBAtoColor32(255, 98, 97, 255);
        case DIFF_CHALLENGE:
            return RGBAtoColor32(109, 142, 210, 255);
    };
    return RGBAtoColor32(180, 183, 186, 255);
}

const char* ToString(SnapType st) {
    static const char* text[NUM_SNAP_TYPES] = {
        "None", "4th",  "8th",  "12th", "16th",  "24th",
        "32nd", "48th", "64th", "96th", "192nd", "Custom"};
    return (st >= 0 && st < NUM_SNAP_TYPES) ? text[st] : text[0];
}

const std::string OrdinalSuffix(int i) {
    int j = i % 10, k = i % 100;

    std::string out = Str::val(i, 0);

    if (j == 1 && k != 11) {
        out = out + "st";
    } else if (j == 2 && k != 12) {
        out = out + "nd";
    } else if (j == 3 && k != 13) {
        out = out + "rd";
    } else {
        out = out + "th";
    }

    return out;
}

RowType ToRowType(int rowIndex) {
    static RowType map[192] = {};
    static bool init = false;
    if (!init) {
        init = true;
        int mod[8] = {48, 24, 16, 12, 8, 6, 4, 3};
        for (int i = 0; i < 192; ++i) {
            for (int j = 0; j < 8 && i % mod[j] != 0; ++j) {
                map[i] = (RowType)(map[i] + 1);
            }
        }
    }
    return map[rowIndex % 192];
}

// ================================================================================================
// Hud message functions.

#define PRINT_TO_BUFFER                          \
    char buffer[512];                            \
    va_list args;                                \
    va_start(args, fmt);                         \
    int len = vsnprintf(buffer, 511, fmt, args); \
    if (len < 0 || len > 511) len = 511;         \
    buffer[len] = 0;                             \
    va_end(args);

void HudNote(const char* fmt, ...) {
    PRINT_TO_BUFFER;
    if (gTextOverlay) gTextOverlay->addMessage(buffer, TextOverlay::NOTE);
}

void HudInfo(const char* fmt, ...) {
    PRINT_TO_BUFFER;
    if (gTextOverlay) gTextOverlay->addMessage(buffer, TextOverlay::INFO);
}

void HudWarning(const char* fmt, ...) {
    PRINT_TO_BUFFER;
    if (gTextOverlay) gTextOverlay->addMessage(buffer, TextOverlay::WARNING);
}

void HudError(const char* fmt, ...) {
    PRINT_TO_BUFFER;
    if (gTextOverlay) gTextOverlay->addMessage(buffer, TextOverlay::ERROR);
}

};  // namespace Vortex
