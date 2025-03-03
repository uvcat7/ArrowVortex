#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>
#include <Core/Common/Serialize.h>

#include <Simfile/Tempo/SegmentSet.h>
#include <Simfile/Tempo/TimingData.h>
#include <Simfile/History.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/View/View.h>
#include <Vortex/Notefield/SegmentBoxes.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Editing.h>

#include <Vortex/Edit/TempoTweaker.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Static data

namespace TempoTweaker
{
	struct State
	{
		Tempo* tempo = nullptr;
		int tweakRow = 0;
		TweakMode tweakMode = TweakMode::None;
		double tweakValue = 0.0;
	};
	static State* state = nullptr;
}
using TempoTweaker::state;

// =====================================================================================================================
// Initialization

void TempoTweaker::initialize(const XmrDoc& settings)
{
	state = new State();
}

void TempoTweaker::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// General editing functions.

static bool Differs(const BpmRange& a, const BpmRange& b)
{
	return a.min != b.min || a.max != b.max;
}

static double ClampAndRound(double val, double min, double max)
{
	return round(clamp(val, min, max) * 1000.0) / 1000.0;
}

// =====================================================================================================================
// Tweak functions.

static void CreateTempo(Tempo* source)
{
	auto chart = Editor::currentChart();
	if (chart)
	{
		state->tempo = new Tempo(chart);
	}
	else
	{
		auto sim = Editor::currentSimfile();
		state->tempo = new Tempo(sim);
	}

	// state->tempo->copy(source);
}

void TempoTweaker::startTweakingOffset()
{
	auto tempo = Editor::currentTempo();

	if (state->tweakMode == TweakMode::Offset || !tempo) return;

	stopTweaking(false);
	CreateTempo(tempo);
	state->tweakMode = TweakMode::Offset;
	state->tweakRow = 0;
	state->tweakValue = tempo->offset.get();
}

void TempoTweaker::startTweakingBpm(Row row)
{
	row = max(0, row);

	auto tempo = Editor::currentTempo();

	if ((state->tweakMode == TweakMode::BPM && state->tweakRow == row) || !tempo) return;

	stopTweaking(false);
	CreateTempo(tempo);
	state->tweakMode = TweakMode::BPM;
	state->tweakRow = row;
	state->tweakValue = tempo->getBpm(row);
}

void TempoTweaker::startTweakingStop(Row row)
{
	auto tempo = Editor::currentTempo();

	if ((state->tweakMode == TweakMode::Stop && state->tweakRow == row) || !tempo) return;

	stopTweaking(false);
	CreateTempo(tempo);
	state->tweakMode = TweakMode::Stop;
	state->tweakRow = row;

	auto stop = tempo->stops.get(state->tweakRow);
	state->tweakValue = stop ? stop->seconds : 0.0;
}

void TempoTweaker::setTweakValue(double value)
{
	state->tweakValue = value;

	if (state->tweakMode == TweakMode::Offset)
	{
		state->tempo->offset.set(value);
	}
	else if (state->tweakMode == TweakMode::BPM)
	{
		// TODO: fix.
		// state->tempo->bpmChanges.set(state->tweakRow, { .bpm = value });
	}
	else if (state->tweakMode == TweakMode::Stop)
	{
		// TODO: fix.
		// state->tempo->stops.set(state->tweakRow, { .seconds = value });
	}

	state->tempo->updateTiming();

	EventSystem::publish<Tempo::TimingChanged>();
}

void TempoTweaker::stopTweaking(bool apply)
{
	if (state->tweakMode == TweakMode::None) return;

	// Temporarily store the tweak values.
	TweakMode mode = state->tweakMode;
	double value = state->tweakValue;
	Row row = state->tweakRow;

	// Stop tweaking.
	delete state->tempo;
	state->tempo = nullptr;

	state->tweakRow = 0;
	state->tweakValue = 0.0;
	state->tweakMode = TweakMode::None;

	// Apply the effect of tweaking.
	auto tempo = Editor::currentTempo();
	if (apply)
	{
		if (mode == TweakMode::Offset)
		{
			tempo->offset.set(value);
		}
		else if (mode == TweakMode::BPM)
		{
			// TODO: fix.
			// tempo->bpmChanges.set(row, { .bpm = value });
		}
		else if (mode == TweakMode::Stop)
		{
			// TODO: fix.
			// tempo->stops.set(row, { .seconds = value });
		}
	}

	EventSystem::publish<Tempo::TimingChanged>();
}

// =====================================================================================================================
// Get functions.

TweakMode TempoTweaker::getTweakMode()
{
	return state->tweakMode;
}

double TempoTweaker::getTweakValue()
{
	return state->tweakValue;
}

int TempoTweaker::getTweakRow()
{
	return state->tweakRow;
}

} // namespace AV
