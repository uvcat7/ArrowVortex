#pragma once

#include <Simfile/Tempo/TimingData.h>

#include <Vortex/Editor.h>
#include <Vortex/Settings.h>

#include <Vortex/View/View.h>
#include <Vortex/NoteField/Notefield.h>

namespace AV {

struct NotefieldPosTracker
{
	NotefieldPosTracker()
		: tracker(Editor::currentTempo()->timing)
		, baseY(View::offsetToY(0))
		, deltaY(View::getPixPerOfs())
	{
		if (Settings::view().isTimeBased)
		{
			advanceFunc = AdvanceTime;
			lookAheadFunc = LookAheadTime;
		}
		else
		{
			advanceFunc = AvanceRow;
			lookAheadFunc = LookAheadRow;
		}
	}

	void reset()
	{
		tracker.reset();
	}

	inline double advance(Row row)
	{
		return advanceFunc(this, row);
	}

	inline double lookAhead(Row row)
	{
		return lookAheadFunc(this, row);
	}

	static double AvanceRow(NotefieldPosTracker* t, Row row)
	{
		return t->baseY + t->deltaY * double(row);
	}
	static double LookAheadRow(NotefieldPosTracker* t, Row row)
	{
		return t->baseY + t->deltaY * double(row);
	}
	static double AdvanceTime(NotefieldPosTracker* t, Row row)
	{
		return t->baseY + t->deltaY * t->tracker.advance(row);
	}
	static double LookAheadTime(NotefieldPosTracker* t, Row row)
	{
		return t->baseY + t->deltaY * t->tracker.lookAhead(row);
	}

	typedef double (*AdvanceFunc)(NotefieldPosTracker*, int);
	typedef double (*LookAheadFunc)(NotefieldPosTracker*, int);

	double baseY, deltaY;
	TimingData::TimeTracker tracker;
	AdvanceFunc advanceFunc;
	LookAheadFunc lookAheadFunc;
};

} // namespace AV
