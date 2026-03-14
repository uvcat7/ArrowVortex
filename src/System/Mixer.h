#include <Core/Core.h>
#include <filesystem>
#include <SDL3_mixer/SDL_mixer.h>
namespace fs = std::filesystem;

namespace Vortex {

enum SoundEffects { kNoteTick, kBeatTick };
struct MixSource {
    virtual void writeFrames(short* buffer, int frames) = 0;
};

struct Mixer {
    static Mixer* create();

    virtual ~Mixer();

    virtual bool is_initialized() = 0;

    virtual bool open_sfx(fs::path path) = 0;

    virtual void play_sfx(SoundEffects s, float volume) = 0;

    /// Opens the mixer for audio output at the given samplerate. The mixer is
    /// initially paused.
    virtual bool open(fs::path path) = 0;

    /// Stops audio output and closes the mixer, until open is called again.
    virtual void close() = 0;

    /// Pauses the audio mixer and stops playing samples from the mix source.
    virtual void pause() = 0;

    /// Unpauses the audio mixer and starts playing samples from the mix source.
    virtual void resume(int64_t ms, float rate, float volume) = 0;
};

};  // namespace Vortex
