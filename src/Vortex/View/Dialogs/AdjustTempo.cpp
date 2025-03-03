#include <Precomp.h>

#include <Vortex/View/Dialogs/AdjustTempo.h>

#include <Core/Utils/Util.h>

#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/CycleButton.h>
#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Label.h>

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

#define Dlg DialogAdjustTempo

namespace AV {

using namespace std;

enum class Actions
{
	SetBPM,
	HalveBPM,
	DoubleBPM,
	TweakBPM,

	SetStop,
	ConvertRegionToStop,
	ConvertRegionToStutter,
	TweakStop,

	SetDelay,
	SetWarp,
	SetTimeSignature,
	SetTickCount,
	SetCombo,
	SetSpeed,
	SetScroll,
	SetFake,
	SetLabel,

	InsertBeats,
	RemoveBeats,
};

struct Dlg::Form
{
	shared_ptr<WVerticalTabs> tabs;
	shared_ptr<WSpinner> bpm;
	shared_ptr<WSpinner> stop;
	shared_ptr<WSpinner> delay;
	shared_ptr<WSpinner> warp;
	shared_ptr<WSpinner> timeSigBpm;
	shared_ptr<WSpinner> timeSigNote;
	shared_ptr<WSpinner> tickCount;
	shared_ptr<WSpinner> comboHit;
	shared_ptr<WSpinner> comboMiss;
	shared_ptr<WSpinner> speedRatio;
	shared_ptr<WSpinner> speedDelay;
	shared_ptr<WCycleButton> speedUnit;
	shared_ptr<WSpinner> scrollRatio;
	shared_ptr<WSpinner> fakeBeats;
	shared_ptr<WLineEdit> labelText;

	EventSubscriptions subscriptions;
};

// =====================================================================================================================
// Constructor and destructor.

Dlg::~Dlg()
{
	delete myForm;
}

Dlg::Dlg()
{
	setTitle("ADJUST TEMPO");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();

	myBeatsToInsert = 1;
	myInsertTarget = 0;

	myForm->subscriptions.add<Editor::ActiveTempoChanged>(this, &Dlg::myHandleActiveTempoChanged);
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitMainTab();
	myInitSm5Tab();
	myInitOffsetTab();
}

void Dlg::myInitMainTab()
{
	static const char* TooltipsBpm[] =
	{
		"Music tempo at the current beat, in beats per minute",
		"Halve the current BPM",
		"Double the current BPM",
		"Tweak the current BPM"
	};

	static const char* TooltipsStop[] =
	{
		"Stop length at the current beat, in seconds",
		"Convert the selected region to a stop",
		"Convert the selected region to a stutter gimmick",
		"Tweak the current stop"
	};

	auto bpmLabel = make_shared<WLabel>("BPM");
	auto stopLabel = make_shared<WLabel>("Stop");

	shared_ptr<WSpinner> spinners[2];
	shared_ptr<WButton> buttons[6];

	for (int i = 1; i >= 0; --i)
	{
		const char** tooltips = i ? TooltipsBpm : TooltipsStop;

		auto spinner = spinners[i] = make_shared<WSpinner>();
		spinner->setPrecision(3, 3);
		spinner->onChange = bind(&Dlg::onAction, this, i ? (int)Actions::SetBPM : (int)Actions::SetStop);
		spinner->setTooltip(tooltips[0]);

		if (i == 0)
			myForm->bpm = spinner;
		else
			myForm->stop = spinner;

		auto buttonA = buttons[0 + i] = make_shared<WButton>(i ? "{g:halve}" : "{g:full selection}");
		buttonA->onPress = bind(&Dlg::onAction, this, i ? (int)Actions::HalveBPM : (int)Actions::ConvertRegionToStop);
		buttonA->setTooltip(tooltips[1]);

		auto buttonB = buttons[2 + i] = make_shared<WButton>(i ? "{g:double}" : "{g:half selection}");
		buttonB->onPress = bind(&Dlg::onAction, this, i ? (int)Actions::DoubleBPM : (int)Actions::ConvertRegionToStutter);
		buttonB->setTooltip(tooltips[2]);

		auto buttonC = buttons[4 + i] = make_shared<WButton>("{g:tweak}");
		buttonC->onPress = bind(&Dlg::onAction, this, i ? (int)Actions::TweakBPM : (int)Actions::TweakStop);
		buttonC->setTooltip(tooltips[3]);
	}

	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4);
	grid->addRow(24, 0);

