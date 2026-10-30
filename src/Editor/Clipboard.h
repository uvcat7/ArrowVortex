#pragma once

#include <vector>

#include <Simfile/Common.h>

namespace Vortex {

struct ClipboardData {
    uint8_t version = 0;
    uint8_t count = 0;
    std::vector<uint8_t> notes;
    std::vector<uint8_t> tempos;
};

// Returns true clipboard has ArrowVortex data.
bool HasClipboardData();

// Encodes the given data to an Base64- string and sends it to the clipboard.
void SetClipboardData(std::string& data);

// Reads a string from the clipboard and decodes it using Base64-.
ClipboardData GetClipboardData();

// Encodes StreamData into Base64-.
// This prepends/appends ":" before and after the encoded data.
// Slight compression is done by replacing "AAA" instances with `-`.
void Base64Encode(std::string& out, const uint8_t* in, int size);

// Decodes Base64- data into StreamData.
void Base64Decode(std::vector<uint8_t>& out, const char* in);

}  // namespace Vortex