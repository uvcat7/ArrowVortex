#include <Simfile/Parsing.h>

#include <Core/StringUtils.h>

#include <System/File.h>
#include <System/Debug.h>

#include <Simfile/Parsing.h>
#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>

#include <Managers/StyleMan.h>

#include <algorithm>

namespace Vortex {

// ===================================================================================
// External load and save functions.

#define LOAD_ARGS const std::string &path, Simfile *sim
#define SAVE_ARGS const Simfile *sim, bool backup

namespace Sm {
bool LoadSm(LOAD_ARGS);   // Defined in LoadSm.cpp
bool SaveSm(SAVE_ARGS);   // Defined in SaveSm.cpp
bool SaveSsc(SAVE_ARGS);  // Defined in SaveSm.cpp
};  // namespace Sm
namespace Osu {
bool LoadOsu(LOAD_ARGS);  // Defined in LoadOsu.cpp
bool SaveOsu(SAVE_ARGS);  // Defined in SaveOsu.cpp
};  // namespace Osu
namespace Dwi {
bool LoadDwi(LOAD_ARGS);  // Defined in LoadDwi.cpp
};

// ================================================================================================
// Parsing utilities.

bool ParseSimfile(std::string& out, const std::string& path) {
    bool success;
    out = File::getText(path, &success);
    if (!success) return false;

    // Preprocess the file contents.
    const char* read = &out[0];
    char* write = &out[0];
    while (*read) {
        if (read[0] == '/' && read[1] == '/') {
            while (*read && *read != '\n') ++read;
        } else if (*read == '\r') {
            ++read;
        } else if (*read == '\t') {
            *write = ' ';
            ++read, ++write;
        } else {
            *write = *read;
            ++read, ++write;
        }
    }
    Str::truncate(out, static_cast<int>(write - &out[0]));

    return true;
}

static char* ZeroTerminateItem(char* start, char* end) {
    while (end != start && (end[-1] == ' ' || end[-1] == '\n')) --end;
    *end = 0;
    return start;
}

bool ParseNextTag(char*& p, char*& outTag, char*& outVal) {
    while (*p && *p != '#') ++p;
    if (*p == 0) return false;
    outTag = ++p;

    while (*p && *p != ':') ++p;
    if (*p) *p++ = 0;

    outVal = p;
    // Allow : and ; to be escaped
    while (*p && (*p != ';' || *(p - 1) == '\\') &&
           !(p[0] == '\n' && p[1] == '#'))
        ++p;
    if (*p) *p++ = 0;

    return true;
}

bool ParseNextItem(char*& p, char*& outVal, char seperator) {
    while (*p == ' ' || *p == '\n') ++p;
    if (*p == 0) return false;

    char* start = p;
    while (*p && *p != seperator) ++p;
    char* end = p;

    if (*p == seperator) ++p;

    outVal = ZeroTerminateItem(start, end);

    return true;
}

bool ParseNextItem(char*& p, char** outVals, int numVals, char setSeperator,
                   char valSeperator) {
    while (*p == ' ' || *p == '\n') ++p;
    if (*p == 0) return false;

    bool setEnded = false;
    for (int i = 0; i < numVals; ++i) {
        char* start = p;
        while (*p && *p != setSeperator && *p != valSeperator) ++p;
        char end = *p;

        outVals[i] = ZeroTerminateItem(start, p);

        if (end == valSeperator) {
            ++p;
            while (*p == ' ' || *p == '\n') ++p;
        } else {
            for (++i; i < numVals; ++i) outVals[i] = p;
            if (end == setSeperator) *p++ = 0;
            setEnded = true;
        }
    }
    if (!setEnded) {
        while (*p && *p != setSeperator) ++p;
        if (*p == setSeperator) ++p;
    }

    return true;
}

bool ParseBool(const char* str, bool& outVal) {
    outVal = !Str::iequal(str, "NO");
    return true;
}

bool ParseVal(const char* str, double& outVal) {
    char* end;
    double v = strtod(str, &end);
    if (v == 0 && (*str == 0 || *end != 0)) return false;
    outVal = v;
    return true;
}

bool ParseVal(const char* str, int& outInt) {
    char* end;
    int v = strtol(str, &end, 10);
    if (v == 0 && (*str == 0 || *end != 0)) return false;
    outInt = v;
    return true;
}

bool ParseBeat(const char* str, int& outRow) {
    double beat;
    if (!ParseVal(str, beat)) return false;
    outRow = (int)(beat * 48 + 0.5);
    return true;
}

// ================================================================================================
// Simfile importing and exporting.

static void ClearSimfile(Simfile& sim, Path& path) {
    sim.~Simfile();
    new (&sim) Simfile();
    sim.dir = path.dir();
    sim.file = path.name();
}

bool LoadSimfile(Simfile& sim, const std::string& path) {
    // Store the song directory, filename and extension.
    Path filePath = path;
    ClearSimfile(sim, filePath);

    // Call the load function associated with the extension.
    bool success = false;
    std::string ext = filePath.ext();
    Str::toLower(ext);
    if (ext == "sm" || ext == "ssc") {
        success = Sm::LoadSm(filePath, &sim);
    } else if (ext == "dwi") {
        success = Dwi::LoadDwi(filePath, &sim);
    } else if (ext == "osu") {
        success = Osu::LoadOsu(filePath, &sim);
    } else {
        Debug::blockBegin(Debug::ERROR, "could not load sim");
        Debug::log("file: %s\n", path.c_str());
        Debug::log("reason: unknown sim format\n");
        Debug::blockEnd();
    }
    if (!success) return false;

    return true;
}

bool SaveSimfile(const Simfile& sim, SimFormat format, bool backup) {
    switch (format) {
        case SIM_SM:
            return Sm::SaveSm(&sim, backup);
        case SIM_SSC:
            return Sm::SaveSsc(&sim, backup);
        case SIM_OSU:
            return Osu::SaveOsu(&sim, backup);
    };
    return false;
}

};  // namespace Vortex
