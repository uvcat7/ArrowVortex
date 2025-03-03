#include <Precomp.h>

#include <Vortex/View/Dialogs/AdjustSync.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/SelectList.h>
#include <Core/Interface/Widgets/Label.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo/SegmentSet.h>
#include <Simfile/Tempo/SegmentUtils.h>
#include <Simfile/History.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/View.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Editing.h>

#define Dlg DialogAdjustSync

namespace AV {

using namespace std;

enum class Actions
{
	SetBPM,
	SetOffset,
	TweakBPM,
	TweakOffset,

	IncreaseOffsetByOne,
	IncreaseOffsetByHalf,
	DecreaseOffsetByHalf,
	DecreaseOffsetByOne,
};

struct Dlg::Form
{
	shared_ptr<WVerticalTabs> tabs;
	shared_ptr<WSelectList> results;
	shared_ptr<WButton> apply;
	shared_ptr<WSpinner> initialBpm;
	shared_ptr<WSpinner> offset;

	EventSubscriptions subscriptions;
};

// =====================================================================================================================
// Constructor and destructor.

Dlg::~Dlg()
{
	delete myForm;
	delete myTempoDetector;
}

Dlg::Dlg()
{
	setTitle("ADJUST SYNC");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();

	myForm->subscriptions.add<Tempo::TimingChanged>(this, &Dlg::myHandleTimingChanged);
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitMainTab();
	myInitTempoDetectionTab();
}

void Dlg::myInitMainTab()
{
	// First row, music offset.

	auto offsetLabel = make_shared<WLabel>("Music offset");

	auto offset = myForm->offset = make_shared<WSpinner>();
	offset->setPrecision(3, 3);
	offset->setRange(-100.0, 100.0);
	offset->setStep(0.001);
	offset->onChange = bind(&Dlg::onAction, this, (int)Actions::SetOffset);
	offset->setTooltip("Music start startTime relative to the first beat, in seconds");

	auto tweakOffset = make_shared<WButton>("{g:tweak}");
	tweakOffset->setTooltip("Tweak the music offset");
	tweakOffset->onPress = bind(&Dlg::onAction, this, (int)Actions::TweakOffset);

	// Second row, initial BPM.

	auto bpmLabel = make_shared<WLabel>("Initial BPM");

	auto bpm = make_shared<WSpinner>();
	bpm->setPrecision(3, 3);
	bpm->setRange(EditorConstants::MinBpm, EditorConstants::MaxBpm);
	bpm->setStep(1.0);
	bpm->onChange = bind(&Dlg::onAction, this, (int)Actions::SetBPM);
	bpm->setTooltip("Music tempo at the first beat, in beats per minute");
	myForm->initialBpm = bpm;

	auto tweakBpm = make_shared<WButton>("{g:tweak}");
	tweakBpm->setTooltip("Tweak the initial BPM");
	tweakBpm->onPress = bind(&Dlg::onAction, this, (int)Actions::TweakBPM);

	// Music offset buttons.

	auto firstBeat = make_shared<WLabel>("Move first beat");

	auto SetButton = [&](const char* text, const char* tooltip, int onPress)
	{
		auto button = make_shared<WButton>(text);
		button->onPress = bind(&Dlg::onAction, this, onPress);
		button->setTooltip(tooltip);
		return button;
	};

	auto upOne = SetButton("{g:up one}",
		"Increase the music offset by one beat", (int)Actions::IncreaseOffsetByOne);

	auto upHalf = SetButton("{g:up half}",
		"Increase the music offset by half a beat", (int)Actions::IncreaseOffsetByHalf);

	auto downHalf = SetButton("{g:down half}",
		"Decrease the music offset by half a beat", (int)Actions::DecreaseOffsetByHalf);

	auto downOne = SetButton("{g:down one}",
		"Decrease the music offset by one beat", (int)Actions::DecreaseOffsetByOne);

	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 0);

	grid->addColumn(80, 4);
	grid->addColumn(32, 4, 1);
	grid->addColumn(24, 4);
	grid->addColumn(24, 4);
	grid->addColumn(24, 4);
	grid->addColumn(24, 0);

	grid->addWidget(offsetLabel, 0, 0);
	grid->addWidget(offset,      0, 1, 1, 4);
	grid->addWidget(tweakOffset, 0, 5);
	grid->addWidget(bpmLabel,    1, 0);
	grid->addWidget(bpm,         1, 1, 1, 4);
	grid->addWidget(tweakBpm,    1, 5);
	grid->addWidget(firstBeat,   2, 0, 1, 2);
	grid->addWidget(upOne,       2, 2);
	grid->addWidget(upHalf,      2, 3);
	grid->addWidget(downHalf,    2, 4);
	grid->addWidget(downOne,     2, 5);

	myForm->tabs->addTab(grid);
}

