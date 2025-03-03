#pragma once

#include <Simfile/Tempo/SegmentTrack.h>

namespace AV {

// Flags that indicate which properties of the tempo have changed.
enum class SegmentType
{
	BpmChange     = 1 << 0,
	Scroll        = 1 << 1,
	Speed         = 1 << 2,
	TimeSignature = 1 << 3,
	TickCount     = 1 << 4,
	Combo         = 1 << 5,
	Stop          = 1 << 6,
	Delay         = 1 << 7,
	Warp          = 1 << 8,
	Fake          = 1 << 9,
	Label         = 1 << 10,
};

// =====================================================================================================================
// Persistent segments.

// Represents a BPM change.
struct BpmChangeSegment
{
	double bpm;

	static constexpr SegmentType type = SegmentType::BpmChange;

	static const BpmChangeSegment fallback;

	auto operator <=> (const BpmChangeSegment&) const = default;

	typedef PersistentSegmentTrack<BpmChangeSegment> Track;
};

// Represents a scroll segment (SM5).
struct ScrollSegment
{
	double ratio;

	static constexpr SegmentType type = SegmentType::Scroll;

	static const ScrollSegment fallback;

	auto operator <=> (const ScrollSegment&) const = default;

	typedef PersistentSegmentTrack<ScrollSegment> Track;
};

// Represents a speed segment (SM5).
struct SpeedSegment
{
	double ratio;
	double delay;
	int unit;

	static constexpr SegmentType type = SegmentType::Speed;

	static const SpeedSegment fallback;

	auto operator <=> (const SpeedSegment&) const = default;

	typedef PersistentSegmentTrack<SpeedSegment> Track;
};

// Represents a startTime signature (SM5).
struct TimeSignatureSegment
{
	int beatsPerMeasure;
	int beatNote;

	static constexpr SegmentType type = SegmentType::TimeSignature;

	static const TimeSignatureSegment fallback;

	auto operator <=> (const TimeSignatureSegment&) const = default;

	typedef PersistentSegmentTrack<TimeSignatureSegment> Track;
};

// Represents a tick count (SM5).
struct TickCountSegment
{
	int ticks;

	static constexpr SegmentType type = SegmentType::TickCount;

	static const TickCountSegment fallback;

	auto operator <=> (const TickCountSegment&) const = default;

	typedef PersistentSegmentTrack<TickCountSegment> Track;
};

// Represents a combo segment (SM5).
struct ComboSegment
{
	int hitCombo;
	int missCombo;

	static constexpr SegmentType type = SegmentType::Combo;

	static const ComboSegment fallback;

	auto operator <=> (const ComboSegment&) const = default;

	typedef PersistentSegmentTrack<ComboSegment> Track;
};

// =====================================================================================================================
// Isolated segments.

// Represents a stop.
struct StopSegment
{
	double seconds;

	static constexpr SegmentType type = SegmentType::Stop;

	static const StopSegment none;

	auto operator <=> (const StopSegment&) const = default;

	typedef IsolatedSegmentTrack<StopSegment> Track;
};

// Represents a delay (SM5).
struct DelaySegment
{
	double seconds;

	static constexpr SegmentType type = SegmentType::Delay;

	static const DelaySegment none;

	auto operator <=> (const DelaySegment&) const = default;

	typedef IsolatedSegmentTrack<DelaySegment> Track;
};

// Represents a warp segment (SM5).
struct WarpSegment
{
	int numRows;

	static constexpr SegmentType type = SegmentType::Warp;

	static const WarpSegment none;

	auto operator <=> (const WarpSegment&) const = default;

	typedef IsolatedSegmentTrack<WarpSegment> Track;
};

// Represents a fake segment (SM5).
struct FakeSegment
{
	int numRows;

	static constexpr SegmentType type = SegmentType::Fake;

	static const FakeSegment none;

	auto operator <=> (const FakeSegment&) const = default;

	typedef IsolatedSegmentTrack<FakeSegment> Track;
};

// Represents a label (SM5).
struct LabelSegment
{
	std::string str;

	static constexpr SegmentType type = SegmentType::Label;

	static const LabelSegment none;

	auto operator <=> (const LabelSegment&) const = default;

	typedef IsolatedSegmentTrack<LabelSegment> Track;
};

} // namespace AV
