#include <System/Mixer.h>

#include <malloc.h>

#define WIN32_LEAN_AND_MEAN
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
static DWORD WINAPI MixerThread(LPVOID param);

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

    int myFrequency = 0;
    int myFreeBlockIndex = 0;

    BYTE* myBlockMemory = nullptr;
    WAVEHDR myHeaders[WAVEOUT_BLOCKS];
    HWAVEOUT myWaveout = nullptr;
    HANDLE myThread = nullptr;

    ThreadEvent myKillThread;
    ThreadEvent myPauseThread;
    ThreadEvent myResumeThread;
    ThreadEvent myThreadPaused;
    ThreadEvent myWriteBlock;

    volatile LONG myFreeBlocks = 0;

    bool myIsOpened = false;
    bool myIsPaused = true;

    MixSource* mySource;

    // ================================================================================================
    // MixerImpl :: constructor and destructor.

    ~MixerImpl() override {
        close();

        _aligned_free(myBlockMemory);
    }

    MixerImpl() {
        memset(myHeaders, 0, sizeof(myHeaders));
        mySource = nullptr;
        myBlockMemory = static_cast<BYTE*>(
            _aligned_malloc(WAVEOUT_BLOCK_SIZE * WAVEOUT_BLOCKS, 16));
        for (WAVEHDR& header : myHeaders) {
            memset(&header, 0, sizeof(WAVEHDR));
        }
    }

    void close() override {
        if (myThread) {
            SetEvent(static_cast<HANDLE>(myKillThread));
            LONG err = WaitForSingleObject(myThread, INFINITE);
            if (err) HudError("failed to close audio thread: %i", err);
            CloseHandle(myThread);
            myThread = nullptr;
        }
        if (myWaveout) {
            LONG err = waveOutReset(myWaveout);
            if (err) HudError("failed to reset wave out: %i\n", err);

            for (int i = 0; i < WAVEOUT_BLOCKS; ++i) {
                WAVEHDR* header = myHeaders + i;
                if (header->dwBufferLength > 0) {
                    LONG err = waveOutUnprepareHeader(myWaveout, myHeaders + i,
                                                      sizeof(WAVEHDR));
                    if (err)
                        HudError("failed to unprepare wave out header: %i",
                                 err);
                    memset(header, 0, sizeof(WAVEHDR));
                }
            }

            err = waveOutClose(myWaveout);
            if (err) HudError("failed to close wave out: %i\n", err);
            myWaveout = nullptr;
        }

        myFreeBlockIndex = 0;
        myFreeBlocks = 0;
        myIsOpened = false;
        myIsPaused = true;
    }

    bool open(MixSource* source, int samplerate) override {
        if (myIsOpened) close();

        // Try to open the waveout device.
        WAVEFORMATEX wfex;
        wfex.wFormatTag = WAVE_FORMAT_PCM;
        wfex.nChannels = WAVEOUT_CHANNELS;
        wfex.nSamplesPerSec = samplerate;
        wfex.nBlockAlign = sizeof(short) * WAVEOUT_CHANNELS;
        wfex.nAvgBytesPerSec = sizeof(short) * WAVEOUT_CHANNELS * samplerate;
        wfex.wBitsPerSample = sizeof(short) * 8;
        wfex.cbSize = 0;

        MMRESULT res =
            waveOutOpen(&myWaveout, WAVE_MAPPER, &wfex,
                        reinterpret_cast<DWORD_PTR>(&MixerCallback),
                        reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);

        if (res != MMSYSERR_NOERROR) {
            HudError("failed to open wave out: %i", res);
            close();
            return false;
        }

        // Create the output buffer blocks.
        int index = 0;
        for (WAVEHDR& header : myHeaders) {
            memset(&header, 0, sizeof(WAVEHDR));
            header.lpData = reinterpret_cast<LPSTR>(
                myBlockMemory +
                static_cast<size_t>(WAVEOUT_BLOCK_SIZE) * index);
            header.dwBufferLength = WAVEOUT_BLOCK_SIZE;
            header.dwUser = index;

            MMRESULT res =
                waveOutPrepareHeader(myWaveout, &header, sizeof(WAVEHDR));
            if (res != MMSYSERR_NOERROR) {
                HudError("Could not prepare waveout header: %i", res);
                close();
                return false;
            }
            ++index;
        }

        myFrequency = samplerate;
        mySource = source;
        myIsOpened = true;

        // Start the MixerDevice update thread.
        myThread = CreateThread(
            nullptr, 0, static_cast<LPTHREAD_START_ROUTINE>(MixerThread), this,
            0, nullptr);

        return true;
    }

    void pause() override {
        if (myIsOpened && !myIsPaused) {
            SetEvent(static_cast<HANDLE>(myPauseThread));
            WaitForSingleObject(static_cast<HANDLE>(myThreadPaused), INFINITE);
            waveOutReset(myWaveout);
            myIsPaused = true;
        }
    }

    void resume() override {
        if (myIsOpened && myIsPaused) {
            myFreeBlockIndex = 0;
            myFreeBlocks = WAVEOUT_BLOCKS;
            SetEvent(static_cast<HANDLE>(myResumeThread));
            waveOutRestart(myWaveout);
            SetEvent(static_cast<HANDLE>(myWriteBlock));
            myIsPaused = false;
        }
    }

    void blockDone() {
        InterlockedIncrement(&myFreeBlocks);
        SetEvent(static_cast<HANDLE>(myWriteBlock));
    }

    void mixThread() {
        const HANDLE events[] = {static_cast<HANDLE>(myKillThread),
                                 static_cast<HANDLE>(myResumeThread),
                                 static_cast<HANDLE>(myPauseThread),
                                 static_cast<HANDLE>(myWriteBlock)};
        while (true) {
            // Wait for a thread event.
            DWORD id = WaitForMultipleObjects(4, events, FALSE, INFINITE);
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

static DWORD WINAPI MixerThread(LPVOID mixer) {
    (static_cast<MixerImpl*>(mixer))->mixThread();
    return 0;
}

// ================================================================================================
// Mixer API.

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

};  // namespace Vortex
