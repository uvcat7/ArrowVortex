#pragma once

#include <vector>

#include <Simfile/Common.h>

namespace Vortex {

// ================================================================================================
// Sorting helpers.

template <typename A, typename B>
static bool LessThanRowCol(const A& a, const B& b)
{
	if(a.row != b.row)
	{
		return (int)a.row < (int)b.row;
	}
	return (int)a.col < (int)b.col;
}

template <typename A, typename B>
static bool LessThanRow(const A& a, const B& b)
{
	return (int)a.row < (int)b.row;
}

template <typename A, typename B>
static int CompareRowCol(const A& a, const B& b)
{
	if(a.row != b.row)
	{
		return (int)a.row - (int)b.row;
	}
	return (int)a.col - (int)b.col;
}

template <typename A, typename B>
static int CompareRow(const A& a, const B& b)
{
	return (int)a.row - (int)b.row;
}

// ================================================================================================
// Editor constants.

/// Minimum/maximum tempo ranges.
enum TempoRanges
{
	VC_MIN_BPM = -100000,
	VC_MAX_BPM = +100000,
	VC_MIN_STOP = -100,
	VC_MAX_STOP = +100,
	VC_MIN_OFFSET = -100,
	VC_MAX_OFFSET = +100,
};

/// Reported simfile changes.
enum VortexChangesMade
{
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
	VCM_MUSIC_IS_LOADED    = 1 << 14,

	VCM_CHART_LIST_CHANGED = 1 << 15,

	VCM_ALL_CHANGES = 0xFFFFFFFF
};

/// Row types recognized by ITG.
enum RowType
{
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

/// Snap types supported by the editor.
enum SnapType
{
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
enum BackgroundStyle
{
	BG_STYLE_STRETCH,   ///< Scale non-uniformly and match the view dimensions.
	BG_STYLE_LETTERBOX, ///< Scale uniformly and letterbox.
	BG_STYLE_CROP       ///< Scale uniformly and crop.
};

// Returns true if the tag of the clipboard data matches the given tag.
bool HasClipboardData(StringRef tag);

// Encodes the given data to an ascii85 string and sends it to the clipboard.
void SetClipboardData(StringRef tag, const uchar* data, int size);

// Reads a string from the clipboard and decodes it using ascii85.
std::vector<uchar> GetClipboardData(StringRef tag);

// Returns a text representation of the given snap type.
const char* ToString(SnapType st);

// Returns a number value with it's ordinal suffix.
const String OrdinalSuffix(int i);

// Returns the color representation of the given difficulty type.
color32 ToColor(Difficulty dt);

// Translates a row index to a row type.
RowType ToRowType(int rowIndex);

}; // namespace Vortex
