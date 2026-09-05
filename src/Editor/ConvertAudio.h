#pragma once

#include <System/Thread.h>
#include <System/Debug.h>

namespace Vortex {
extern bool canConvertAudio(const char* filename);

enum class AudioFormat {
    OGG,
    MP3,
    WAV,
};

struct OggConversionThread : public BackgroundThread {
    OggConversionThread();
    uint8_t progress;
    std::string inPath, outPath, error;
    AudioFormat format;
    bool isSimfile;

    /// Seconds dropped from the start of the song, and the lengths of
    /// the fades that ease it in and out. All zero leaves the audio as
    /// it is.
    double trimStart = 0.0;
    double fadeIn = 0.0;
    double fadeOut = 0.0;

    /// Seconds of silence to put in front of the song and after it.
    double padStart = 0.0;
    double padEnd = 0.0;

    void exec() override;
};

};  // namespace Vortex