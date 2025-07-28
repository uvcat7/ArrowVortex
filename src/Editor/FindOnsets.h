#pragma once

#include <Core/Vector.h>

namespace Vortex {

struct Onset {
    int pos;
    double strength;
};

void FindOnsets(const float* samples, int samplerate, int numFrames,
                int numThreads, Vector<Onset>& out);

};  // namespace Vortex