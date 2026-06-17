#pragma once

#include <Core/Core.h>

namespace Vortex {

/// Determines how the selection is modified.
enum SelectModifier {
    SELECT_SET,  ///< Replace the current selection.
    SELECT_ADD,  ///< Add to the current selection.
    SELECT_SUB,  ///< Subtract from the current selection.
};

/// Simfile limitations/constants.
enum SimConstants {
    SIM_MAX_COLUMNS = 32,
    SIM_MAX_PLAYERS = 16,
    SIM_DEFAULT_BPM = 120,
};

/// Supported difficulties.
enum Difficulty {
    DIFF_BEGINNER,
    DIFF_EASY,
    DIFF_MEDIUM,
    DIFF_HARD,
    DIFF_CHALLENGE,
    DIFF_EDIT,

    NUM_DIFFICULTIES
};

/// Represents a row/column index.
struct RowCol {
    int row, col;
};

// Represents a generic tag/value property.
struct Property {
    std::string tag, val;
};

/// Represents a single note.
struct Note {
    int row, endrow;
    uint32_t col : 8;
    uint32_t player : 4;
    uint32_t type : 4;
    uint32_t quant : 8;
};

};  // namespace Vortex
