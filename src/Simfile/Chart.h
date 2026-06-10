#pragma once

#include <vector>
#include <Core/NonCopyable.h>

#include <Simfile/NoteList.h>
#include <Simfile/Common.h>

namespace Vortex {

/// Holds data for a chart.
struct Chart : NonCopyable {
    Chart();
    ~Chart();

    // Returns the difficulty and meter of the chart (e.g. "Challenge 12").
    std::string description() const;

    // Returns true if the chart has split timing, false if the chart uses song
    // timing.
    bool hasTempo() const;

    // Returns the chart tempo if the chart has split timing, or the simfile
    // tempo otherwise.
    Tempo* getTempo(Simfile* simfile);

    // Returns the chart tempo if the chart has split timing, or the simfile
    // tempo otherwise.
    const Tempo* getTempo(const Simfile* simfile) const;

    // Returns the total number of notes in the chart, excluding mines.
    int stepCount() const;

    // Sanitizes the notes and tempo, and makes sure the chart parameters are
    // valid.
    void sanitize();

    const Style* style = nullptr;
    std::string artist;
    Difficulty difficulty = DIFF_CHALLENGE;
    std::vector<double> radar;
    int meter = 1;

    NoteList notes;
    Tempo* tempo = nullptr;
};

// Returns the name of the given difficulty type.
const char* GetDifficultyName(Difficulty dt);

};  // namespace Vortex
