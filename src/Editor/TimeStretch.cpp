// libavutil refuses to be included from C++ without this.
#define __STDC_CONSTANT_MACROS
#include <stdint.h>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include <Editor/TimeStretch.h>

#include <Core/Core.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <System/Debug.h>

#include <algorithm>
#include <math.h>
#include <string.h>

namespace Vortex {

namespace {

static const int CHANNELS = 2;

// atempo only accepts a factor between 0.5 and 2.0, so a larger change is made
// out of several filters chained together.
static const double TEMPO_MIN = 0.5;
static const double TEMPO_MAX = 2.0;

/// Splits a tempo into a chain of factors that atempo will accept, each of
/// which multiplies out to the requested tempo.
static void SplitTempo(double tempo, std::vector<double>& out) {
    while (tempo > TEMPO_MAX) {
        out.push_back(TEMPO_MAX);
        tempo /= TEMPO_MAX;
    }
    while (tempo < TEMPO_MIN) {
        out.push_back(TEMPO_MIN);
        tempo /= TEMPO_MIN;
    }
    out.push_back(tempo);
}

};  // anonymous namespace

// ================================================================================================
// TimeStretch :: filter graph.

struct TimeStretch::Impl {
    AVFilterGraph* graph = nullptr;
    AVFilterContext* src = nullptr;
    AVFilterContext* sink = nullptr;
    AVFrame* in = nullptr;
    AVFrame* out = nullptr;
};

TimeStretch::TimeStretch() {
    impl_ = new Impl;
    graph_ = nullptr;
    tempo_ = 1.0;
    sampleRate_ = 44100;
    pendingRead_ = 0;
}

TimeStretch::~TimeStretch() {
    destroy();
    delete impl_;
}

void TimeStretch::destroy() {
    if (impl_->in) av_frame_free(&impl_->in);
    if (impl_->out) av_frame_free(&impl_->out);
    if (impl_->graph) avfilter_graph_free(&impl_->graph);

    impl_->src = nullptr;
    impl_->sink = nullptr;
    graph_ = nullptr;

    pending_.clear();
    pendingRead_ = 0;
}

bool TimeStretch::reset(double tempo, int sampleRate) {
    destroy();

    tempo_ = tempo;
    sampleRate_ = sampleRate;

    if (sampleRate <= 0) return false;

    const AVFilter* abuffer = avfilter_get_by_name("abuffer");
    const AVFilter* atempo = avfilter_get_by_name("atempo");
    const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffer || !atempo || !abuffersink) {
        Debug::blockBegin(Debug::WARNING, "could not build the tempo filter");
        Debug::log("reason: atempo is missing from this ffmpeg build\n");
        Debug::blockEnd();
        return false;
    }

    impl_->graph = avfilter_graph_alloc();
    if (!impl_->graph) return false;

    // Source: interleaved 16-bit stereo at the mixer's rate.
    char args[256];
    snprintf(
        args, sizeof(args),
        "sample_rate=%d:sample_fmt=%s:channel_layout=stereo:time_base=1/%d",
        sampleRate, av_get_sample_fmt_name(AV_SAMPLE_FMT_S16), sampleRate);

    if (avfilter_graph_create_filter(&impl_->src, abuffer, "in", args, nullptr,
                                     impl_->graph) < 0) {
        destroy();
        return false;
    }

    // One atempo per factor, chained together.
    std::vector<double> factors;
    SplitTempo(tempo, factors);