	grid->addColumn(64, 4);
	grid->addColumn(64, 4, 1);
	grid->addColumn(25, 4);
	grid->addColumn(25, 4);
	grid->addColumn(25, 0);

	grid->addWidget(bpmLabel,    0, 0);
	grid->addWidget(spinners[0], 0, 1);
	grid->addWidget(buttons[0],  0, 2);
	grid->addWidget(buttons[2],  0, 3);
	grid->addWidget(buttons[4],  0, 4);
	grid->addWidget(stopLabel,   1, 0);
	grid->addWidget(spinners[1], 1, 1);
	grid->addWidget(buttons[1],  1, 2);
	grid->addWidget(buttons[3],  1, 3);
	grid->addWidget(buttons[5],  1, 4);

	myForm->tabs->addTab(grid);
}

void Dlg::myInitSm5Tab()
{
	auto delayLabel = make_shared<WLabel>("Delay");
	auto delay = make_shared<WSpinner>();
	delay->setPrecision(3, 3);
	delay->onChange = bind(&Dlg::onAction, this, (int)Actions::SetDelay);
	delay->setTooltip("Stop length at the current beat, in seconds");
	delay->setStep(0.001);
	delay->setRange(0, 1000);
	myForm->delay = delay;

	auto warpLabel = make_shared<WLabel>("Delay");
	auto warp = make_shared<WSpinner>();
	warp->setPrecision(3, 3);
	warp->onChange = bind(&Dlg::onAction, this, (int)Actions::SetWarp);
	warp->setTooltip("Warp length at the current beat, in beats");
	warp->setRange(0, 1000);
	myForm->warp = warp;

	auto sigLabel = make_shared<WLabel>("Sign.");
	auto sigNum = make_shared<WSpinner>();
	sigNum->setPrecision(0, 0);
	sigNum->onChange = bind(&Dlg::onAction, this, (int)Actions::SetTimeSignature);
	sigNum->setTooltip("Beats per measure");
	sigNum->setRange(1, 1000);
	myForm->timeSigBpm = sigNum;

	auto sigDen = make_shared<WSpinner>();
	sigDen->setPrecision(0, 0);
	sigDen->onChange = bind(&Dlg::onAction, this, (int)Actions::SetTimeSignature);
	sigDen->setTooltip("Beat note type");
	sigDen->setRange(1, 1000);
	myForm->timeSigNote = sigDen;

	auto ticksLabel = make_shared<WLabel>("Ticks");
	auto ticks = make_shared<WSpinner>();
	ticks->setPrecision(0, 0);
	ticks->onChange = bind(&Dlg::onAction, this, (int)Actions::SetTickCount);
	ticks->setTooltip("Hold combo ticks per beat");
	ticks->setRange(0, 1000);
	myForm->tickCount = ticks;

	auto comboLabel = make_shared<WLabel>("Combo");
	auto comboHit = make_shared<WSpinner>();
	comboHit->setPrecision(0, 0);
	comboHit->onChange = bind(&Dlg::onAction, this, (int)Actions::SetCombo);
	comboHit->setTooltip("Combo hit multiplier");
	comboHit->setRange(0, 1000);
	myForm->comboHit = comboHit;

	auto comboMiss = make_shared<WSpinner>();
	comboMiss->setPrecision(0, 0);
	comboMiss->onChange = bind(&Dlg::onAction, this, (int)Actions::SetCombo);
	comboMiss->setTooltip("Combo miss multiplier");
	comboMiss->setRange(0, 1000);
	myForm->comboMiss = comboMiss;

	auto speedLabel = make_shared<WLabel>("Speed");
	auto speedStretch = make_shared<WSpinner>();
	speedStretch->setPrecision(2, 2);
	speedStretch->onChange = bind(&Dlg::onAction, this, (int)Actions::SetSpeed);
	speedStretch->setTooltip("Stretch ratio");
	speedStretch->setStep(0.1);
	speedStretch->setRange(0, 1000);
	myForm->speedRatio = speedStretch;

	auto speedDelay = make_shared<WSpinner>();
	speedDelay->setPrecision(2, 2);
	speedDelay->onChange = bind(&Dlg::onAction, this, (int)Actions::SetSpeed);
	speedDelay->setTooltip("Delay startTime");
	speedDelay->setStep(0.1);
	speedDelay->setRange(0, 1000);
	myForm->speedDelay = speedDelay;

	auto speedUnit = make_shared<WCycleButton>();
	speedUnit->onChange = bind(&Dlg::onAction, this, (int)Actions::SetSpeed);
	speedUnit->setTooltip("Delay unit (beats/startTime)");
	speedUnit->addItem("B");
	speedUnit->addItem("T");
	myForm->speedUnit = speedUnit;

	auto scrollLabel = make_shared<WLabel>("Scroll");
	auto scroll = make_shared<WSpinner>();
	scroll->setPrecision(2, 2);
	scroll->onChange = bind(&Dlg::onAction, this, (int)Actions::SetScroll);
	scroll->setTooltip("Scroll ratio");
	scroll->setStep(0.1);
	scroll->setRange(0, 1000);
	myForm->scrollRatio = scroll;

	auto fakesLabel = make_shared<WLabel>("Fakes");
	auto fakes = make_shared<WSpinner>();
	fakes->setPrecision(3, 3);
	fakes->onChange = bind(&Dlg::onAction, this, (int)Actions::SetFake);
	fakes->setTooltip("Fake region, in beats");
	fakes->setRange(0, 1000);
	myForm->fakeBeats = fakes;

	auto labelLabel = make_shared<WLabel>("Label");
	auto label = make_shared<WLineEdit>();
	label->setMaxLength(32);
	label->onChange = bind(&Dlg::onAction, this, (int)Actions::SetLabel);
	label->setTooltip("Label text");
	myForm->labelText = label;
	
	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 0);

	grid->addColumn(64, 4, 0);
	grid->addColumn(48, 4, 2);
	grid->addColumn(22, 4, 1);
	grid->addColumn(22, 4, 1);
	grid->addColumn(48, 0, 2);

	grid->addWidget(delayLabel, 0, 0);
	grid->addWidget(delay, 0, 1, 1, 4);
	grid->addWidget(warpLabel, 1, 0);
	grid->addWidget(warp, 1, 1, 1, 4);
	grid->addWidget(sigLabel, 2, 0);
	grid->addWidget(sigNum, 2, 1, 1, 2);
	grid->addWidget(sigDen, 2, 3, 1, 2);
	grid->addWidget(ticksLabel, 3, 0);
	grid->addWidget(ticks, 3, 1, 1, 4);
	grid->addWidget(comboLabel, 4, 0);
	grid->addWidget(comboHit, 4, 1, 1, 2);
	grid->addWidget(comboMiss, 4, 3, 1, 2);
	grid->addWidget(speedLabel, 5, 0);
	grid->addWidget(speedStretch, 5, 1);
	grid->addWidget(speedDelay, 5, 2, 1, 2);
	grid->addWidget(speedUnit, 5, 4);
	grid->addWidget(scrollLabel, 6, 0);
	grid->addWidget(scroll, 6, 1, 1, 4);
	grid->addWidget(fakesLabel, 7, 0);
	grid->addWidget(fakes, 7, 1, 1, 4);
	grid->addWidget(labelLabel, 8, 0);
	grid->addWidget(label, 8, 1, 1, 4);

	myForm->tabs->addTab(grid, "SM5 Features", false);
}

