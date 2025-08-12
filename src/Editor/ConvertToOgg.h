#pragma once

#include <System/Thread.h>
#include <System/Debug.h>

namespace Vortex {

struct OggConversionThread : public BackgroundThread {
    OggConversionThread();
    uint8_t progress;
    std::string outPath, error;
    void exec() override;
};

};  // namespace Vortex