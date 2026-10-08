#include <Editor/MusicStretch.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
}

namespace Vortex {

static const int CHANNELS = 2;

// The window is the power of two at or above samplerate / 24, see
// <libavutil/af_atempo.c>. The sample rate affects the delay caused by the
// filter, so this compensates for it close enough.
static const double LEAD_PER_WINDOW = 0.4;

static int calculateWindow(int samplerate) {
    int window = 1;
    while (window < samplerate / 24) window *= 2;
    return window;
}

struct MusicStretch::Impl {
    AVFilterGraph* graph = nullptr;
    AVFilterContext* src = nullptr;
    AVFilterContext* sink = nullptr;
    int samplerate = 0;
    int64_t pushedFrames = 0;
};

MusicStretch::MusicStretch() : myImpl(nullptr), myLeadFrames(0) {}

MusicStretch::~MusicStretch() { clear(); }

void MusicStretch::clear() {
    if (myImpl) {
        avfilter_graph_free(&myImpl->graph);
        delete myImpl;
        myImpl = nullptr;
    }
    myQueue.clear();
    myLeadFrames = 0;
}

bool MusicStretch::isReady() const { return myImpl != nullptr; }

bool MusicStretch::init(int samplerate, double tempo) {
    clear();

    if (samplerate < 24 || tempo <= 0.0) return false;

    // Build graph description.
    char source[128];
    snprintf(source, sizeof(source),
             "abuffer@in=time_base=1/%d:sample_rate=%d:sample_fmt=s16:"
             "channel_layout=stereo",
             samplerate, samplerate);
    std::string desc = source;

    double lead = 0.0, scale = 1.0;
    const double advance = LEAD_PER_WINDOW * calculateWindow(samplerate);
    auto addStep = [&](double t) {
        desc += ",atempo=" + std::to_string(t);
        lead += advance * scale;
        scale *= t;
    };
    for (; tempo > 2.0; tempo /= 2.0) addStep(2.0);
    for (; tempo < 0.5; tempo /= 0.5) addStep(0.5);
    addStep(tempo);
    desc += ",abuffersink@out";

    // Build filter graph.
    myImpl = new Impl;
    myImpl->samplerate = samplerate;
    myImpl->graph = avfilter_graph_alloc();
    if (myImpl->graph &&
        avfilter_graph_parse_ptr(myImpl->graph, desc.c_str(), nullptr, nullptr,
                                 nullptr) >= 0 &&
        avfilter_graph_config(myImpl->graph, nullptr) >= 0) {
        AVFilterGraph* g = myImpl->graph;
        myImpl->src = avfilter_graph_get_filter(g, "abuffer@in");
        myImpl->sink = avfilter_graph_get_filter(g, "abuffersink@out");
    }

    if (!myImpl->src || !myImpl->sink) {
        clear();
        return false;
    }
    myLeadFrames = static_cast<int>(lead + 0.5);
    return true;
}

bool MusicStretch::push(const short* interleaved, int numFrames) {
    if (!myImpl || numFrames <= 0) return false;

    AVFrame* frame = av_frame_alloc();
    if (!frame) return false;

    // Setup frame.
    frame->nb_samples = numFrames;
    frame->format = AV_SAMPLE_FMT_S16;
    frame->sample_rate = myImpl->samplerate;
    frame->pts = myImpl->pushedFrames;
    av_channel_layout_default(&frame->ch_layout, CHANNELS);

    // Process frames.
    bool ok = av_frame_get_buffer(frame, 0) >= 0;
    if (ok) {
        memcpy(frame->data[0], interleaved,
               sizeof(short) * CHANNELS * static_cast<size_t>(numFrames));
        ok = av_buffersrc_add_frame_flags(myImpl->src, frame, 0) >= 0;
    }
    if (ok) myImpl->pushedFrames += numFrames;

    // Move to pull queue.
    av_frame_unref(frame);
    while (ok && av_buffersink_get_frame(myImpl->sink, frame) >= 0) {
        const short* data = reinterpret_cast<const short*>(frame->data[0]);
        myQueue.insert(myQueue.end(), data,
                       data + frame->nb_samples * CHANNELS);
        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    return ok;
}

int MusicStretch::available() const {
    return static_cast<int>(myQueue.size() / CHANNELS);
}

int MusicStretch::pull(short* interleaved, int maxFrames) {
    int n = std::min(maxFrames, available());
    auto end = myQueue.begin() + static_cast<size_t>(n) * CHANNELS;
    std::copy(myQueue.begin(), end, interleaved);
    myQueue.erase(myQueue.begin(), end);
    return n;
}

};  // namespace Vortex