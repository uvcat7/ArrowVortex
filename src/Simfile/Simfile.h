#pragma once

#include <vector>
#include <Core/NonCopyable.h>

#include <Simfile/Common.h>

namespace Vortex {

/// Formats from which the simfile can be loaded/saved.
enum SimFormat {
    SIM_NONE,

    SIM_SM,
    SIM_SSC,
    SIM_OSU,
    SIM_DWI,
    SIM_QUA,

    NUM_SIMFILE_FORMATS
};

/// Hold data for a foreground/background change.
struct BgChange {
    std::string effect;
    std::string file, file2;
    std::string color, color2;
    std::string transition;
    double startBeat;
    double rate;
};

/// Holds the simfile metadata.
struct Simfile : NonCopyable {
    Simfile();
    ~Simfile();

    void sanitize();

    std::vector<Chart*> charts;
    Tempo* tempo;

    std::string dir;
    std::string file;
    SimFormat format = SIM_NONE;

    std::string title, titleTr;
    std::string subtitle, subtitleTr;
    std::string artist, artistTr;
    std::string genre;
    std::string credit;

    std::string music;
    std::string banner;
    std::string background;
    std::string cdTitle;
    std::string lyricsPath;

    std::vector<BgChange> fgChanges;
    std::vector<BgChange> bgChanges[2];

    double previewStart = 0.0;
    double previewLength = 0.0;

    bool isSelectable = false;
};

// Returns the abbreviation of the given simfile format.
const char* GetFormatAbbreviation(SimFormat format);

};  // namespace Vortex
