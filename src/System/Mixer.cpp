#include <System/Mixer.h>
// NOLINTBEGIN
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_stdinc.h>
// NOLINTEND
#include <Editor/Common.h>

namespace Vortex {

static const int WAVEOUT_CHANNELS = 2;
static const int WAVEOUT_BLOCKS = 8;
static const int WAVEOUT_BLOCK_FRAMES = 8192;
static const int WAVEOUT_BLOCK_SIZE =
    sizeof(short) * WAVEOUT_CHANNELS * WAVEOUT_BLOCK_FRAMES;

// ================================================================================================
// MixerImpl :: member data.
SDL_AudioStream* audio_stream = nullptr;
MixSource* mix_source = nullptr;
static void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream,
                                   int additional_amount, int total_amount) {
    int remaining = additional_amount;

    while (remaining > 0) {
        int current = std::min(WAVEOUT_BLOCK_SIZE, remaining);

        mix_source->writeFrames(static_cast<short*>(userdata),
                                current / WAVEOUT_CHANNELS / sizeof(short));
        SDL_PutAudioStreamData(audio_stream, userdata, current);
        remaining -= current;
    }
}

struct MixerImpl : public Mixer {
    SDL_AudioSpec audio_spec = {SDL_AUDIO_S16LE, WAVEOUT_CHANNELS, 0};

    short* sample_memory = nullptr;

    bool is_paused = false;
    bool music_loaded = false;

    // ================================================================================================
    // MixerImpl :: constructor and destructor.

    ~MixerImpl() override { close(); }

    MixerImpl() {
        mix_source = nullptr;
        sample_memory =
            static_cast<short*>(SDL_aligned_alloc(WAVEOUT_BLOCK_SIZE, 16));
    }

    void close() override {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = nullptr;
        music_loaded = false;
        is_paused = true;
    }

    bool open(MixSource* source, int samplerate) override {
        if (music_loaded) close();

        audio_spec.freq = samplerate;
        audio_stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, audio_callback,
            sample_memory);
        if (!audio_stream) {
            HudError("Failed to open audio device stream with error %s.\n",
                     SDL_GetError());
            return false;
        }
        mix_source = source;
        music_loaded = true;
        return true;
    }

    void pause() override {
        if (music_loaded && !is_paused) {
            is_paused = true;
            SDL_PauseAudioStreamDevice(audio_stream);
        }
    }

    void resume() override {
        if (music_loaded && is_paused) {
            is_paused = false;
            SDL_ResumeAudioStreamDevice(audio_stream);
        }
    }

};  // MixerImpl

// ================================================================================================
// Mixer API.

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

};  // namespace Vortex
