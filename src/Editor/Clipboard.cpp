#include <Editor/Clipboard.h>

#include <Core/StringUtils.h>

#include <Managers/TempoMan.h>
#include <Managers/NoteMan.h>

#include <System/System.h>

namespace Vortex {

static const char* tag = "ArrowVortex:";
static const uint8_t version = 2;
static const uint8_t tagLength = strlen(tag);

// ================================================================================================
// Clipboard functions.

bool HasClipboardData() {
    std::string text = gSystem->getClipboardText();
    return Str::startsWith(text, tag) && text.length() > tagLength;
}

void SetClipboardData(std::string& data) {
    std::string text = "ArrowVortex:ver:" + Str::val(version) + ":" + data;
    gSystem->setClipboardText(text);
}

ClipboardData GetClipboardData() {
    ClipboardData clip;

    std::string text = gSystem->getClipboardText();

    if (Str::startsWith(text, tag)) {
        auto dem = Str::endsWith(text, ":") ? 1 : 0;

        text = Str::substr(text, tagLength, text.length() - tagLength - dem);
        if (text.length() == 0) return clip;

        auto pairs = Str::split(text, ":", false, false);

        // Parse "key:data" pairs.
        for (size_t i = 0; i + 1 < pairs.size(); i += 2) {
            std::string& key = pairs[i];
            std::string& value = pairs[i + 1];

            if (key == "ver")
                clip.version = Str::readInt(value);

            else if (key == NotesMan::clipboardTag)
                Base64Decode(clip.notes, value.c_str());

            else if (key == TempoMan::clipboardTag)
                Base64Decode(clip.tempos, value.c_str());
        }
    }

    return clip;
}

// ================================================================================================
// Encoding & Decoding functions.

static const char base64Table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static const int8_t base64TableDecode[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 64, 64, 64, 65, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64,
    64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64};

void Base64Encode(std::string& out, const uint8_t* in, int size) {
    int i = 0;
    out.push_back(':');
    while (i < size) {
        uint32_t v = (in[i] << 16) | ((i + 1 < size ? in[i + 1] : 0) << 8) |
                     ((i + 2 < size ? in[i + 2] : 0));

        // Compression - 3 chars -> 1 char
        if (v == 0 && i + 2 < size) {
            out.push_back('-');
            i += 3;
            continue;
        }

        out.push_back(base64Table[(v >> 18) & 63]);
        out.push_back(base64Table[(v >> 12) & 63]);
        out.push_back(i + 1 < size ? base64Table[(v >> 6) & 63] : '=');
        out.push_back(i + 2 < size ? base64Table[v & 63] : '=');

        i += 3;
    }
    out.push_back(':');
}

void Base64Decode(Vector<uint8_t>& out, const char* in) {
    uint32_t v = 0;
    int valb = -8;

    for (const char* p = in; *p; p++) {
        // Compression - 1 char -> 3 chars
        if (*p == '-') {
            out.push_back(0);
            out.push_back(0);
            out.push_back(0);
            continue;
        }

        uint8_t d = base64TableDecode[static_cast<uint8_t>(*p)];
        if (d == 64) continue;
        if (d == 65) break;
        v = (v << 6) | d;
        valb += 6;
        if (valb >= 0) {
            out.push_back((v >> valb) & 0xFF);
            valb -= 8;
        }
    }
}
}  // namespace Vortex