#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/Common/Event.h>

#include <Simfile/Common.h>
#include <Simfile/Tempo/TimingData.h>
#include <Simfile/Tempo/SegmentSet.h>

namespace AV {

// Row/beat conversion constants.

constexpr double RowsPerBeat = 48.0;
constexpr double BeatsPerRow = 1.0 / 48.0;

// Converts a BPM value to seconds per row.
constexpr double SecPerRow(double beatsPerMin)
{
	return 60.0 / (beatsPerMin * RowsPerBeat);
}

// Converts seconds per row to a BPM value.
constexpr double BeatsPerMin(double secPerRow)
{
	return 60.0 / (secPerRow * RowsPerBeat);
}

// Different ways to display the song BPM.
enum class DisplayBpmType { Actual, Random, Custom };

// Represents a minimum and maximum BPM value.
struct BpmRange
{
	BpmRange(double min, double max);

	double min;
	double max;
};

// Represents a display BPM type and range.
struct DisplayBpm
{
	DisplayBpm(double low, double high, DisplayBpmType type);

	double low;
	double high;
	DisplayBpmType type;

	static const DisplayBpm actual;
	static const DisplayBpm random;

	auto operator <=> (const DisplayBpm&) const = default;
};

// Unit used for attack timing.
enum class AttackUnit { Length, End };

// Represents an attack (SM5).
struct Attack
{
	double time = 0.0;
	double duration = 0.0;
	std::string mods;
	AttackUnit unit = AttackUnit::Length;
};

// Holds data that determines the tempo of a song or chart (and a few other things).
class Tempo : NonCopyable
{
public:
	~Tempo();

	Tempo(Chart* chart);
	Tempo(Simfile* simfile);

	// Sanitizes the segments and makes sure there is a BPM change at row zero.
	void sanitize();

	// Updates the timing data based on the current segments and offset.
	void updateTiming();

	// Returns the BPM at the given row.
	double getBpm(Row row) const;

	// Returns true if the two tempo objects are equivalent.
	bool isEquivalent(const Tempo& other);

	// Returns true if the tempo contains one or more segments, false otherwise.
	bool hasSegments() const;

// EVENTS:

	struct OffsetChanged : Event {};
	struct DisplayBpmChanged : Event {};
	struct MetadataChanged : Event {}; // One of the metadata properties changed.
	struct TimingChanged : Event {};   // One or more properties that affect timing changed.
	struct SegmentsChanged : Event {}; // One or more segments have changed.

// PUBLIC MEMBERS:

	// Seconds delay between the first beat and the music start.
	// A negative offset means the music starts before the first beat.
	Observable<double> offset;

	// Attacks (i.e. modifiers, notefield effects).
	vector<Attack> attacks;

	// Segment tracks.
	BpmChangeSegment::Track bpmChanges;
	StopSegment::Track stops;
	DelaySegment::Track delays;
	WarpSegment::Track warps;
	ScrollSegment::Track scrolls;
	TimeSignatureSegment::Track timeSignatures;
	TickCountSegment::Track tickCounts;
	ComboSegment::Track combos;
	SpeedSegment::Track speeds;
	FakeSegment::Track fakes;
	LabelSegment::Track labels;

	// Timing based on segments.
	TimingData timing;

	// The display BPM range and type.
	Observable<DisplayBpm> displayBpm;

	// Refers back to the chart that the tempo is part of.
	// If the tempo is the simfile tempo, chart is null.
	const Chart* const chart;

	// Refers back to the simfile that the tempo is part of.
	const Simfile* const simfile;
};

} // namespace AV
