#include <Precomp.h>

#include <Simfile/Tempo.h>
#include <Simfile/Tempo/SegmentUtils.h>

#include "TestUtils.h"

#ifdef UNIT_TEST_BUILD

namespace Vortex {

using namespace std;

template <typename T>
static void CheckSize(const T& track, size_t size)
{
	Check(track.begin() + size == track.end());
}

static void CheckSegment(const BpmChangeSegment::Track& track, int index, Row row, double bpm)
{
	auto segment = track.begin()[index];
	Check(segment.row == row);
	Check(segment.value.bpm == bpm);
}

static void CheckSegment(const StopSegment::Track& track, int index, Row row, double seconds)
{
	auto segment = track.begin()[index];
	Check(segment.row == row);
	Check(segment.value.seconds == seconds);
}

static void CheckChange(const SegmentChanges<BpmChangeSegment>& changes, Row row, double bpmBefore, double bpmAfter)
{
	auto it = changes.find(row);
	Check(it != changes.end());
	Check(it->second.before.bpm == bpmBefore);
	Check(it->second.after.bpm == bpmAfter);
}

static void CheckChange(const SegmentChanges<StopSegment>& changes, Row row, double secondsBefore, double secondsAfter)
{
	auto it = changes.find(row);
	Check(it != changes.end());
	Check(it->second.before.seconds == secondsBefore);
	Check(it->second.after.seconds == secondsAfter);
}

static void CheckEqual(const BpmChangeSegment::Track& a, const BpmChangeSegment::Track& b)
{
	Check(a.end() - a.begin() == b.end() - b.begin());
	for (auto itA = a.begin(), itB = b.begin(); itA != a.end() && itB != b.end(); ++itA, ++itB)
	{
		Check(itA->row == itB->row);
		Check(itA->value.bpm == itB->value.bpm);
	}
}

static void CheckEqual(const StopSegment::Track& a, const StopSegment::Track& b)
{
	Check(a.end() - a.begin() == b.end() - b.begin());
	for (auto itA = a.begin(), itB = b.begin(); itA != a.end() && itB != b.end(); ++itA, ++itB)
	{
		Check(itA->row == itB->row);
		Check(itA->value.seconds == itB->value.seconds);
	}
}

TestMethod(PersistentSegmentTest)
{
	auto track = BpmChangeSegment::Track();
	auto history = vector<SegmentChanges<BpmChangeSegment>>();
	auto states = vector<BpmChangeSegment::Track>();

	// Initial track should contain a single BPM, the fallback value 120.
	CheckSize(track, 1);
	CheckSegment(track, 0, 0, 120);
	states.push_back(track);

	// Append BPM of 150 at row 100.
	history.push_back(SetSegment(track, 100, { 150 }));
	states.push_back(track);
	CheckSize(track, 2);
	CheckSegment(track, 0, 0, 120);
	CheckSegment(track, 1, 100, 150);
	Check(history[0].size() == 1);
	CheckChange(history[0], 100, 120, 150);

	// Appending another BPM of 150 at row 200 should not result in a change.
	history.push_back(SetSegment(track, 200, { 150 }));
	states.push_back(track);
	CheckSize(track, 2);
	Check(history[1].size() == 0);

	// Inserting a BPM of 150 at row 50 should remove the one at row 100.
	history.push_back(SetSegment(track, 50, { 150 }));
	states.push_back(track);
	CheckSize(track, 2);
	CheckSegment(track, 0, 0, 120);
	CheckSegment(track, 1, 50, 150);
	Check(history[2].size() == 2);
	CheckChange(history[2], 50, 120, 150);
	CheckChange(history[2], 100, 150, 150);

	// Test if reverting results in the correct previous states.
	for (int i = 2; i >= 0; --i)
	{
		track.undo(history[i]);
		CheckEqual(track, states[i]);
	}
	for (int i = 0; i <= 2; ++i)
	{
		track.reapply(history[i]);
		CheckEqual(track, states[i + 1]);
	}
}

TestMethod(IsolatedSegmentTest)
{
	auto track = StopSegment::Track();
	auto history = vector<SegmentChanges<StopSegment>>();
	auto states = vector<StopSegment::Track>();

	// Initial track should be empty.
	CheckSize(track, 0);
	states.push_back(track);

	// Set stop of 1 second at row 100.
	history.push_back(SetSegment(track, 100, { 1 }));
	states.push_back(track);
	CheckSize(track, 1);
	CheckSegment(track, 0, 100, 1);
	Check(history[0].size() == 1);
	CheckChange(history[0], 100, 0, 1);

	// Set another stop of 2 at row 200.
	history.push_back(SetSegment(track, 200, { 2 }));
	states.push_back(track);
	CheckSize(track, 2);
	Check(history[1].size() == 1);
	CheckChange(history[1], 200, 0, 2);

	// Test if reverting results in the correct previous states.
	for (int i = 1; i >= 0; --i)
	{
		track.undo(history[i]);
		CheckEqual(track, states[i]);
	}
	for (int i = 0; i <= 1; ++i)
	{
		track.reapply(history[i]);
		CheckEqual(track, states[i + 1]);
	}
}

}; // namespace Vortex

#endif // UNIT_TEST_BUILD