#pragma once

#include <Core/Vector.h>
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

    const Style* style;
    std::string artist;
    Difficulty difficulty;
    Vector<double> radar;
    int meter;

    NoteList notes;
    Tempo* tempo;
};

// Returns the name of the given difficulty type.
const char* GetDifficultyName(Difficulty dt);

};  // namespace Vortex