void Dlg::myInitOffsetTab()
{
	/*
	auto offset = myFormOffset = new FormOffset;

	offset->lBeats.setText("Offset in beats");

	offset->sBeats.setRange(0.0, 100000.0);
	offset->sBeats.value.bind(&myBeatsToInsert);
	offset->sBeats.setPrecision(3, 3);
	offset->sBeats.setTooltip("Number of beats to insert or remove");

	offset->lApplyTo.setText("Apply offset to");

	offset->bChart.addItem("This chart");
	offset->bChart.addItem("All charts");
	offset->bChart.value.bind(&myInsertTarget);
	offset->bChart.setTooltip("Determines which notes and/or tempo changes are targeted");

	offset->bInsert.text.set("Insert beats");
	offset->bInsert.onPress.bind(this, &Dlg::onAction, (int)Actions::INSERT_BEATS);
	offset->bInsert.setTooltip("Insert the above number of beats at the cursor position\n"
		"All notes and tempo changes after the cursor will be shifted down");

	offset->bDelete.text.set("Delete beats");
	offset->bDelete.onPress.bind(this, &Dlg::onAction, (int)Actions::REMOVE_BEATS);
	offset->bDelete.setTooltip("Delete the above number of beats at the cursor position\n"
		"All notes and tempo changes after the cursor will be shifted up\n"
		"Notes and tempo changes in the deleted region will be removed");
	*/

	/*
	struct Dlg::FormOffset
	{
		FormOffset();

		WLabel lBeats;
		WSpinner sBeats;
		WLabel lApplyTo;
		WCycleButton bChart;
		WButton bInsert;
		WButton bDelete;

		WGrid grid;
	};

	const WGrid::Span sRows[3] = {
		{24, 10000, 0, 4},
		{24, 10000, 0, 4},
		{24, 10000, 0, 4},
	};
	grid.setRows(sRows, 3);

	const WGrid::Span sCols[2] = {
		{96, 10000, 1, 4},
		{96, 10000, 1, 4},
	};
	grid.setCols(sCols, 2);

	const WGrid::Cell sCells[6] = {
		{0, 0, 1, 1, &lBeats},
		{0, 1, 1, 1, &sBeats},
		{1, 0, 1, 1, &lApplyTo},
		{1, 1, 1, 1, &bChart},
		{2, 0, 1, 1, &bInsert},
		{2, 1, 1, 1, &bDelete},
	};
	grid.setCells(sCells, 6);
	*/
}

