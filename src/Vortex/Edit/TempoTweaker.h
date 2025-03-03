#pragma once

#include <Simfile/Tempo.h>

namespace AV {

// Indicates which tempo value is being tweaked, when tweak mode is active.
enum class TweakMode
{
	None,   // Tweak mode is currently not active.
	Offset, // The music offset is being tweaked.
	BPM,    // The BPM at the tweak row is being tweaked.
	Stop,   // The stop at the tweak row is being tweaked.
};

namespace TempoTweaker
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	// Start tweaking the music offset.
	void startTweakingOffset();

	// Start tweaking the BPM value at the given row.
	void startTweakingBpm(Row row);

	// Start tweaking the stop value at the given row.
	void startTweakingStop(Row row);

	// Sets a new tweak value, if tweaking is currently active.
	void setTweakValue(double value);

	// Stops tweaking. If apply is false then the original value is restored.
	void stopTweaking(bool apply = false);

	// Returns the tweak mode that is currently active.
	TweakMode getTweakMode();

	// Returns the current tweak value.
	double getTweakValue();

	// Returns the current tweak row.
	int getTweakRow();
};

} // namespace AV
