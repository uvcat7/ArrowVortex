#pragma once

#include <vector>

namespace Vortex {

class MusicStretch {
   public:
    MusicStretch();
    ~MusicStretch();

    MusicStretch(const MusicStretch&) = delete;
    MusicStretch& operator=(const MusicStretch&) = delete;

    /// Creates the filter graph.
    bool init(int samplerate, double tempo);

    /// Destroy the filter graph and all buffered audio.
    void clear();

    /// Returns true if ready.
    bool isReady() const;

    /// Total number of lead frames from ideal playback position.
    int leadFrames() const { return myLeadFrames; }

    /// Pushes source frames into the filter.
    bool push(const short* interleaved, int numFrames);

    /// Returns the number of stretched frames that can be pulled right now.
    int available() const;

    /// Pulls up to maxFrames stretched frames the number of frames written.
    int pull(short* interleaved, int maxFrames);

   private:
    struct Impl;
    Impl* myImpl;
    std::vector<short> myQueue;
    int myLeadFrames;
};

};  // namespace Vortex
