#pragma once

#include <vector>

#include <Simfile/Common.h>

#include <Core/Draw.h>

namespace Vortex {

// ================================================================================================
// Sorting helpers.

template <typename A, typename B>
static bool LessThanRowCol(const A& a, const B& b) {
    if (a.row != b.row) {
        return (int)a.row < (int)b.row;
    }
    return (int)a.col < (int)b.col;
}

template <typename A, typename B>
static bool LessThanRow(const A& a, const B& b) {
    return (int)a.row < (int)b.row;
}

template <typename A, typename B>
static int CompareRowCol(const A& a, const B& b) {
    if (a.row != b.row) {
        return (int)a.row - (int)b.row;
    }
    return (int)a.col - (int)b.col;
}

template <typename A, typename B>
static int CompareRow(const A& a, const B& b) {
    return (int)a.row - (int)b.row;
}

// ================================================================================================
// Editor constants.

/// Minimum/maximum tempo ranges.
enum TempoRanges {
    VC_MIN_BPM = -100000,
    VC_MAX_BPM = +100000,
    VC_MIN_STOP = -100,
    VC_MAX_STOP = +100,
    VC_MIN_OFFSET = -100,
    VC_MAX_OFFSET = +100,
};

/// Reported simfile changes.
enum VortexChangesMade {
    VCM_FILE_CHANGED = 1 << 0,
    VCM_CHART_CHANGED = 1 << 1,
    VCM_NOTES_CHANGED = 1 << 2,
    VCM_TEMPO_CHANGED = 1 << 3,
    VCM_VIEW_CHANGED = 1 << 4,
    VCM_ZOOM_CHANGED = 1 << 5,

    VCM_SELECTION_CHANGED = 1 << 6,

    VCM_BACKGROUND_PATH_CHANGED = 1 << 7,
    VCM_BANNER_PATH_CHANGED = 1 << 8,
    VCM_MUSIC_PATH_CHANGED = 1 << 9,

    VCM_CHART_PROPERTIES_CHANGED = 1 << 10,
    VCM_SONG_PROPERTIES_CHANGED = 1 << 11,

    VCM_END_ROW_CHANGED = 1 << 12,

    VCM_MUSIC_IS_ALLOCATED = 1 << 13,
    VCM_MUSIC_IS_LOADED = 1 << 14,

    VCM_CHART_LIST_CHANGED = 1 << 15,

    VCM_ALL_CHANGES = 0xFFFFFFFF
};

/// Row types recognized by ITG.
enum RowType {
    RT_4TH,
    RT_8TH,
    RT_12TH,
    RT_16TH,
    RT_24TH,
    RT_32ND,
    RT_48TH,
    RT_64TH,
    RT_192TH,

    NUM_ROW_TYPES,
};

const uint32_t ROW_TYPE_COLOR[9] = {
    RGBAtoColor32(255, 0, 0, 255),      // Red.
    RGBAtoColor32(41, 118, 245, 255),   // Blue.
    RGBAtoColor32(145, 12, 206, 255),   // Purple.
    RGBAtoColor32(255, 255, 0, 255),    // Yellow.
    RGBAtoColor32(206, 12, 113, 255),   // Pink.
    RGBAtoColor32(247, 148, 29, 255),   // Orange.
    RGBAtoColor32(105, 231, 245, 255),  // Teal.
    RGBAtoColor32(0, 198, 0, 255),      // Green.
    RGBAtoColor32(132, 132, 132, 255),  // Gray.
};

/// Snap types supported by the editor.
enum SnapType {
    ST_NONE,

    ST_4TH,
    ST_8TH,
    ST_12TH,
    ST_16TH,
    ST_24TH,
    ST_32ND,
    ST_48TH,
    ST_64TH,
    ST_96TH,
    ST_192TH,
    ST_CUSTOM,

    NUM_SNAP_TYPES,
};

/// Determines the resize algorithm when the song background is drawn.
enum BackgroundStyle {
    BG_STYLE_STRETCH,    ///< Scale non-uniformly and match the view dimensions.
    BG_STYLE_LETTERBOX,  ///< Scale uniformly and letterbox.
    BG_STYLE_CROP        ///< Scale uniformly and crop.
};

// Returns a text representation of the given snap type.
const char* ToString(SnapType st);

// Returns a number value with it's ordinal suffix.
const std::string OrdinalSuffix(int i);

// Returns the color representation of the given difficulty type.
uint32_t ToColor(Difficulty dt);

// Translates a row index to a row type.
RowType ToRowType(int rowIndex);

// Translates a row type to a color.
uint32_t ToRowTypeColor(RowType type);
uint32_t ToRowTypeColor(int type);

};  // namespace Vortex
