#include <System/Mixer.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_stdinc.h>
#include <Editor/Common.h>

#include <malloc.h>

namespace Vortex {

static const int WAVEOUT_CHANNELS = 2;
static const int WAVEOUT_BLOCKS = 8;
static const int WAVEOUT_BLOCK_FRAMES = 8192;
static const int WAVEOUT_BLOCK_SIZE =
    sizeof(short) * WAVEOUT_CHANNELS * WAVEOUT_BLOCK_FRAMES;

// ================================================================================================
// MixerImpl :: member data.
SDL_AudioStream* myAudioStream = nullptr;
MixSource* mySource;
void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream,
                            int additional_amount, int total_amount) {
    int remaining = additional_amount;
    short samples[WAVEOUT_BLOCK_SIZE];

    while (remaining > 0) {
        int current = std::min(WAVEOUT_BLOCK_SIZE, remaining);

        mySource->writeFrames(samples,
                              current / WAVEOUT_CHANNELS / sizeof(short));
        SDL_PutAudioStreamData(myAudioStream, samples, current);
        remaining -= current;
    }
}

struct MixerImpl : public Mixer {
    int myFreeBlockIndex = 0;

    SDL_AudioSpec myAudioSpec = {SDL_AUDIO_S16LE, WAVEOUT_CHANNELS, 0};
    SDL_AudioDeviceID myDeviceId = 0;

    char* myBlockMemory = nullptr;

    bool myPauseThread = false;
    bool myIsOpened = false;

    // ================================================================================================
    // MixerImpl :: constructor and destructor.

    ~MixerImpl() override { close(); }

    MixerImpl() {
        mySource = nullptr;
        myBlockMemory = static_cast<char*>(
            SDL_aligned_alloc(WAVEOUT_BLOCK_SIZE * WAVEOUT_BLOCKS, 16));
    }

    void close() override {
        SDL_PauseAudioStreamDevice(myAudioStream);
        SDL_DestroyAudioStream(myAudioStream);
        if (myDeviceId) {
            SDL_CloseAudioDevice(myDeviceId);
            myDeviceId = 0;
        }
        myFreeBlockIndex = 0;
        myIsOpened = false;
        myPauseThread = true;
    }

    bool open(MixSource* source, int samplerate) override {
        if (myIsOpened) close();

        myAudioSpec.freq = samplerate;
        myAudioStream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &myAudioSpec, audio_callback,
            &myBlockMemory);
        if (!myAudioStream) {
            HudError("Failed to open audio device stream with error %s.\n",
                     SDL_GetError());
            return false;
        }
        mySource = source;
        myIsOpened = true;
        return true;
    }

    void pause() override {
        if (myIsOpened && !myPauseThread) {
            myPauseThread = true;
            SDL_PauseAudioStreamDevice(myAudioStream);
        }
    }

    void resume() override {
        if (myIsOpened && myPauseThread) {
            myPauseThread = false;
            SDL_ResumeAudioStreamDevice(myAudioStream);
        }
    }

};  // MixerImpl

// ================================================================================================
// Mixer API.

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

};  // namespace Vortex
