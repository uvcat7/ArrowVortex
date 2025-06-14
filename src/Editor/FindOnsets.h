#pragma once

#include <vector>

namespace Vortex {

struct Onset { int pos; double strength; };

void FindOnsets(const float* samples, int samplerate, int numFrames, int numThreads, std::vector<Onset>& out);

}; // namespace Vortex
