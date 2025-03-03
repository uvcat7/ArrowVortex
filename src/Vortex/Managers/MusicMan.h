#pragma once

#include <Core/Common/Event.h>

#include <Audio/Sound.h>

#include <Vortex/Settings.h>

namespace AV {
namespace MusicMan {

void initialize(const XmrDoc& settings);
void deinitialize();

void tick();

void setMusicLoadingEnabled(bool enabled);

// Creates the audio out and starts loading music.
Sound::LoadResult load(Simfile* simfile);

// Destroys the audio out and unloads the current music.
void unload();

// Pauses the audio out.
void pause();

// Resumes the audio out.
void play();

// Sets the playback position of the audio out to the given location.
void seek(double seconds);

// Starts a thread that converts the current music to ogg-vorbis. If the music was loaded from
// an ogg-vorbis file, nothing happens. When conversion is completed, the thread is terminated
// and the ogg-vorbis file is saved.
void startOggConversion();

// Returns true if the audio out is paused, false otherwise.
bool isPaused();

// Returns the playback position of the audio out.
double getPlayTime();

// Returns the total song length of the current music.
double getSongLength();

// Returns the audio samples of the current music.
const Sound& getSamples();

struct MusicLengthChanged : Event {};
struct MusicSamplesChanged : Event {};

// extern Event musicLengthChanged;
// extern Event musicSamplesChanged;

} // namespace MusicMan
} // namespace AV
