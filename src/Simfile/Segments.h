#pragma once

namespace Vortex {

struct Segment;

/// Contains metadata and functions of a specific segment type.
struct SegmentMeta {
    typedef void (*New)(Segment* seg);
    typedef void (*Del)(Segment* seg);
    typedef void (*Cpy)(Segment* seg, const Segment* src);

    typedef void (*Dec)(ReadStream& in, SegmentGroup* out);
    typedef void (*Enc)(WriteStream& out, const Segment* seg);

    typedef bool (*Red)(const Segment* seg, const Segment* prev);
    typedef bool (*Equ)(const Segment* seg, const Segment* other);
    typedef std::string (*Dsc)(const Segment* seg);

    enum DisplaySide { LEFT, RIGHT };

    int stride;
    const char* singular;
    const char* plural;
    const char* help;
    uint32_t color;
    DisplaySide side;

    New construct;
    Del destruct;
    Cpy copy;

    Dec decode;
    Enc encode;

    Red isRedundant;
    Equ isEquivalent;
    Dsc getDescription;
};

/// Base class for all segments.
struct Segment {
    enum Type {
        BPM,
        STOP,
        DELAY,
        WARP,
        TIME_SIG,
        TICK_COUNT,
        COMBO,
        SPEED,
        SCROLL,
        FAKE,
        LABEL,
        NUM_TYPES
    };

    static const SegmentMeta* meta[NUM_TYPES];

    Segment();
    Segment(int row);

    int row;
};

/// Represents a BPM change.
struct BpmChange : public Segment {
    enum { TYPE = BPM };

    BpmChange();
    BpmChange(int row, double bpm);

    double bpm;
};

/// Represents a stop.
struct Stop : public Segment {
    enum { TYPE = STOP };

    Stop();
    Stop(int row, double seconds);

    double seconds;
};

/// Represents a delay (SM5).
struct Delay : public Segment {
    enum { TYPE = DELAY };

    Delay();
    Delay(int row, double seconds);

    double seconds;
};

/// Represents a warp segment (SM5).
struct Warp : public Segment {
    enum { TYPE = WARP };

    Warp();
    Warp(int row, int numRows);

    int numRows;
};

/// Represents a time signature (SM5).
struct TimeSignature : public Segment {
    enum { TYPE = TIME_SIG };

    TimeSignature();
    TimeSignature(int row, int rowsPerMeasure, int beatNote);

    int rowsPerMeasure, beatNote;
};

/// Represents a tick count (SM5).
struct TickCount : public Segment {
    enum { TYPE = TICK_COUNT };

    TickCount();
    TickCount(int row, int ticks);

    int ticks;
};

/// Represents a combo segment (SM5).
struct Combo : public Segment {
    enum { TYPE = COMBO };

    Combo();
    Combo(int row, int hit, int miss);

    int hitCombo, missCombo;
};

/// Represents a speed segment (SM5).
struct Speed : public Segment {
    enum { TYPE = SPEED };

    Speed();
    Speed(int row, double ratio, double delay, int unit);

    double ratio, delay;
    int unit;
};

/// Represents a scroll segment (SM5).
struct Scroll : public Segment {
    enum { TYPE = SCROLL };

    Scroll();
    Scroll(int row, double ratio);

    double ratio;
};

/// Represents a fake segment (SM5).
struct Fake : public Segment {
    enum { TYPE = FAKE };

    Fake();
    Fake(int row, int numRows);

    int numRows;
};

/// Represents a label (SM5).
struct Label : public Segment {
    enum { TYPE = LABEL };

    Label();
    Label(int row, std::string str);

    std::string str;
};

};  // namespace Vortex