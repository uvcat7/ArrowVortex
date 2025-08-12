#include <Editor/Sound.h>

#include <System/Debug.h>
#include <System/File.h>
#include <System/Thread.h>

#include <Core/Core.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/WideString.h>

#include <stdint.h>
#include <limits.h>
#include <chrono>
#include <fstream>

namespace Vortex {

static int Int24(const uint8_t* v) {
    if (v[2] & 0x80) {
        return ((0xff << 24) | (v[2] << 16) | (v[1] << 8) | v[0]);
    } else {
        return ((v[2] << 16) | (v[1] << 8) | v[0]);
    }
}

static void ConvertSamples(int numFrames, int16_t* dstL, int16_t* dstR,
                           const void* source, int numChannels,
                           int bytesPerSample) {
    if (bytesPerSample == 2) {
        const int16_t* src = static_cast<const int16_t*>(source);
        if (numChannels == 1) {
            memcpy(dstL, src, numFrames * 2);
            memcpy(dstR, src, numFrames * 2);
        } else {
            for (int i = 0; i < numFrames;
                 ++i, ++dstL, ++dstR, src += numChannels) {
                *dstL = src[0];
                *dstR = src[1];
            }
        }
    } else {
        const uint8_t* l = static_cast<const uint8_t*>(source);
        const uint8_t* r = l + ((numChannels > 1) ? bytesPerSample : 0);
        const int ofs = bytesPerSample * numChannels;
        if (bytesPerSample == 1) {
            for (int i = 0; i < numFrames;
                 ++i, ++dstL, ++dstR, l += ofs, r += ofs) {
                *dstL =
                    static_cast<int16_t>((static_cast<int>(*l) - 128) * 256);
                *dstR =
                    static_cast<int16_t>((static_cast<int>(*r) - 128) * 256);
            }
        } else if (bytesPerSample == 3) {
            for (int i = 0; i < numFrames;
                 ++i, ++dstL, ++dstR, l += ofs, r += ofs) {
                *dstL = static_cast<int16_t>(Int24(l) / 256);
                *dstR = static_cast<int16_t>(Int24(r) / 256);
            }
        }
    }
}

// ================================================================================================
// Loader

static const int BUFFER_SIZE = 1024;

class Sound::Thread : public BackgroundThread {
   public:
    ~Thread();
    Thread(Sound* sound, SoundSource* source);

    void exec();
    bool readBlock();
    void cleanup();
    uint8_t progress() { return myProgress; }
    double elapsedTime() { return Debug::getElapsedTime(myStartTime); }

