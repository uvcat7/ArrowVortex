#include <Editor/FindOnsets.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <System/Thread.h>

#include <aubio/aubio.h>
#include <math.h>
#include <mutex>
#include <vector>
#include <cstring>

namespace Vortex {

// ================================================================================================
// Main function.

void FindOnsets(const float* samples, int samplerate, int numFrames,
                int numThreads, std::vector<Onset>& out) {
    static const int windowlen = 256;
    static const int bufsize = windowlen * 4;
    static const char* method = "complex";

    auto onset = new_aubio_onset(method, bufsize, windowlen, samplerate);
    fvec_t *samplevec = new_fvec(windowlen), *beatvec = new_fvec(2);
    for (int i = 0; i <= numFrames - windowlen; i += windowlen) {
        std::memcpy(samplevec->data, samples + i, sizeof(float) * windowlen);
        aubio_onset_do(onset, samplevec, beatvec);
        if (beatvec->data[0] > 0) {
            int pos = aubio_onset_get_last(onset);
            Onset new_onset = {pos, 1.0f};
            if (pos >= 0) out.emplace_back(new_onset);
        }
    }
    del_fvec(samplevec);
    del_fvec(beatvec);
    del_aubio_onset(onset);
}

};  // namespace Vortex