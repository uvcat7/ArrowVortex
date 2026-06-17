#include <Editor/FindOnsets.h>

#include <Core/Utils.h>
#include <Core/Vector.h>
#include <Core/AlignedMemory.h>

#include <System/Thread.h>

#include <aubio/aubio.h>
#include <math.h>
#include <mutex>

namespace Vortex {

// ================================================================================================
// Main function.

void FindOnsets(const float *samples, int samplerate, int numFrames,
                int numThreads, Vector<Onset> &out) {
    static const int windowlen = 256;
    static const int bufsize = windowlen * 4;
    static const char *method = "complex";

    auto onset = new_aubio_onset(method, bufsize, windowlen, samplerate);
    fvec_t *samplevec = new_fvec(windowlen), *beatvec = new_fvec(2);
    for (int i = 0; i <= numFrames - windowlen; i += windowlen) {
        memcpy(samplevec->data, samples + i, sizeof(float) * windowlen);
        aubio_onset_do(onset, samplevec, beatvec);
        if (beatvec->data[0] > 0) {
            int pos = aubio_onset_get_last(onset);
            if (pos >= 0) out.push_back({pos, 1.0});
        }
    }
    del_fvec(samplevec);
    del_fvec(beatvec);
    del_aubio_onset(onset);
}

};  // namespace Vortex