    AVFilterContext* tail = impl_->src;
    for (int i = 0; i < static_cast<int>(factors.size()); ++i) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "atempo%d", i);
        snprintf(value, sizeof(value), "%.10f", factors[i]);

        AVFilterContext* step = nullptr;
        if (avfilter_graph_create_filter(&step, atempo, name, value, nullptr,
                                         impl_->graph) < 0) {
            destroy();
            return false;
        }
        if (avfilter_link(tail, 0, step, 0) < 0) {
            destroy();
            return false;
        }
        tail = step;
    }

    if (avfilter_graph_create_filter(&impl_->sink, abuffersink, "out", nullptr,
                                     nullptr, impl_->graph) < 0) {
        destroy();
        return false;
    }

    // The sink has to be told which format it may hand back, otherwise it is
    // free to pick a planar one.
    static const enum AVSampleFormat formats[] = {AV_SAMPLE_FMT_S16,
                                                  AV_SAMPLE_FMT_NONE};
    if (av_opt_set_int_list(impl_->sink, "sample_fmts", formats,
                            AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
        destroy();
        return false;
    }

    if (avfilter_link(tail, 0, impl_->sink, 0) < 0) {
        destroy();
        return false;
    }
    if (avfilter_graph_config(impl_->graph, nullptr) < 0) {
        destroy();
        return false;
    }

    impl_->in = av_frame_alloc();
    impl_->out = av_frame_alloc();
    if (!impl_->in || !impl_->out) {
        destroy();
        return false;
    }

    graph_ = impl_->graph;
    return true;
}

void TimeStretch::flush() {
    if (!isReady()) return;
    reset(tempo_, sampleRate_);
}

// ================================================================================================
// TimeStretch :: pushing and pulling audio.

void TimeStretch::push(const short* src, int frames) {
    if (!isReady() || frames <= 0) return;

    AVFrame* frame = impl_->in;

    // Reallocate only when the size changes or the filter is still holding
    // the previous buffer, so the common case costs nothing.
    if (frame->nb_samples != frames || frame->format != AV_SAMPLE_FMT_S16 ||
        !frame->data[0]) {
        av_frame_unref(frame);
        frame->nb_samples = frames;
        frame->format = AV_SAMPLE_FMT_S16;
        frame->sample_rate = sampleRate_;
        av_channel_layout_default(&frame->ch_layout, CHANNELS);
        if (av_frame_get_buffer(frame, 0) < 0) return;
    } else if (av_frame_make_writable(frame) < 0) {
        return;
    }

    memcpy(frame->data[0], src, sizeof(short) * CHANNELS * frames);

    if (av_buffersrc_add_frame_flags(impl_->src, frame,
                                     AV_BUFFERSRC_FLAG_KEEP_REF) >= 0) {
        drain();
    }
}

void TimeStretch::drain() {
    AVFrame* frame = impl_->out;
    while (true) {
        av_frame_unref(frame);
        if (av_buffersink_get_frame(impl_->sink, frame) < 0) break;

        const int frames = frame->nb_samples;
        if (frames > 0) {
            const int oldSize = static_cast<int>(pending_.size());
            pending_.resize(oldSize + frames * CHANNELS);
            memcpy(pending_.data() + oldSize, frame->data[0],
                   sizeof(short) * CHANNELS * frames);
        }
    }
    av_frame_unref(frame);

    // Drop what has already been handed out. Everything taken is the common
    // case and costs nothing; otherwise the move is put off until half the
    // buffer is stale, so it does not run on every callback.
    const int left = static_cast<int>(pending_.size()) - pendingRead_;
    if (left <= 0) {
        pending_.clear();
        pendingRead_ = 0;
    } else if (pendingRead_ >= left) {
        memmove(pending_.data(), pending_.data() + pendingRead_,
                sizeof(short) * left);
        pending_.resize(left);
        pendingRead_ = 0;
    }
}

int TimeStretch::available() const {
    return (static_cast<int>(pending_.size()) - pendingRead_) / CHANNELS;
}

int TimeStretch::pull(short* dst, int frames) {
    if (frames <= 0) return 0;

    const int taken = std::min(frames, this->available());
    if (taken > 0) {
        memcpy(dst, pending_.data() + pendingRead_,
               sizeof(short) * CHANNELS * taken);
        pendingRead_ += taken * CHANNELS;
    }
    return taken;
}

};  // namespace Vortex