// =====================================================================================================================
// Widget functions.

static void EnableWidget(Widget& w)
{
	w.setEnabled(true);
}

static void DisableWidget(Widget& w)
{
	w.setEnabled(false);
}

void Dlg::myHandleActiveTempoChanged()
{
	clear();
	if (Editor::currentTempo())
	{
		myForm->tabs->setEnabled(true);
	}
	else
	{
		myForm->tabs->setEnabled(false);
	}
}

void Dlg::clear()
{
	myForm->bpm->setValue(0.0);
	myForm->stop->setValue(0.0);
	myForm->delay->setValue(0.0);
	myForm->warp->setValue(0.0);
	myForm->timeSigBpm->setValue(4);
	myForm->timeSigNote->setValue(4);
	myForm->tickCount->setValue(0);
	myForm->comboHit->setValue(1);
	myForm->comboMiss->setValue(1);
	myForm->speedRatio->setValue(1.0);
	myForm->speedDelay->setValue(0.0);
	myForm->speedUnit->setSelectedItem(0);
	myForm->scrollRatio->setValue(1.0);
	myForm->fakeBeats->setValue(0.0);
	myForm->labelText->setText(string());

	myBeatsToInsert = 0;
}

void Dlg::tick(bool enabled)
{
	auto tempo = Editor::currentTempo();
	if (tempo)
	{
		Row row = View::getCursorRowI();

		auto bpm = tempo->bpmChanges.get(row).bpm;
		myForm->bpm->setValue(bpm);

		auto stop = tempo->stops.get(row);
		myForm->stop->setValue(stop ? stop->seconds : 0.0);

		auto delay = tempo->delays.get(row);
		myForm->delay->setValue(delay ? delay->seconds : 0.0);
		
		auto warp = tempo->warps.get(row);
		myForm->warp->setValue(warp ? warp->numRows / (double)RowsPerBeat : 0.0);

		auto sig = tempo->timeSignatures.get(row);
		myForm->timeSigBpm->setValue(sig.beatsPerMeasure);
		myForm->timeSigNote->setValue(sig.beatNote);

		auto tick = tempo->tickCounts.get(row);
		myForm->tickCount->setValue(tick.ticks);

		auto combo = tempo->combos.get(row);
		myForm->comboHit->setValue(combo.hitCombo);
		myForm->comboMiss->setValue(combo.missCombo);

		auto speed = tempo->speeds.get(row);
		myForm->speedRatio->setValue(speed.ratio);
		myForm->speedDelay->setValue(speed.delay);
		myForm->speedUnit->setSelectedItem(speed.unit);

		auto scroll = tempo->scrolls.get(row);
		myForm->scrollRatio->setValue(scroll.ratio);

		auto fake = tempo->fakes.get(row);
		myForm->fakeBeats->setValue(fake ? fake->numRows / (double)RowsPerBeat : 0);

		auto label = tempo->labels.get(row);
		myForm->labelText->setText(label ? label->str : string());
	}
	EditorDialog::tick(enabled);
}

