#pragma once

#include <Precomp.h>

namespace AV {

// Controls the audio output stream.
namespace AudioOut
{
	// A mixer that provides samples to play.
	struct Mixer
	{
		// Used when more audio samples are requested for playback. The playback is currently always
		// in stereo, so the number of sample values written to the buffer must be twice the number
		// of frames and must be interleaved L-R-L-R-(...).
		virtual void mix(short* buffer, int frames) = 0;
	};
	
	void initialize();
	void deinitialize();
	
	// Prepares for output at the given sampleRate. Playback is initially paused.
	bool open(Mixer* mixer, int sampleRate);
	
	// Discards all buffered frames and closes the output.
	void close();
	
	// Discards all current buffered frames and plauses playback.
	void pause();
	
	// Unpauses the playback and starts requesting new frames from the mixer.
	void resume();
};

} // namespace AV