   private:
    SoundSource* mySource;
    Sound* mySound;
    short* myBuffer;
    int myCurrentFrame;
    int myReservedFrames;
    uint8_t myProgress;
    std::chrono::steady_clock::time_point myStartTime;
};

Sound::Thread::~Thread() {
    terminate();
    cleanup();
}

Sound::Thread::Thread(Sound* sound, SoundSource* source) {
    myStartTime = Debug::getElapsedTime();

    mySound = sound;
    mySource = source;

    int bytesPerFrame = source->getNumChannels() * source->getBytesPerSample();
    myBuffer = (short*)malloc(BUFFER_SIZE * bytesPerFrame);

    myCurrentFrame = 0;
    myReservedFrames = 0;
    myProgress = 0;
}

void Sound::Thread::exec() {
    while (readBlock());
    cleanup();
}

bool Sound::Thread::readBlock() {
    if (getStopToken().stop_requested()) return false;

    int srcChannels = mySource->getNumChannels();
    int framesRead = mySource->readFrames(BUFFER_SIZE, myBuffer);

    if (mySound->myIsAllocated) {
        framesRead = min(framesRead, mySound->myNumFrames - myCurrentFrame);
    }

    if (framesRead > 0) {
        if (!mySound->myIsAllocated) {
            if (myCurrentFrame + framesRead > myReservedFrames) {
                myReservedFrames =
                    max(myReservedFrames * 2, myCurrentFrame + framesRead);

                short* newBufferL = static_cast<short*>(realloc(
                    mySound->mySamplesL, myReservedFrames * sizeof(short)));
                short* newBufferR = static_cast<short*>(realloc(
                    mySound->mySamplesR, myReservedFrames * sizeof(short)));

                if (newBufferL && newBufferR) {
                    mySound->mySamplesL = newBufferL;
                    mySound->mySamplesR = newBufferR;
                } else {
                    HudError(
                        "Insufficient memory to load the entire audio file.");

                    if (newBufferL) mySound->mySamplesL = newBufferL;
                    if (newBufferR) mySound->mySamplesR = newBufferR;

                    framesRead = 0;
                }
            }
            mySound->myNumFrames += framesRead;
        }

        short* dstL = mySound->mySamplesL + myCurrentFrame;
        short* dstR = mySound->mySamplesR + myCurrentFrame;

        ConvertSamples(framesRead, dstL, dstR, myBuffer,
                       mySource->getNumChannels(),
                       mySource->getBytesPerSample());

        myCurrentFrame += framesRead;

        if (mySound->myIsAllocated) {
            myProgress = (uint8_t)((uint64_t)100 * myCurrentFrame /
                                   mySound->myNumFrames);
        }
    }

    if (framesRead == 0) {
        mySound->myIsAllocated = true;
        mySound->myIsCompleted = true;
    }

    return (framesRead > 0);
}

void Sound::Thread::cleanup() {
    delete mySource;
    mySource = nullptr;

    free(myBuffer);
    myBuffer = nullptr;
}

// ================================================================================================
// Sound

SoundSource* LoadOgg(std::ifstream&& file, std::string& title,
                     std::string& artist);  // Defined in "load_ogg.cpp".
SoundSource* LoadMP3(std::ifstream&& file, std::string& title,
                     std::string& artist);  // Defined in "load_mp3.cpp".
SoundSource* LoadWav(std::ifstream&& file, std::string& title,
                     std::string& artist);  // Defined in "load_wav.cpp".

Sound::Sound() : myThread(nullptr), mySamplesL(nullptr), mySamplesR(nullptr) {
    clear();
}

Sound::~Sound() { clear(); }

void Sound::clear() {
    delete myThread;
    myThread = nullptr;

    if (mySamplesL) {
        free(mySamplesL);
        mySamplesL = nullptr;
    }

    if (mySamplesR) {
        free(mySamplesR);
        mySamplesR = nullptr;
    }

    myNumFrames = 0;
    myFrequency = 44100;
    myIsAllocated = true;
    myIsCompleted = true;
}

bool Sound::load(const char* path, bool threaded, std::string& title,
                 std::string& artist) {
    clear();

    SoundSource* source = nullptr;

    // Try to open the file.
    if (std::ifstream file(Widen(path).str(), std::ios::in | std::ios::binary);
        file.good()) {
        // Call the load function associated with the extension.
        std::string ext = Path(path).ext();
        Str::toLower(ext);
        if (ext == "ogg")
            source = LoadOgg(std::move(file), title, artist);
        else if (ext == "mp3")
            source = LoadMP3(std::move(file), title, artist);
        else if (ext == "wav")
            source = LoadWav(std::move(file), title, artist);
        else {
            Debug::blockBegin(Debug::ERROR, "could not load audio file");
            Debug::log("file: %s\n", path);
            Debug::log("reason: unknown audio format\n");
            Debug::blockEnd();
        }
    }

    if (!source) return false;

    // Start a thread that reads samples from the source.
    myFrequency = source->getFrequency();
    myNumFrames = source->getNumFrames();
    myIsAllocated = false;
    myIsCompleted = false;

    // If the number of frames is known beforehand, we can pre-allocate the
    // buffer.
    if (myNumFrames > 0) {
        uint64_t numBytes = myNumFrames * sizeof(short);

        if (threaded) {
            mySamplesL = static_cast<short*>(calloc(numBytes, 1));
            mySamplesR = static_cast<short*>(calloc(numBytes, 1));
        } else {
            mySamplesL = static_cast<short*>(malloc(numBytes));
            mySamplesR = static_cast<short*>(malloc(numBytes));
        }

        if (!mySamplesL || !mySamplesR) {
            HudError("Insufficient memory to load the entire audio file.");

            clear();
            delete source;
            return false;
        }

        myIsAllocated = true;
    }

    // Start a sample reading thread.
    if (threaded) {
        myThread = new Sound::Thread(this, source);
        myThread->start();
    } else {
        Sound::Thread thread(this, source);
        thread.exec();
    }

    return true;
}

int Sound::getLoadingProgress() const {
    return myThread ? myThread->progress() : 100;
}

double Sound::getLoadingTime() const {
    return myThread ? myThread->elapsedTime() : 0.0;
}

};  // namespace Vortex
