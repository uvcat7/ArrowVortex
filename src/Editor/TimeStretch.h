#pragma once

#include <vector>

namespace Vortex {

/// Changes the playback tempo of interleaved 16-bit stereo audio without
/// moving the pitch, which plain resampling cannot do.
///
/// The work is done by ffmpeg's atempo filter, so this is a thin wrapper: push
/// source frames in, pull the same audio out shorter or longer. The filter
/// holds roughly one window of audio, so the first block after a reset comes
/// out short and is padded with silence by the caller.
class TimeStretch {
   public:
    ~TimeStretch();
    TimeStretch();

    /// Prepares the filter for the given tempo, where 1.0 is unchanged, 2.0 is
    /// twice as fast. Returns false if the filter could not be built, in which
    /// case the caller should fall back to resampling.
    bool reset(double tempo, int sampleRate);

    /// Throws away the buffered audio and starts the filter over at the tempo
    /// it was last configured with. Used when playback jumps to a new position.
    void flush();

    /// True when a filter is configured and ready to be fed.
    bool isReady() const { return graph_ != nullptr; }

    /// The tempo the filter is currently configured for.
    double tempo() const { return tempo_; }

    /// Feeds `frames` interleaved stereo frames to the filter.
    void push(const short* src, int frames);

    /// Takes up to `frames` frames of stretched audio. Returns how many were
    /// actually written, which is less than asked for while the filter is
    /// still filling up.
    int pull(short* dst, int frames);

    /// How many frames of stretched audio are ready to be taken. The caller
    /// feeds more source audio until this covers the block it has to fill,
    /// which is what keeps the output free of gaps.
    int available() const;

   private:
    void destroy();
    void drain();

    struct Impl;
    Impl* impl_;

    void* graph_;
    double tempo_;
    int sampleRate_;

    /// Holds what the filter has produced but the caller has not taken yet.
    std::vector<short> pending_;
    int pendingRead_;
};

};  // namespace Vortex
