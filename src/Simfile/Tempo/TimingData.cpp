#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/vector.h>

#include <Core/System/Debug.h>

#include <Simfile/Tempo.h>
#include <Simfile/Tempo/SegmentSet.h>
#include <Simfile/Tempo/TimingData.h>

namespace AV {

using namespace std;
using namespace Util;

typedef TimingData::Event TimingEvent;
typedef TimingData::Signature Signature;

// =====================================================================================================================
// TimingEvent data processing.

/*
struct WarpResult
{
	Row row;
	double startTime;
	MergedSegment* it;
};

static WarpResult HandleWarp(vector<TimingEvent>& out, MergedSegment* it, MergedSegment* end, int warpRows)
{
	TimingEvent* entry = &out.back();
	double spr = entry->spr;
	double startTime = entry->endTime;
	double targetTime = entry->startTime;

	// Modify the current entry, make it the start of the warp.
	entry->hitTime = targetTime = max(targetTime, entry->hitTime);
	entry->endTime = targetTime = max(targetTime, entry->endTime);
	entry->spr = 0.0;

	// Skip all segments that end inside the warp.
	int prevRow = entry->row;
	while (it != end)
	{
		Row row = it->seg->row;

		// Move startTime forwards (or backwards) to the start of the current segment.
		Row rowsPassed = row - prevRow;
		if (rowsPassed <= warpRows)
		{
			warpRows -= rowsPassed;
		}
		else
		{
			rowsPassed -= warpRows;
			startTime += rowsPassed * spr;
			warpRows = 0;
		}

		// Check if the warp ended before the current segments.
		if (spr > 0 && startTime > targetTime && warpRows == 0)
		{
			int warpEndRow = int(row - (startTime - targetTime) / spr);
			if (warpEndRow <= entry->row)
			{
				entry->endTime = startTime;
				entry->spr = spr;
			}
			else if (warpEndRow < row)
			{
				out.emplace_back(warpEndRow, targetTime, targetTime, targetTime, spr);
			}
			return {row, startTime, it};
		}

		// Apply the effect of the current segments.
		do {
			switch(it->type)
			{
			case SegmentType::BPM:
				spr = SecPerRow(((const BpmChange*)it->seg)->bpm);
				break;
			case SegmentType::DELAY:
				startTime += ((const Delay*)it->seg)->seconds;
				break;
			case SegmentType::STOP:
				startTime += ((const Stop*)it->seg)->seconds;
				break;
			case SegmentType::WARP:
				warpRows += ((const Warp*)it->seg)->numRows;
				break;
			}
		} while (++it != end && it->seg->row <= row);

		// Check if the warp ended during the current segments.
		if (spr > 0 && startTime > targetTime && warpRows == 0)
		{
			if (row <= entry->row)
			{
				entry->endTime = startTime;
				entry->spr = spr;
			}
			else
			{
				out.emplace_back(row, targetTime, targetTime, startTime, spr);
			}
			return {row, startTime, it};
		}

		prevRow = row;
	}

	// If we reach this point, none of the segments caused the warp to end.
	spr = fabs(spr);
	Row row = prevRow + warpRows;
	if (startTime < targetTime) row += int((targetTime - startTime) / spr);
	out.emplace_back(row, targetTime, targetTime, targetTime, spr);
	return {row, targetTime, it};
}
*/

static void CreateTimingEvents(vector<TimingEvent>& out, double rowZeroTime, const Tempo& tempo)
{
	auto bpm = tempo.bpmChanges.begin();
	auto bpmEnd = tempo.bpmChanges.end();

	auto stop = tempo.stops.begin();
	auto stopEnd = tempo.stops.end();

	auto delay = tempo.delays.begin();
	auto delayEnd = tempo.delays.end();

	auto warp = tempo.warps.begin();
	auto warpEnd = tempo.warps.end();

	out.clear();
	double previousTimingEventRow = 0.0;
	int warpRows = 0;
	double spr = 1.0;
	double startTime = rowZeroTime;
	while (true)
	{
		// First, we find next row at which a segment that affects timing occurs.

		double row = DBL_MAX;

		if (bpm != bpmEnd && bpm->row < row)
			row = bpm->row;

		if (stop != stopEnd && stop->row < row)
			row = stop->row;

		if (delay != delayEnd && delay->row < row)
			row = delay->row;

		if (row == DBL_MAX)
			break;

		// Now that we know the next row, we can apply the startTime passed since the previous event.

		startTime += (row - previousTimingEventRow) * spr;

		// Then we apply the affects of all segments that appear on that row.

		double stopTime = 0.0;
		double delayTime = 0.0;

		if (bpm != bpmEnd && bpm->row == row)
		{
			spr = SecPerRow(bpm->value.bpm);
			++bpm;
		}
		if (stop != stopEnd && stop->row == row)
		{
			stopTime += stop->value.seconds;
			++stop;
		}
		if (delay != delayEnd && delay->row == row)
		{
			delayTime += delay->value.seconds;
			++delay;
		}
		if (warp != warpEnd && warp->row == row)
		{
			warpRows += warp->value.numRows;
			++warp;
		}

		double hitTime = startTime + delayTime;
		double endTime = hitTime + stopTime;
		out.emplace_back(int(row), startTime, hitTime, endTime, spr);

		if (endTime < startTime || spr < 0 || warpRows > 0)
		{
			// TODO: fix
			// auto result = HandleWarp(out, it, end, warpRows);
			// spr = out.back().spr;
			// endTime = result.startTime;
			// row = result.row;
			// it = result.it;
		}
		
		startTime = endTime;
		previousTimingEventRow = row;
	}

	if (out.empty())
		out.emplace_back(0, 0.0, 0.0, 0.0, BeatsPerRow);
}

// =====================================================================================================================
// TimingEvent signature processing.

/*
static void CreateSignatures(vector<Signature>& out, const TimeSignature* it, const TimeSignature* end)
{
	out.clear();
	Row row = 0, measure = 0, rowsPerMeasure = 4 * RowsPerBeat;
	while (it != end)
	{
		for (; it != end && it->row <= row; ++it)
		{
			rowsPerMeasure = max(it->beatsPerMeasure, 1) * RowsPerBeat;
		}
		out.emplace_back(row, measure, rowsPerMeasure);
		if (it != end)
		{
			int passedMeasures = (it->row - row + rowsPerMeasure - 1) / rowsPerMeasure;
			row += passedMeasures * rowsPerMeasure;
			measure += passedMeasures;
		}
	}
	if (out.empty())
	{
		out.emplace_back(0, 0, RowsPerBeat * 4);
	}
}
*/

// =====================================================================================================================
// TimingEvent translation functions.

static const Signature* MostRecentTimeSig(const vector<Signature>& signatures, double row)
{
	const Signature* it = signatures.data(), *mid;
	size_t count = signatures.size(), step;
	while (count > 1)
	{
		step = count >> 1;
		mid = it + step;
		if (mid->row <= row)
		{
			it = mid;
			count -= step;
		}
		else count = step;
	}
	return it;
}

static const TimingEvent* MostRecentTimingEventAtRow(const vector<TimingEvent>& timings, double row)
{
	const TimingEvent* it = timings.data(), *mid;
	size_t count = timings.size(), step;
	while (count > 1)
	{
		step = count >> 1;
		mid = it + step;
		if (mid->row <= row)
		{
			it = mid;
			count -= step;
		}
		else count = step;
	}
	return it;
}

static const TimingEvent* MostRecentTimingEventAtTime(const vector<TimingEvent>& timings, double time)
{
	const TimingEvent* it = timings.data(), *mid;
	size_t count = timings.size(), step;
	while (count > 1)
	{
		step = count >> 1;
		mid = it + step;
		if (mid->startTime <= time)
		{
			it = mid;
			count -= step;
		}
		else count = step;
	}
	return it;
}

static double TimeToRow(const TimingEvent* it, double time)
{
	if (time > it->endTime && it->spr > 0.0)
		return it->row + (time - it->endTime) / it->spr;

	return it->row;
}

static double RowToTime(const TimingEvent* it, double row)
{
	if (row > it->row)
		return it->endTime + (row - it->row) * it->spr;

	return it->hitTime;
}

static double RowToMeasure(const Signature* sig, double row)
{
	return sig->measure + (row - sig->row) / (double)sig->rowsPerMeasure;
}

// =====================================================================================================================
// Tempo list :: implementation.

TimingData::TimingData()
{
	myEvents.emplace_back(0, 0.0, 0.0, 0.0, BeatsPerRow);
	mySignatures.emplace_back(0, 0, int(RowsPerBeat * 4));
}

void TimingData::update(double offset, const Tempo& tempo)
{
	CreateTimingEvents(myEvents, -offset, tempo);

	// TODO: fix signatures.
	// CreateSignatures(mySignatures, segments.begin<TimeSignature>(), segments.end<TimeSignature>());
}

double TimingData::timeToRow(double time) const
{
	return TimeToRow(MostRecentTimingEventAtTime(myEvents, time), time);
}

double TimingData::rowToTime(double row) const
{
	return RowToTime(MostRecentTimingEventAtRow(myEvents, row), row);
}

double TimingData::rowToMeasure(double row) const
{
	return RowToMeasure(MostRecentTimeSig(mySignatures, row), row);
}

TimingData::TimeTracker TimingData::timeTracker() const
{
	return TimeTracker(*this);
}

TimingData::RowTracker TimingData::rowTracker() const
{
	return RowTracker(*this);
}

const vector<TimingEvent>& TimingData::events() const
{
	return myEvents;
}

const vector<Signature>& TimingData::signatures() const
{
	return mySignatures;
}

// =====================================================================================================================
// TimeTracker.

TimingData::TimeTracker::TimeTracker(const TimingData& data)
	: it(data.myEvents.data())
	, end(data.myEvents.data() + data.myEvents.size())
{
	VortexAssertf(it != end, "TimingData timings must never be empty");
	reset();
}

double TimingData::TimeTracker::advance(Row row)
{
	while (row >= nextRow)
	{
		auto next = it + 1;
		if (next == end) break;
		it = next;
		next = it + 1;
		if (next != end)
		{
			nextRow = next->row;
		}
		else
		{
			nextRow = INT_MAX;
		}
		warping = (it->spr == 0);
	}
	return RowToTime(it, row);
}

double TimingData::TimeTracker::lookAhead(Row row) const
{
	auto tmpIt = it;
	auto tmpNextRow = nextRow;
	while (row >= tmpNextRow)
	{
		auto next = tmpIt + 1;
		if (next == end) break;
		tmpIt = next;
		next = tmpIt + 1;
		if (next != end)
		{
			tmpNextRow = next->row;
		}
		else
		{
			tmpNextRow = INT_MAX;
		}
	}
	return RowToTime(tmpIt, row);
}

void TimingData::TimeTracker::reset()
{
	auto next = it + 1;
	if (next != end)
	{
		nextRow = next->row;
	}
	else
	{
		nextRow = INT_MAX;
	}
}

// =====================================================================================================================
// RowTracker.

TimingData::RowTracker::RowTracker(const TimingData& data)
	: it(data.myEvents.data())
	, end(data.myEvents.data() + data.myEvents.size())
{
	VortexAssertf(it != end, "TimingData timings must never be empty");
	reset();
}

double TimingData::RowTracker::advance(double time)
{
	while (time >= nextTime)
	{
		auto next = it + 1;
		if (next == end) break;
		it = next;
		next = it + 1;
		if (next != end)
		{
			nextTime = next->startTime;
		}
		else
		{
			nextTime = DBL_MAX;
		}
		warping = (it->spr == 0);
	}
	return TimeToRow(it, time);
}

double TimingData::RowTracker::lookAhead(double time) const
{
	auto tmpIt = it;
	auto tmpNextTime = nextTime;
	while (time >= tmpNextTime)
	{
		auto next = tmpIt + 1;
		if (next == end) break;
		tmpIt = next;
		next = tmpIt + 1;
		if (next != end)
		{
			tmpNextTime = next->startTime;
		}
		else
		{
			tmpNextTime = DBL_MAX;
		}
	}
	return TimeToRow(tmpIt, time);
}

void TimingData::RowTracker::reset()
{
	auto next = it + 1;
	if (next != end)
	{
		nextTime = next->startTime;
	}
	else
	{
		nextTime = DBL_MAX;
	}
}

} // namespace AV
