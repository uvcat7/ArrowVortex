#include <Simfile/TimingData.h>

#include <math.h>

#include <Core/Vector.h>
#include <Core/Utils.h>

#include <System/Debug.h>

#include <Simfile/SegmentGroup.h>
#include <Simfile/Tempo.h>

#include <Managers/TempoMan.h>

#include <float.h>

namespace Vortex {
namespace {

typedef TimingData::Event Event;
typedef TimingData::TimeSig TimeSig;
typedef TimingData::ScrollRow ScrollRow;
typedef TimingData::ScrollSpeed ScrollSpeed;
typedef TimingData::ScrollFake ScrollFake;

// ================================================================================================
// Timing segments merge.

struct MergedTS {
    const Segment* seg;
    Segment::Type type;
};

static void Merge(Vector<MergedTS>& out, const SegmentList& in) {
    if (in.empty()) return;

    out.grow(out.size() + in.size());

    // Work backwards, that way insertion can be done on the fly.
    auto write = out.end() - 1;

    auto read = out.end() - in.size() - 1;
    auto readEnd = out.begin() - 1;

    auto ins = in.rbegin();
    auto insEnd = in.rend();

    while (read != readEnd) {
        // Copy segments from the in list.
        while (ins != insEnd && ins->row > read->seg->row) {
            write->type = in.type();
            write->seg = ins.ptr;
            --ins, --write;
        }
        if (ins == insEnd) break;

        // Move existing segments.
        while (read != readEnd && read->seg->row >= ins->row) {
            *write = *read;
            --read, --write;
        }
    }

    // After read end is reached, there might still be some segments that need
    // to be inserted.
    while (ins != insEnd) {
        write->type = in.type();
        write->seg = ins.ptr;
        --ins, --write;
    }
}

// ================================================================================================
// Timing data processing.

struct WarpResult {
    int row;
    double time;
    MergedTS* it;
};

static WarpResult HandleWarp(Vector<Event>& out, MergedTS* it, MergedTS* end,
                             int warpRows) {
    Event* entry = &out.back();
    double spr = entry->spr;
    double time = entry->endTime;
    double targetTime = entry->time;

    // Modify the current entry, make it the start of the warp.
    entry->rowTime = targetTime = max(targetTime, entry->rowTime);
    entry->endTime = targetTime = max(targetTime, entry->endTime);
    entry->spr = 0.0;

    // Skip all segments that end inside the warp.
    int prevRow = entry->row;
    while (it != end) {
        int row = it->seg->row;

        // Move time forwards (or backwards) to the start of the current
        // segment.
        int rowsPassed = row - prevRow;
        if (rowsPassed <= warpRows) {
            warpRows -= rowsPassed;
        } else {
            rowsPassed -= warpRows;
            time += rowsPassed * spr;
            warpRows = 0;
        }

        // Check if the warp ended before the current segments.
        if (spr > 0 && time > targetTime && warpRows == 0) {
            int warpEndRow = (int)(row - round((time - targetTime) / spr));
            if (warpEndRow <= entry->row) {
                entry->endTime = time;
                entry->spr = spr;
            } else if (warpEndRow < row) {
                out.push_back(
                    {warpEndRow, targetTime, targetTime, targetTime, spr});
            }
            return {row, time, it};
        }

        // Apply the effect of the current segments.
        do {
            switch (it->type) {
                case Segment::BPM:
                    spr = SecPerRow(((const BpmChange*)it->seg)->bpm);
                    break;
                case Segment::DELAY:
                    time += ((const Delay*)it->seg)->seconds;
                    break;
                case Segment::STOP:
                    time += ((const Stop*)it->seg)->seconds;
                    break;
                case Segment::WARP:
                    warpRows += ((const Warp*)it->seg)->numRows;
                    break;
            }
        } while (++it != end && it->seg->row < row);

        // Check if the warp ended during the current segments.
        if (spr > 0 && time > targetTime && warpRows == 0) {
            if (row <= entry->row) {
                entry->endTime = time;
                entry->spr = spr;
            } else {
                out.push_back({row, targetTime, targetTime, time, spr});
            }
            return {row, time, it};
        }

        prevRow = row;
    }

    // If we reach this point, none of the segments caused the warp to end.
    spr = fabs(spr);
    int row = prevRow + warpRows;
    if (time < targetTime) row += (int)round((targetTime - time) / spr);
    out.push_back({row, targetTime, targetTime, targetTime, spr});
    return {row, targetTime, it};
}

static void CreateEvents(Vector<Event>& out, double time, MergedTS* it,
                         MergedTS* end) {
    int row = 0, warp;
    double spr = 1.0, stop, delay;
    while (it != end) {
        warp = 0;
        stop = delay = 0;

        do {
            switch (it->type) {
                case Segment::BPM:
                    spr = SecPerRow(((const BpmChange*)it->seg)->bpm);
                    break;
                case Segment::DELAY:
                    delay += ((const Delay*)it->seg)->seconds;
                    break;
                case Segment::STOP:
                    stop += ((const Stop*)it->seg)->seconds;
                    break;
                case Segment::WARP:
                    warp += ((const Warp*)it->seg)->numRows;
                    break;
            }
        } while (++it != end && it->seg->row < row);

        double rowTime = time + delay;
        double endTime = rowTime + stop;
        out.push_back({row, time, rowTime, endTime, spr});

        if (endTime < time || spr < 0 || warp > 0) {
            auto result = HandleWarp(out, it, end, warp);
            spr = out.back().spr;
            endTime = result.time;
            row = result.row;
            it = result.it;
        }

        if (it == end) break;

        time = endTime + (it->seg->row - row) * spr;
        row = it->seg->row;
    }
    if (out.empty()) {
        out.push_back({0, 0.0, 0.0, 0.0, BEATS_PER_ROW});
    }
}

// ================================================================================================
// Timing signature processing.

static void CreateTimeSigs(Vector<TimeSig>& out, const TimeSignature* it,
                           const TimeSignature* end) {
    int row = 0, measure = 0;
    while (it != end) {
        int beatsPerMeasure = max(it->rowsPerMeasure / ROWS_PER_BEAT, 1);
        int rowsPerMeasure = beatsPerMeasure * ROWS_PER_BEAT;
        out.push_back({row, measure, rowsPerMeasure});
        if (++it != end) {
            int passedMeasures =
                (it->row - row + rowsPerMeasure - 1) / rowsPerMeasure;
            row += passedMeasures * rowsPerMeasure;
            measure += passedMeasures;
            auto next = it + 1;
            while (next != end && next->row <= row) {
                ++it, ++next;
            }
        }
    }
    if (out.empty()) {
        out.push_back({0, 0, ROWS_PER_BEAT * 4});
    }
}

// ================================================================================================
// Scroll processing.

static void CreateScrollRows(Vector<ScrollRow>& out, const Scroll* it,
                             const Scroll* end) {
    int row = 0;
    double rowScroll = 0, ratio = 1;
    if (it != end) {
        row = it->row;
        rowScroll = (double)it->row;
        ratio = it->ratio;
        out.push_back({row, rowScroll, ratio});

        while (++it != end) {
            int passedRows = (it->row - row);
            rowScroll += passedRows * ratio;
            row = it->row;
            ratio = it->ratio;
            out.push_back({row, rowScroll, ratio});

            auto next = it + 1;
            while (next != end && next->row <= row) {
                ++it, ++next;
            }
        }
    }
    if (out.empty() || out.at(0).row > 0) {
        out.insert(0, {0, 0, 1}, 1);
    }
}

static void CreateScrollSpeeds(Vector<ScrollSpeed>& out, const Speed* it,
                               const Speed* end) {
    TempoTimeTracker tracker;
    double previous = 1;
    while (it != end) {
        int row = it->row;
        double rowTime = tracker.advance(row);
        double delay = it->unit == 0
                           ? (round(it->delay * ROWS_PER_BEAT) / ROWS_PER_BEAT)
                           : it->delay;
        out.push_back({row, it->unit, previous, it->ratio, delay, rowTime});
        previous = it->ratio;
        ++it;
    }
    if (out.empty() || out.at(0).row > 0) {
        out.insert(0, {0, 0, 1, 1, 0, 0}, 1);
    }
}

static void CreateScrollFakes(Vector<ScrollFake>& out, const Fake* it,
                              const Fake* end) {
    while (it != end) {
        out.push_back({it->row, it->numRows});
        ++it;
    }
}

// ================================================================================================
// Timing translation functions.

static const ScrollRow* MostRecentScrollRow(const Vector<ScrollRow>& scrolls,
                                            int row) {
    const ScrollRow *it = scrolls.begin(), *mid;
    int count = scrolls.size(), step;
    while (count > 1) {
        step = count >> 1;
        mid = it + step;
        if (mid->row <= row) {
            it = mid;
            count -= step;
        } else
            count = step;
    }
    return it;
}

static const ScrollSpeed* MostRecentScrollSpeed(
    const Vector<ScrollSpeed>& speeds, int row) {
    const ScrollSpeed *it = speeds.begin(), *mid;
    int count = speeds.size(), step;
    while (count > 1) {
        step = count >> 1;
        mid = it + step;
        if (mid->row <= row) {
            it = mid;
            count -= step;
        } else
            count = step;
    }
    return it;
}

static const TimeSig* MostRecentTimeSig(const Vector<TimeSig>& sigs, int row) {
    const TimeSig *it = sigs.begin(), *mid;
    int count = sigs.size(), step;
    while (count > 1) {
        step = count >> 1;
        mid = it + step;
        if (mid->row <= row) {
            it = mid;
            count -= step;
        } else
            count = step;
    }
    return it;
}

static const Event* MostRecentEvent(const Vector<Event>& events, int row) {
    const Event *it = events.begin(), *mid;
    int count = events.size(), step;
    while (count > 1) {
        step = count >> 1;
        mid = it + step;
        if (mid->row <= row) {
            it = mid;
            count -= step;
        } else
            count = step;
    }
    return it;
}

static const Event* MostRecentEvent(const Vector<Event>& events, double time) {
    const Event *it = events.begin(), *mid;
    int count = events.size(), step;
    while (count > 1) {
        step = count >> 1;
        mid = it + step;
        if (mid->time <= time) {
            it = mid;
            count -= step;
        } else
            count = step;
    }
    return it;
}

static double TimeToBeat(const Event* it, double time) {
    double row = it->row;
    if (time > it->endTime && it->spr > 0.0) {
        row += (time - it->endTime) / it->spr;
    }
    return row * BEATS_PER_ROW;
}

static int TimeToRow(const Event* it, double time) {
    int row = it->row;
    if (time > it->endTime && it->spr > 0.0) {
        row += (int)((time - it->endTime) / it->spr);
    }
    return row;
}

static double RowToTime(const Event* it, int row) {
    if (row > it->row) {
        return it->endTime + (row - it->row) * it->spr;
    }
    return it->rowTime;
}

static double BeatToTime(const Event* it, double beat) {
    double row = beat * ROWS_PER_BEAT;
    if (row > it->row) {
        return it->endTime + (row - it->row) * it->spr;
    }
    return it->rowTime;
}

static double BeatToMeasure(const TimeSig* sig, double beat) {
    double row = beat * ROWS_PER_BEAT;
    return sig->measure + (row - sig->row) / (double)sig->rowsPerMeasure;
}

static double RowToScroll(const ScrollRow* scroll, int row) {
    return scroll->positionRow + (row - scroll->row) * scroll->ratio;
}

static double BeatToScroll(const ScrollRow* scroll, double beat) {
    double row = beat * ROWS_PER_BEAT;
    return scroll->positionRow + (row - scroll->row) * scroll->ratio;
}

static double PositionToSpeed(const ScrollSpeed* speed, double beat,
                              double time) {
    if (speed->delay == 0) {
        return speed->end;
    }

    double strength;
    if (speed->unit == 1) {
        strength = (time - ((double)speed->rowTime)) / speed->delay;
    } else {
        strength = (beat - ((double)speed->row / ROWS_PER_BEAT)) / speed->delay;
    }

    return lerp(speed->start, speed->end, clamp(strength, 0.0, 1.0));
}

};  // anonymous namespace

// ================================================================================================
// Tempo list :: implementation.

TimingData::TimingData() {
    events.push_back({0, 0.0, 0.0, 0.0, BEATS_PER_ROW});
    sigs.push_back({0, 0, ROWS_PER_BEAT * 4});
    scrolls.push_back({0, 0, 1});
    speeds.push_back({0, 1, 1, 0, 0});
}

void TimingData::update(const Tempo* tempo) {
    // Create an event list from BPM changes, stops, delays and warps.
    Vector<MergedTS> items(128);
    auto segments = tempo->segments;
    Merge(items, segments->getList<BpmChange>());
    Merge(items, segments->getList<Stop>());
    Merge(items, segments->getList<Delay>());
    Merge(items, segments->getList<Warp>());

    events.clear();
    CreateEvents(events, -tempo->offset, items.begin(), items.end());
    events.squeeze();

    // Create a measure list from time signatures.
    sigs.clear();
    CreateTimeSigs(sigs, segments->begin<TimeSignature>(),
                   segments->end<TimeSignature>());
    sigs.squeeze();

    // Create a scroll offset list from the scroll ratios.
    scrolls.clear();
    CreateScrollRows(scrolls, segments->begin<Scroll>(),
                     segments->end<Scroll>());
    scrolls.squeeze();

    // Create a scroll speed list.
    speeds.clear();
    CreateScrollSpeeds(speeds, segments->begin<Speed>(),
                       segments->end<Speed>());
    speeds.squeeze();

    // Create a scroll fake region list.
    fakes.clear();
    CreateScrollFakes(fakes, segments->begin<Fake>(), segments->end<Fake>());
    fakes.squeeze();
}

double TimingData::timeToBeat(double time) const {
    return TimeToBeat(MostRecentEvent(events, time), time);
}

int TimingData::timeToRow(double time) const {
    return TimeToRow(MostRecentEvent(events, time), time);
}

double TimingData::rowToTime(int row) const {
    return RowToTime(MostRecentEvent(events, row), row);
}

double TimingData::beatToTime(double beat) const {
    int row = (int)ceil(beat * ROWS_PER_BEAT);
    return BeatToTime(MostRecentEvent(events, row), beat);
}

double TimingData::beatToMeasure(double beat) const {
    int row = (int)ceil(beat * ROWS_PER_BEAT);
    return BeatToMeasure(MostRecentTimeSig(sigs, row), beat);
}

double TimingData::rowToScroll(int row) const {
    return RowToScroll(MostRecentScrollRow(scrolls, row), row);
}

double TimingData::beatToScroll(double beat) const {
    int row = (int)floor(beat * ROWS_PER_BEAT);  // floor matches games better?
    return BeatToScroll(MostRecentScrollRow(scrolls, row), beat);
}

double TimingData::positionToSpeed(double beat, double time) const {
    int row = (int)ceil(beat * ROWS_PER_BEAT);
    return PositionToSpeed(MostRecentScrollSpeed(speeds, row), beat, time);
}

// ================================================================================================
// TempoTimeTracker.

TempoTimeTracker::TempoTimeTracker()
    : TempoTimeTracker(gTempo->getTimingData()) {}

TempoTimeTracker::TempoTimeTracker(const TimingData& data)
    : it(data.events.begin()), end(data.events.end()) {
    VortexAssert(it != end);
    auto next = it + 1;
    if (next != end) {
        nextRow = next->row;
    } else {
        nextRow = INT_MAX;
    }
}

double TempoTimeTracker::advance(int row) {
    while (row >= nextRow) {
        auto next = it + 1;
        if (next == end) break;
        it = next;
        next = it + 1;
        if (next != end) {
            nextRow = next->row;
        } else {
            nextRow = INT_MAX;
        }
    }
    return RowToTime(it, row);
}

double TempoTimeTracker::lookAhead(int row) const {
    auto tmpIt = it;
    auto tmpNextRow = nextRow;
    while (row >= tmpNextRow) {
        auto next = tmpIt + 1;
        if (next == end) break;
        tmpIt = next;
        next = tmpIt + 1;
        if (next != end) {
            tmpNextRow = next->row;
        } else {
            tmpNextRow = INT_MAX;
        }
    }
    return RowToTime(tmpIt, row);
}

// ================================================================================================
// TempoRowTracker.

TempoRowTracker::TempoRowTracker() : TempoRowTracker(gTempo->getTimingData()) {}

TempoRowTracker::TempoRowTracker(const TimingData& data)
    : it(data.events.begin()), end(data.events.end()) {
    VortexAssert(it != end);
    auto next = it + 1;
    if (next != end) {
        nextTime = next->time;
    } else {
        nextTime = DBL_MAX;
    }
}

int TempoRowTracker::advance(double time) {
    while (time >= nextTime) {
        auto next = it + 1;
        if (next == end) break;
        it = next;
        next = it + 1;
        if (next != end) {
            nextTime = next->time;
        } else {
            nextTime = DBL_MAX;
        }
    }
    return TimeToRow(it, time);
}

int TempoRowTracker::lookAhead(double time) const {
    auto tmpIt = it;
    auto tmpNextTime = nextTime;
    while (time >= tmpNextTime) {
        auto next = it + 1;
        if (next == end) break;
        tmpIt = next;
        next = it + 1;
        if (next != end) {
            tmpNextTime = next->time;
        } else {
            tmpNextTime = DBL_MAX;
        }
    }
    return TimeToRow(tmpIt, time);
}

};  // namespace Vortex