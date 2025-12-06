#pragma once

#include <Core/Core.h>

namespace Vortex {

struct MixSource {
    virtual void writeFrames(short* buffer, int frames) = 0;
};

struct Mixer {
    static Mixer* create();

    virtual ~Mixer();

    /// Opens the mixer for audio output at the given samplerate. The mixer is
    /// initially paused.
    virtual bool open(MixSource* source, int samplerate) = 0;

    /// Stops audio output and closes the mixer, until open is called again.
    virtual void close() = 0;

    /// Pauses the audio mixer and stops playing samples from the mix source.
    virtual void pause() = 0;

    /// Unpauses the audio mixer and starts playing samples from the mix source.
    virtual void resume() = 0;
};

};  // namespace Vortex