void Dlg::myInitTempoDetectionTab()
{
	auto results = myForm->results = make_shared<WSelectList>();
	results->setTooltip("BPM estimates calculated by the editor");

	auto apply = myForm->apply = make_shared<WButton>("Apply BPM");
	apply->onPress = bind(&Dlg::onApplyTempo, this);
	apply->setTooltip("Apply the selected BPM estimate");
	apply->setEnabled(false);

	auto find = make_shared<WButton>("{g:calculate} Find BPM");
	find->onPress = bind(&Dlg::onFindTempo, this);
	find->setTooltip("Estimate the music BPM by analyzing the audio");

	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4, 1);
	grid->addRow(24, 0);

	grid->addColumn(48, 4, 1);
	grid->addColumn(48, 0, 1);

	grid->addWidget(results, 0, 0, 1, 2);
	grid->addWidget(apply,   1, 0);
	grid->addWidget(find,    1, 1);

	myForm->tabs->addTab(grid, "Tempo Detection");
}

// =====================================================================================================================
// Widget functions.

void Dlg::myHandleTimingChanged()
{
	auto tempo = Editor::currentTempo();
	if (tempo)
	{
		myForm->tabs->setEnabled(true);
		myForm->initialBpm->setValue(tempo->getBpm(0));
		myForm->offset->setValue(tempo->offset.get());
	}
	else
	{
		myForm->tabs->setEnabled(false);
		myForm->initialBpm->setValue(0);
		myForm->offset->setValue(0);
	}
	myResetTempoDetection();
}

void Dlg::tick(bool enabled)
{
	EditorDialog::tick(enabled);

	if (myTempoDetector)
	{
		// Update the progress text.
		const char* progress = myTempoDetector->getProgress();

		// Check if the BPM detector has finished.
		if (myTempoDetector->hasResult())
		{
			myDetectionResults = myTempoDetector->getResult();
			double scalar = 0.0;
			for (auto& t : myDetectionResults)
			{
				scalar += t.fitness;
			}
			for (auto& t : myDetectionResults)
			{
				t.fitness /= scalar;
				t.offset = -t.offset;
			}
			for (int i = 0; i < (int)myDetectionResults.size(); ++i)
			{
				auto& t = myDetectionResults[i];

				auto fmt = format("#{} :: {:.2f} BPM :: {:.0f}%", i + 1, t.bpm, t.fitness * 100);
				myForm->results->addItem(fmt);
			}
			myForm->results->setEnabled(true);
			Util::reset(myTempoDetector);
		}
	}
}

void Dlg::onAction(int rawId)
{
	auto tempo = Editor::currentTempo();

	auto id = (Actions)rawId;
	switch (id)
	{
	case Actions::SetOffset: {
		tempo->offset.set(myForm->offset->value());
	} break;
	case Actions::SetBPM: {
		double bpm = myForm->initialBpm->value();
		SetSegment<BpmChangeSegment>(tempo->bpmChanges, 0, { .bpm = bpm });
	} break;
	case Actions::TweakBPM: {
		TempoTweaker::startTweakingBpm(0);
	} break;
	case Actions::TweakOffset: {
		TempoTweaker::startTweakingOffset();
	} break;
	case Actions::DecreaseOffsetByHalf:
	case Actions::DecreaseOffsetByOne:
	case Actions::IncreaseOffsetByOne:
	case Actions::IncreaseOffsetByHalf: {
		Row rows = Row(RowsPerBeat);
		if (id == Actions::DecreaseOffsetByHalf || id == Actions::IncreaseOffsetByHalf) rows /= 2;
		if (id == Actions::DecreaseOffsetByHalf || id == Actions::DecreaseOffsetByOne) rows *= -1;
		double seconds = (double)rows * SecPerRow(tempo->getBpm(0));
		tempo->offset.set(tempo->offset.get() + seconds);
	} break;
	};
}

void Dlg::onApplyTempo()
{
	auto tempo = Editor::currentTempo();
	auto item = myForm->results->selectedItem();
	if (item >= 0 && item < (int)myDetectionResults.size())
	{
		auto& v = myDetectionResults[item];
		if (myDetectionRow == 0)
		{
			tempo->offset.set(v.offset);
			SetSegment<BpmChangeSegment>(tempo->bpmChanges, 0, { .bpm = v.bpm });
		}
		else
		{
			SetSegment<BpmChangeSegment>(tempo->bpmChanges, myDetectionRow, { .bpm = v.bpm });
		}
	}
}

void Dlg::onFindTempo()
{
	if (!myTempoDetector)
	{
		myResetTempoDetection();
		auto region = Selection::getRegion();
		auto tempo = Editor::currentTempo();
		if (region.beginRow < region.endRow)
		{
			myDetectionRow = region.beginRow;
			auto& timing = tempo->timing;
			double time = timing.rowToTime(region.beginRow);
			double len = timing.rowToTime(region.endRow) - time;
			myTempoDetector = TempoDetector::New(time, len);
		}
		else
		{
			myDetectionRow = 0;
			myTempoDetector = TempoDetector::New(0, 600.0);
		}
	}
}

void Dlg::myResetTempoDetection()
{
	if (myTempoDetector)
		Util::reset(myTempoDetector);

	myDetectionResults.clear();
	myForm->apply->setEnabled(false);
	myForm->results->clearItems();
}

} // namespace AV
