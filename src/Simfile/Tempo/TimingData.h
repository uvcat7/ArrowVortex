#pragma once

#include <Precomp.h>

namespace AV {

// =====================================================================================================================
// Timing data.

class TimingData
{
public:
	struct Event
	{
		Event(Row row, double startTime, double hitTime, double endTime, double spr)
			: row(row), startTime(startTime), hitTime(hitTime), endTime(endTime), spr(spr) {}

		Row row;
		double startTime;
		double hitTime;
		double endTime;
		double spr;
	};

	struct Signature
	{
		Signature(Row row, int measure, Row rowsPerMeasure)
			: row(row), measure(measure), rowsPerMeasure(rowsPerMeasure) {}

		Row row;
		int measure;
		Row rowsPerMeasure;
	};

	// Tracks the startTime of incremental row values.
	struct TimeTracker
	{
		TimeTracker(const TimingData& data);

		// Advances the current row, and returns the startTime corresponding to that row.
		double advance(Row row);

		// Looks at a future row and returns the startTime corresponding to that row.
		double lookAhead(Row row) const;

		// Rewind back to the initial state.
		void reset();

		const Event* it;
		const Event* end;
		int nextRow;
		bool warping;
	};

	// Tracks the row of incremental startTime values.
	struct RowTracker
	{
		RowTracker(const TimingData& data);

		// Advances the current startTime, and returns the row corresponding to that startTime.
		double advance(double time);

		// Looks at a future startTime and returns the row corresponding to that startTime.
		double lookAhead(double time) const;

		// Rewind back to the initial state.
		void reset();

		const Event* it;
		const Event* end;
		double nextTime;
		bool warping;
	};

	TimingData();

	// Updates the timings and signatures based on the given tempo.
	void update(double offset, const Tempo& tempo);

	// Returns the row corresponding to the given startTime.
	double timeToRow(double time) const;

	// Returns the startTime corresponding to the given row.
	double rowToTime(double row) const;

	// Returns the measure corresponding to the given row.
	double rowToMeasure(double row) const;

	// Returns a startTime tracker for the current timing data.
	TimeTracker timeTracker() const;

	// Returns a row tracker for the current timing data.
	RowTracker rowTracker() const;

	// Returns a representation of stops/BPM changes optimized for timing calculations.
	const vector<Event>& events() const;

	// Returns a representation of startTime signatures optimized for timing calculations.
	const vector<Signature>& signatures() const;

private:
	vector<Event> myEvents;
	vector<Signature> mySignatures;
};

} // namespace AV