void Dlg::onAction(int rawId)
{
	auto tempo = Editor::currentTempo();
	if (!tempo) return;

	Row row = View::getCursorRowI();
	auto& timing = tempo->timing;

	auto id = (Actions)rawId;
	switch (id)
	{
	case Actions::SetBPM: {
		SetSegment<BpmChangeSegment>(tempo->bpmChanges, row, { .bpm = myForm->bpm->value() });
	} break;

	case Actions::HalveBPM:
	case Actions::DoubleBPM: {
		double bpm = tempo->getBpm(row) * ((id == Actions::DoubleBPM) ? 2 : 0.5);
		SetSegment<BpmChangeSegment>(tempo->bpmChanges, row, { .bpm = bpm });
	} break;

	case Actions::TweakBPM:
		TempoTweaker::startTweakingBpm(row);
		break;

	case Actions::SetStop: {
		SetSegment<StopSegment>(tempo->stops, row, { .seconds = myForm->stop->value() });
	} break;

	case Actions::ConvertRegionToStutter:
	case Actions::ConvertRegionToStop: {
		auto region = Selection::getRegion();
		double t1 = timing.rowToTime(region.beginRow);
		double t2 = timing.rowToTime(region.endRow);
		if (t2 > t1)
		{
			if (id == Actions::ConvertRegionToStop)
			{
				SetSegment<StopSegment>(tempo->stops, region.beginRow, { .seconds = t2 - t1 });
			}
			else if (region.endRow - region.beginRow >= 2)
			{
				double halfTime = (t2 - t1) * 0.5;
				Row rows = region.endRow - region.beginRow;

				// TODO: Fix.
				// TempoModification mod;
				// 
				// mod.segmentsToAdd.stops.set(region.beginRow, { .seconds = halfTime });
				// 
				// double stutterBpm = BeatsPerMin(halfTime / rows);
				// mod.segmentsToAdd.bpmChanges.set(region.beginRow, { .bpm = stutterBpm });
				// 
				// double endBpm = tempo->getBpm(region.endRow);
				// mod.segmentsToAdd.bpmChanges.set(region.endRow, { .bpm = endBpm });
				// 
				// tempo->modify(nullptr, mod);
			}
		}
	} break;

	case Actions::TweakStop:
		TempoTweaker::startTweakingStop(row);
		break;

		/*case Actions::INSERT_BEATS:
		case Actions::REMOVE_BEATS: {
			int numRows = int(RowsPerBeat * myBeatsToInsert + 0.5);
			if (id == Actions::REMOVE_BEATS) numRows *= -1;
			Editing::insertRows(View::getCursorRowI(), numRows, (myInsertTarget == 0));
		} break;*/

	case Actions::SetDelay: {
		SetSegment<DelaySegment>(tempo->delays, row, {
			.seconds = myForm->delay->value()
		});
	} break;

	case Actions::SetWarp: {
		Row rows = int(RowsPerBeat * myForm->warp->value() + 0.5);
		SetSegment<WarpSegment>(tempo->warps, row, {
			.numRows = rows
		});
	} break;

	case Actions::SetTimeSignature: {
		SetSegment<TimeSignatureSegment>(tempo->timeSignatures, row, {
			.beatsPerMeasure = myForm->timeSigBpm->ivalue(),
			.beatNote = myForm->timeSigNote->ivalue(),
		});
	} break;

	case Actions::SetTickCount: {
		SetSegment<TickCountSegment>(tempo->tickCounts, row, {
			.ticks = myForm->tickCount->ivalue()
		});
	} break;

	case Actions::SetCombo: {
		SetSegment<ComboSegment>(tempo->combos, row, {
			.hitCombo = myForm->comboHit->ivalue(),
			.missCombo = myForm->comboMiss->ivalue()
		});
	} break;

	case Actions::SetSpeed: {
		SetSegment<SpeedSegment>(tempo->speeds, row, {
			.ratio = myForm->speedRatio->value(),
			.delay = myForm->speedDelay->value(),
			.unit = myForm->speedUnit->selectedItem(),
		});
	} break;

	case Actions::SetScroll: {
		SetSegment<ScrollSegment>(tempo->scrolls, row, {
			.ratio = myForm->scrollRatio->value()
		});
	} break;

	case Actions::SetFake: {
		Row rows = int(RowsPerBeat * myForm->fakeBeats->value() + 0.5);
		SetSegment<FakeSegment>(tempo->fakes, row, {
			.numRows = rows
		});
	} break;

	case Actions::SetLabel: {
		SetSegment<LabelSegment>(tempo->labels, row, {
			.str = myForm->labelText->text()
		});
	} break;
	};
}

} // namespace AV
