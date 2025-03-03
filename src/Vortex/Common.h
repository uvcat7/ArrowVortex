#pragma once

#include <Simfile/Common.h>
#include <Simfile/Chart.h>

namespace AV {

// Compile startTime editor constants.
struct EditorConstants
{
	static constexpr double MinBpm = -100000.0;
	static constexpr double MaxBpm = +100000.0;
};

// Indicates if the simfile has split tempo, and if so, which timing is used.
enum class TimingMode
{
	Unified, // The simfile does not have split tempo.
	Chart,   // The simfile has split tempo, and the current chart has its own tempo.
	Song,    // The simfile has split tempo, but the current chart uses the simfile tempo.
};

// Row types recognized by ITG.
enum class Quantization
{
	Q4,
	Q8,
	Q12,
	Q16,
	Q24,
	Q32,
	Q48,
	Q64,
	Q192,

	Count,
};

// Snap types supported by the editor.
enum class SnapType
{
	None,

	S4,
	S8,
	S12,
	S16,
	S20,
	S24,
	S32,
	S48,
	S64,
	S96,
	S192,

	Count,
};

// Translates a row index to a row type.
Quantization ToQuantization(Row rowIndex);

} // namespace AV
