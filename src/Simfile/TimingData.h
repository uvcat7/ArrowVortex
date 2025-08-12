#pragma once

#include <Core/Vector.h>

namespace Vortex {

// ================================================================================================
// Timing data.

struct TimingData {
    struct Event {
        int row;
        double time, rowTime, endTime, spr;
    };
    struct TimeSig {
        int row, measure, rowsPerMeasure;
    };
    struct ScrollRow {
        int row;
        double positionRow, ratio;
    };
    struct ScrollSpeed {
        int row;
        int unit;
        double start, end, delay, rowTime;
    };
    struct ScrollFake {
        int row, length;
    };

    TimingData();

    void update(const Tempo* tempo);

    // Returns the row corresponding to the given time.
    int timeToRow(double time) const;

    // Returns the beat corresponding to the given time.
    double timeToBeat(double time) const;

    // Returns the time corresponding to the given row.
    double rowToTime(int row) const;

    // Returns the time corresponding to the given beat.
    double beatToTime(double beat) const;

    // Returns the measure corresponding to the given beat.
    double beatToMeasure(double beat) const;

    /// Returns the translated row at a given row.
    double rowToScroll(int row) const;

    /// Returns the translated beat at a given beat.
    double beatToScroll(double beat) const;

    /// Returns the Speed value at a given position. Both values are needed as
    /// speed can use beats or time.
    double positionToSpeed(double beat, double time) const;

    Vector<Event> events;
    Vector<TimeSig> sigs;
    Vector<ScrollRow> scrolls;
    Vector<ScrollSpeed> speeds;
    Vector<ScrollFake> fakes;
};

// ================================================================================================
// Tempo trackers.

struct TempoTimeTracker {
    // Constructs a tracker from the timing data of the active chart.
    TempoTimeTracker();

    // Constructs a tracker from the given timing data.
    TempoTimeTracker(const TimingData& data);

    // Advances the current row, and returns the time corresponding to that row.
    double advance(int row);

    // Looks ahead at a future row and returns the time corresponding to that
    // row.
    double lookAhead(int row) const;

    const TimingData::Event *it, *end;
    int nextRow;
};

struct TempoRowTracker {
    // Constructs a tracker from the timing data of the active chart.
    TempoRowTracker();

    // Constructs a tracker from the given timing data.
    TempoRowTracker(const TimingData& data);

    // Advances the current time, and returns the row corresponding to that
    // time.
    int advance(double time);

    // Looks ahead at a future time and returns the row corresponding to that
    // time.
    int lookAhead(double time) const;

    const TimingData::Event *it, *end;
    double nextTime;
};

};  // namespace Vortex