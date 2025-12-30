#include <System/Mixer.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_stdinc.h>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <malloc.h>

#include "windows.h"
#include "mmsystem.h"

namespace Vortex {

static const int WAVEOUT_CHANNELS = 2;
static const int WAVEOUT_BLOCKS = 8;
static const int WAVEOUT_BLOCK_FRAMES = 8192;
static const int WAVEOUT_BLOCK_SIZE =
    sizeof(short) * WAVEOUT_CHANNELS * WAVEOUT_BLOCK_FRAMES;

static void CALLBACK MixerCallback(HWAVEOUT hwo, UINT msg, DWORD_PTR, DWORD_PTR,
                                   DWORD_PTR);

    void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream,
                            int additional_amount, int total_amount) {};

struct ThreadEvent {
    ThreadEvent() { handle = CreateEvent(nullptr, FALSE, FALSE, nullptr); }
    ~ThreadEvent() { CloseHandle(handle); }
    explicit operator HANDLE() { return handle; }
    HANDLE handle;
};

// ================================================================================================
// MixerImpl :: member data.

struct MixerImpl : public Mixer {
    enum ThreadEvents {
        WO_KILL_THREAD = WAIT_OBJECT_0 + 0,
        WO_RESUME_THREAD = WAIT_OBJECT_0 + 1,
        WO_PAUSE_THREAD = WAIT_OBJECT_0 + 2,
        WO_WRITE_BLOCK = WAIT_OBJECT_0 + 3,
    };

    int myFreeBlockIndex = 0;

    SDL_AudioSpec myAudioSpec = {SDL_AUDIO_U8, WAVEOUT_CHANNELS, 0};
    SDL_AudioDeviceID myDeviceId = 0;
    SDL_AudioStream* myAudioStream = nullptr;
    std::condition_variable myConditionVariable;
    std::mutex myMutex;
    std::optional<std::jthread> myThread{};

    char* myBlockMemory = nullptr;
    WAVEHDR myHeaders[WAVEOUT_BLOCKS];
    HWAVEOUT myWaveout = nullptr;

    std::atomic<bool> myKillThread;
    std::atomic<bool> myPauseThread = false;
    std::atomic<bool> myResumeThread = false;
    std::atomic<bool> myThreadPaused = false;
    std::atomic<bool> myWriteBlock = false;

    volatile LONG myFreeBlocks = 0;

    std::atomic<bool> myIsOpened = false;

    MixSource* mySource;

    // ================================================================================================
    // MixerImpl :: constructor and destructor.

    ~MixerImpl() override {
        close();
        SDL_aligned_free(myBlockMemory);
    }

    MixerImpl() {
        memset(myHeaders, 0, sizeof(myHeaders));
        mySource = nullptr;
        
        myBlockMemory = static_cast<char*>(
            SDL_aligned_alloc(WAVEOUT_BLOCK_SIZE * WAVEOUT_BLOCKS, 16));
        for (WAVEHDR& header : myHeaders) {
            memset(&header, 0, sizeof(WAVEHDR));
        }
    }

    void close() override {
        SDL_DestroyAudioStream(myAudioStream);
        if (myThread) {
            myThread.value().request_stop();
        }
        if (myDeviceId){
            SDL_CloseAudioDevice(myDeviceId);
            myDeviceId = 0;
        }
        myFreeBlockIndex = 0;
        myFreeBlocks = 0;
        myIsOpened = false;
        myPauseThread = true;
    }
   

    bool open(MixSource* source, int samplerate) override {
        if (myIsOpened) close();

        myAudioSpec.freq = samplerate;
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &myAudioSpec, audio_callback, &myBlockMemory);

        mySource = source;
        myIsOpened = true;

        // Start the MixerDevice update thread.
        if (myThread) myThread.reset();
        myThread.emplace(new std::jthread(&mixThread, this));

        return true;
    }

    void pause() override {
        if (myIsOpened && !myPauseThread) {
            myPauseThread = true;
            myThreadPaused.wait(true);
            waveOutReset(myWaveout);
        }
    }

    void resume() override {
        if (myIsOpened && myPauseThread) {
            myPauseThread = false;
            myFreeBlockIndex = 0;
            myFreeBlocks = WAVEOUT_BLOCKS;
            myThreadPaused.wait(false);
            SetEvent(static_cast<HANDLE>(myResumeThread));
            waveOutRestart(myWaveout);
            SetEvent(static_cast<HANDLE>(myWriteBlock));
        }
    }

    void blockDone() {
        InterlockedIncrement(&myFreeBlocks);
        SetEvent(static_cast<HANDLE>(myWriteBlock));
    }

    void mixThread() {
        while (true) {
            // Wait for a thread event.
            if (id == WO_KILL_THREAD) {
                return;
            } else if (id == WO_PAUSE_THREAD) {
                SetEvent(static_cast<HANDLE>(myThreadPaused));
                id = WaitForMultipleObjects(2, events, FALSE, INFINITE);
                if (id == WO_KILL_THREAD) return;
            } else if (id == WO_WRITE_BLOCK) {
                while (myFreeBlocks > 0) {
                    LONG result = InterlockedDecrement(&myFreeBlocks);
                    if (result < 0) break;

                    // Get the next free buffer block.
                    BYTE* samples =
                        myBlockMemory + myFreeBlockIndex * WAVEOUT_BLOCK_SIZE;
                    WAVEHDR* header = myHeaders + myFreeBlockIndex;
                    myFreeBlockIndex = (myFreeBlockIndex + 1) % WAVEOUT_BLOCKS;

                    // Send the filled block to wave out.
                    mySource->writeFrames(reinterpret_cast<short*>(samples),
                                          WAVEOUT_BLOCK_FRAMES);
                    waveOutWrite(myWaveout, header, sizeof(WAVEHDR));
                }
            }
        }
    }

};  // MixerImpl
// ================================================================================================
// Mixing callback functions.

static void CALLBACK MixerCallback(HWAVEOUT hwo, UINT msg, DWORD_PTR mixer,
                                   DWORD_PTR, DWORD_PTR) {
    if (msg == WOM_DONE) {
        reinterpret_cast<MixerImpl*>(mixer)->blockDone();
    }
}

// ================================================================================================
// Mixer API.

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

};  // namespace Vortex
