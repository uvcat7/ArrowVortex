#include <Simfile/SegmentGroup.h>

#include <Core/StringUtils.h>
#include <Core/ByteStream.h>
#include <Core/Draw.h>

#include <Simfile/Common.h>
#include <Simfile/Tempo.h>

#include <new>
#include <math.h>

namespace Vortex {

// ================================================================================================
// Low level segment functions.

template <typename T>
void Encode(WriteStream& out, const T& seg);

template <typename T>
void Decode(ReadStream& in, T& seg);

template <typename T>
bool IsRedundant(const T& seg, const T* prev);

template <typename T>
bool IsEquivalent(const T& seg, const T& other);

template <typename T>
String GetDescription(const T& seg);

// ================================================================================================
// Segment wrapper functions.

template <typename T>
static void WrapNew(Segment* seg)
{
	new ((T*)seg) T();
}

template <typename T>
static void WrapDel(Segment* seg)
{
	(*(T*)seg).~T();
}

template <typename T>
static void WrapCpy(Segment* seg, const Segment* src)
{
	*((T*)seg) = *((const T*)src);
}

template <typename T>
static void WrapEnc(WriteStream& out, const Segment* seg)
{
	out.write(seg->row);
	Encode<T>(out, *(const T*)seg);
}

template <typename T>
static void WrapDec(ReadStream& in, SegmentGroup* out)
{
	T seg;
	in.read(seg.row);
	Decode<T>(in, seg);
	out->append(seg);
}

template <typename T>
static bool WrapRed(const Segment* seg, const Segment* prev)
{
	return IsRedundant<T>(*(const T*)seg, (const T*)prev);
}

template <typename T>
static bool WrapEqu(const Segment* seg, const Segment* other)
{
	return IsEquivalent<T>(*(const T*)seg, *(const T*)other);
}

template <typename T>
static String WrapDsc(const Segment* seg)
{
	return GetDescription<T>(*(const T*)seg);
}

#define WRAP(x)\
	WrapNew<x>,\
	WrapDel<x>,\
	WrapCpy<x>,\
	WrapDec<x>,\
	WrapEnc<x>,\
	WrapRed<x>,\
	WrapEqu<x>,\
	WrapDsc<x>

// ================================================================================================
// Segment.

Segment::Segment()
	: row(0)
{
}

Segment::Segment(int row)
	: row(row)
{
}

// ================================================================================================
// BpmChange.

BpmChange::BpmChange()
	: bpm(0)
{
}

BpmChange::BpmChange(int row, double bpm)
	: Segment(row), bpm(bpm)
{
}

template <>
void Encode(WriteStream& out, const BpmChange& seg)
{
	out.write(seg.bpm);
}

template <>
void Decode(ReadStream& in, BpmChange& seg)
{
	in.read(seg.bpm);
}

template <>
bool IsRedundant(const BpmChange& seg, const BpmChange* prev)
{
	//return (prev && prev->bpm == seg.bpm); -- original code before Visual Sync commit
	return prev && prev->row == seg.row;
}

template <>
bool IsEquivalent(const BpmChange& seg, const BpmChange& other)
{
	return (seg.bpm == other.bpm);
}

template <>
String GetDescription(const BpmChange& seg)
{
	return Str::val(seg.bpm, 3, 6);
}

static const SegmentMeta BpmChangeMeta =
{
	sizeof(BpmChange),
	"BPM change",
	"BPM changes",
	"Beats per minute",
	RGBAtoColor32(128, 26, 26, 255),
	SegmentMeta::RIGHT,
	WRAP(BpmChange)
};

// ================================================================================================
// Stop.

Stop::Stop()
	: seconds(0)
{
}

Stop::Stop(int row, double seconds)
	: Segment(row), seconds(seconds)
{
}

template <>
void Encode(WriteStream& out, const Stop& seg)
{
	out.write(seg.seconds);
}

template <>
void Decode(ReadStream& in, Stop& seg)
{
	in.read(seg.seconds);
}

template <>
bool IsRedundant(const Stop& seg, const Stop* prev)
{
	return (fabs(seg.seconds) < 0.0005) || (prev && prev->row == seg.row);
}

template <>
bool IsEquivalent(const Stop& seg, const Stop& other)
{
	return (seg.seconds == other.seconds);
}

template <>
String GetDescription(const Stop& seg)
{
	return Str::val(seg.seconds, 3, 3);
}

static const SegmentMeta StopMeta =
{
	sizeof(Stop),
	"Stop",
	"Stops",
	"Duration (seconds)",
	RGBAtoColor32(128, 128, 51, 255),
	SegmentMeta::LEFT,
	WRAP(Stop)
};

// ================================================================================================
// Delay.

Delay::Delay()
	: seconds(0)
{
}

Delay::Delay(int row, double seconds)
	: Segment(row), seconds(seconds)
{
}

template <>
void Encode(WriteStream& out, const Delay& seg)
{
	out.write(seg.seconds);
}

template <>
void Decode(ReadStream& in, Delay& seg)
{
	in.read(seg.seconds);
}

template <>
bool IsRedundant(const Delay& seg, const Delay* prev)
{
	return (fabs(seg.seconds) < 0.0005) || (prev && prev->row == seg.row);
}

template <>
bool IsEquivalent(const Delay& seg, const Delay& other)
{
	return (seg.seconds == other.seconds);
}

template <>
String GetDescription(const Delay& seg)
{
	return Str::val(seg.seconds, 3, 3);
}

static const SegmentMeta DelayMeta =
{
	sizeof(Delay),
	"Delay",
	"Delays",
	"Duration (seconds)",
	RGBAtoColor32(26, 128, 128, 255),
	SegmentMeta::LEFT,
	WRAP(Delay)
};

// ================================================================================================
// Warp.

Warp::Warp()
	: numRows(0)
{
}

Warp::Warp(int row, int numRows)
	: Segment(row), numRows(numRows)
{
}

template <>
void Encode(WriteStream& out, const Warp& seg)
{
	out.write(seg.numRows);
}

template <>
void Decode(ReadStream& in, Warp& seg)
{
	in.read(seg.numRows);
}

template <>
bool IsRedundant(const Warp& seg, const Warp* prev)
{
	return (seg.numRows == 0) || (prev && prev->row == seg.row);
}

template <>
bool IsEquivalent(const Warp& seg, const Warp& other)
{
	return (seg.numRows == other.numRows);
}

template <>
String GetDescription(const Warp& seg)
{
	return Str::val(seg.numRows * BEATS_PER_ROW, 3, 3);
}

static const SegmentMeta WarpMeta =
{
	sizeof(Warp),
	"Warp",
	"Warps",
	"Duration (beats)",
	RGBAtoColor32(128, 26, 51, 255),
	SegmentMeta::RIGHT,
	WRAP(Warp)
};

// ================================================================================================
// TimeSignature.

TimeSignature::TimeSignature()
	: rowsPerMeasure(192), beatNote(4)
{
}

TimeSignature::TimeSignature(int row, int rowsPerMeasure, int beatNote)
	: Segment(row), rowsPerMeasure(rowsPerMeasure), beatNote(beatNote)
{
}

template <>
void Encode(WriteStream& out, const TimeSignature& seg)
{
	out.write(seg.rowsPerMeasure);
	out.write(seg.beatNote);
}

template <>
void Decode(ReadStream& in, TimeSignature& seg)
{
	in.read(seg.rowsPerMeasure);
	in.read(seg.beatNote);
}

template <>
String GetDescription(const TimeSignature& seg)
{
	int beatsPerMeasure = seg.rowsPerMeasure / ROWS_PER_BEAT;
	return Str::fmt("%1/%2").arg(beatsPerMeasure).arg(seg.beatNote);
}

template <>
bool IsRedundant(const TimeSignature& seg, const TimeSignature* prev)
{
	return (prev && prev->rowsPerMeasure == seg.rowsPerMeasure && prev->beatNote == seg.beatNote);
}

template <>
bool IsEquivalent(const TimeSignature& seg, const TimeSignature& other)
{
	return (other.rowsPerMeasure == seg.rowsPerMeasure && other.beatNote == seg.beatNote);
}

static const SegmentMeta TimeSignatureMeta =
{
	sizeof(TimeSignature),
	"Time signature",
	"Time signatures",
	"Beats per measure\nBeat note type",
	RGBAtoColor32(128, 102, 26, 255),
	SegmentMeta::LEFT,
	WRAP(TimeSignature)
};

// ================================================================================================
// TickCount.

TickCount::TickCount()
	: ticks(0)
{
}

TickCount::TickCount(int row, int ticks)
	: Segment(row), ticks(ticks)
{
}

template <>
void Encode(WriteStream& out, const TickCount& seg)
{
	out.write(seg.ticks);
}

template <>
void Decode(ReadStream& in, TickCount& seg)
{
	in.read(seg.ticks);
}

template <>
String GetDescription(const TickCount& seg)
{
	return Str::val(seg.ticks);
}

template <>
bool IsRedundant(const TickCount& seg, const TickCount* prev)
{
	return (prev && prev->ticks == seg.ticks);
}

template <>
bool IsEquivalent(const TickCount& seg, const TickCount& other)
{
	return (other.ticks == seg.ticks);
}

static const SegmentMeta TickCountMeta =
{
	sizeof(TickCount),
	"Tick count",
	"Tick counts",
	"Hold ticks per beat",
	RGBAtoColor32(51, 128, 26, 255),
	SegmentMeta::RIGHT,
	WRAP(TickCount)
};

// ================================================================================================
// Combo.

Combo::Combo()
	: hitCombo(0), missCombo(0)
{
}

Combo::Combo(int row, int hit, int miss)
	: Segment(row), hitCombo(hit), missCombo(miss)
{
}

template <>
void Encode(WriteStream& out, const Combo& seg)
{
	out.write(seg.hitCombo);
	out.write(seg.missCombo);
}

template <>
void Decode(ReadStream& in, Combo& seg)
{
	in.read(seg.hitCombo);
	in.read(seg.missCombo);
}

template <>
String GetDescription(const Combo& seg)
{
	return Str::fmt("%1/%2").arg(seg.hitCombo).arg(seg.missCombo);
}

template <>
bool IsRedundant(const Combo& seg, const Combo* prev)
{
	return (prev && prev->hitCombo == seg.hitCombo && prev->missCombo == seg.missCombo);
}

template <>
bool IsEquivalent(const Combo& seg, const Combo& other)
{
	return (other.hitCombo == seg.hitCombo && other.missCombo == seg.missCombo);
}

static const SegmentMeta ComboMeta =
{
	sizeof(Combo),
	"Combo segment",
	"Combo segments",
	"Hit multiplier\nMiss multiplier",
	RGBAtoColor32(51, 102, 26, 255),
	SegmentMeta::RIGHT,
	WRAP(Combo)
};

// ================================================================================================
// Speed.

Speed::Speed()
	: ratio(1), delay(0), unit(0)
{
}

Speed::Speed(int row, double ratio, double delay, int unit)
	: Segment(row), ratio(ratio), delay(delay), unit(unit)
{
}

template <>
void Encode(WriteStream& out, const Speed& seg)
{
	out.write(seg.ratio);
	out.write(seg.delay);
	out.write(seg.unit);
}

template <>
void Decode(ReadStream& in, Speed& seg)
{
	in.read(seg.ratio);
	in.read(seg.delay);
	in.read(seg.unit);
}

template <>
String GetDescription(const Speed& seg)
{
	return Str::fmt("%1/%2/%3").arg(seg.ratio).arg(seg.delay).arg(seg.unit ? 'T' : 'B');
}

template <>
bool IsRedundant(const Speed& seg, const Speed* prev)
{
	return (prev && prev->ratio == seg.ratio && prev->delay == seg.delay && prev->unit == seg.unit);
}

template <>
bool IsEquivalent(const Speed& seg, const Speed& other)
{
	return (other.ratio == seg.ratio && other.delay == seg.delay && other.unit == seg.unit);
}

static const SegmentMeta SpeedMeta =
{
	sizeof(Speed),
	"Speed segment",
	"Speed segments",
	"Stretch ratio\nDelay time\nUnit (beats/time)",
	RGBAtoColor32(26, 51, 51, 255),
	SegmentMeta::RIGHT,
	WRAP(Speed)
};

// ================================================================================================
// Scroll.

Scroll::Scroll()
	: ratio(1)
{
}

Scroll::Scroll(int row, double ratio)
	: Segment(row), ratio(ratio)
{
}

template <>
void Encode(WriteStream& out, const Scroll& seg)
{
	out.write(seg.ratio);
}

template <>
void Decode(ReadStream& in, Scroll& seg)
{
	in.read(seg.ratio);
}

template <>
String GetDescription(const Scroll& seg)
{
	return Str::val(seg.ratio);
}

template <>
bool IsRedundant(const Scroll& seg, const Scroll* prev)
{
	return (prev && prev->ratio == seg.ratio);
}

template <>
bool IsEquivalent(const Scroll& seg, const Scroll& other)
{
	return (other.ratio == seg.ratio);
}

static const SegmentMeta ScrollMeta =
{
	sizeof(Scroll),
	"Scroll segment",
	"Scroll segments",
	"Scroll rate multiplier",
	RGBAtoColor32(51, 102, 102, 255),
	SegmentMeta::LEFT,
	WRAP(Scroll)
};

// ================================================================================================
// Fake.

Fake::Fake()
	: numRows(0)
{
}

Fake::Fake(int row, int numRows)
	: Segment(row), numRows(numRows)
{
}

template <>
void Encode(WriteStream& out, const Fake& seg)
{
	out.write(seg.numRows);
}

template <>
void Decode(ReadStream& in, Fake& seg)
{
	in.read(seg.numRows);
}

template <>
String GetDescription(const Fake& seg)
{
	return Str::val(seg.numRows * BEATS_PER_ROW, 3, 3);
}

template <>
bool IsRedundant(const Fake& seg, const Fake* prev)
{
	return (seg.numRows == 0) || (prev && prev->row == seg.row);
}

template <>
bool IsEquivalent(const Fake& seg, const Fake& other)
{
	return (other.numRows == seg.numRows);
}

const SegmentMeta FakeMeta =
{
	sizeof(Fake),
	"Fake segment",
	"Fake segments",
	"Duration (beats)",
	RGBAtoColor32(128, 128, 51, 255),
	SegmentMeta::LEFT,
	WRAP(Fake)
};

// ================================================================================================
// Label.

Label::Label()
{
}

Label::Label(int row, String str)
	: Segment(row), str(str)
{
}

template <>
void Encode(WriteStream& out, const Label& seg)
{
	out.writeStr(seg.str);
}

template <>
void Decode(ReadStream& in, Label& seg)
{
	in.readStr(seg.str);
}

template <>
String GetDescription(const Label& seg)
{
	return seg.str;
}

template <>
bool IsRedundant(const Label& seg, const Label* prev)
{
	return seg.str.empty() || (prev && prev->row == seg.row);
}

template <>
bool IsEquivalent(const Label& seg, const Label& other)
{
	return (other.str == seg.str);
}

const SegmentMeta LabelMeta =
{
	sizeof(Label),
	"Label",
	"Labels",
	"Description",
	RGBAtoColor32(102, 51, 51, 255),
	SegmentMeta::RIGHT,
	WRAP(Label)
};

// ================================================================================================
// SegmentMeta.

const SegmentMeta* Segment::meta[NUM_TYPES] =
{
	&BpmChangeMeta,
	&StopMeta,
	&DelayMeta,
	&WarpMeta,
	&TimeSignatureMeta,
	&TickCountMeta,
	&ComboMeta,
	&SpeedMeta,
	&ScrollMeta,
	&FakeMeta,
	&LabelMeta,
};

}; // namespace Vortex
