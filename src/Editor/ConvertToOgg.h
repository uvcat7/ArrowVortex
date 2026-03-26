#pragma once

#include <System/Thread.h>
#include <System/Debug.h>

namespace Vortex {

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
    void exec() override;
};

};  // namespace Vortex