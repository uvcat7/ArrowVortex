#include <Editor/Common.h>

#include <stdio.h>
#include <stdarg.h>

#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <System/System.h>

#include <Editor/TextOverlay.h>

namespace Vortex {

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
                map[i] = static_cast<RowType>(map[i] + 1);
            }
        }
    }
    return map[rowIndex % 192];
}

uint32_t ToRowTypeColor(RowType type) { return ROW_TYPE_COLOR[type]; }
uint32_t ToRowTypeColor(int type) { return ROW_TYPE_COLOR[type]; }

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
