#include <Precomp.h>

#include <Vortex/View/Dialogs/ChartProperties.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Droplist.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/Label.h>
#include <Core/Interface/Widgets/Checkbox.h>

#include <Core/System/System.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Tempo.h>
#include <Simfile/NoteIterator.h>

#include <Vortex/Common.h>
#include <Vortex/Application.h>

#include <Vortex/Managers/GameMan.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/Notefield/Notefield.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

#include <Vortex/Editor.h>
#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/RatingEstimator.h>


#define Dlg DialogChartProperties

namespace AV {

using namespace std;

// =====================================================================================================================
// Helper controls.

/*
class ChartMetaLineEdit : public WLineEdit
{
public:
	typedef Observable<string> Chart::* Property;

	ChartMetaLineEdit(stringref tooltip, Property prop, Event& propChanged)
		: myProperty(prop)
	{
		mySubscription.set(propChanged, bind(&ChartMetaLineEdit::myReadProperty, this));
		onChange = bind(&ChartMetaLineEdit::myWriteProperty, this, placeholders::_1);
		setTooltip(tooltip);
	}

private:
	void myWriteProperty(stringref text)
	{
		if (auto c = Editor::currentChart())
			(c->*myProperty).set(text);
	}

	void myReadProperty()
	{
		if (auto c = Editor::currentChart())
			setText((c->*myProperty).get());
		else
			setText({});
	}

	EventSubscription mySubscription;
	Property myProperty;
};
*/

struct Dlg::Form
{
	shared_ptr<WVerticalTabs> tabs;

	shared_ptr<WDroplist> gameMode;
	shared_ptr<WDroplist> playStyle;
	shared_ptr<WDroplist> difficulty;
	shared_ptr<WSpinner> meter;

	shared_ptr<WLineEdit> stepAuthor;
	shared_ptr<WLineEdit> chartName;
	shared_ptr<WLineEdit> chartStyle;
	shared_ptr<WLineEdit> description;

	shared_ptr<WLineEdit> numSteps;
	shared_ptr<WLineEdit> numHolds;
	shared_ptr<WLineEdit> numMines;
	shared_ptr<WLineEdit> numJumps;
	shared_ptr<WLineEdit> numRolls;
	shared_ptr<WLineEdit> numWarps;
	shared_ptr<WLineEdit> noteDensity;
	shared_ptr<WLineEdit> numMeasures;

	shared_ptr<WBreakdownList> breakdown;
	shared_ptr<WCheckbox> simplifyBreakdown;

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
	setTitle("CHART PROPERTIES");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();
	myUpdateChart();

	myForm->subscriptions.add<Editor::ActiveChartChanged>(this, &Dlg::myUpdateChart);
	myForm->subscriptions.add<Chart::NotesChangedEvent>(this, &Dlg::myUpdateNoteInfo);
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitMainTab();
	myInitMetadataTab();
	myInitNoteInformationTab();
	myInitStreamBreakdownTab();
}

void Dlg::myInitMainTab()
{
	auto typeLabel = make_shared<WLabel>("Chart type");
	auto gameMode = make_shared<WDroplist>();
	gameMode->setEnabled(false);
	gameMode->setTooltip("Game mode of the chart");
	myForm->gameMode = gameMode;

	auto playStyle = make_shared<WDroplist>();
	playStyle->setEnabled(false);
	playStyle->setTooltip("Play style of the chart");
	myForm->playStyle = playStyle;

	auto diffLabel = make_shared<WLabel>("Difficulty");
	auto diff = make_shared<WDroplist>();
	diff->setTooltip("Difficulty type of the chart");
	myForm->difficulty = diff;

	for (int i = 0; i < (int)Difficulty::Count; ++i)
		diff->addItem(GetDifficultyName((Difficulty)i));

	auto meter = make_shared<WSpinner>();
	meter->setRange(1.0, 100000.0);
	meter->onChange = bind(&Dlg::mySetMeter, this);
	meter->setTooltip("Difficulty rating of the chart");
	myForm->meter = meter;

	auto calc = make_shared<WButton>("{g:calculate}");
	calc->onPress = bind(&Dlg::myCalcMeter, this);
	calc->setTooltip("Estimate the chart difficulty by analyzing the notes");

	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(22, 0);

	grid->addColumn(88, 4);
	grid->addColumn(88, 4, 1);
	grid->addColumn(16, 4);
	grid->addColumn(60, 4);
	grid->addColumn(22, 0);

	grid->addWidget(typeLabel, 0, 0);
	grid->addWidget(gameMode, 0, 1);
	grid->addWidget(playStyle, 0, 2, 1, 3);
	grid->addWidget(diffLabel, 1, 0);
	grid->addWidget(diff, 1, 1, 1, 2);
	grid->addWidget(meter, 1, 3);
	grid->addWidget(calc, 1, 4);

	myForm->tabs->addTab(grid);
}

void Dlg::myInitMetadataTab()
{
	auto authorLabel = make_shared<WLabel>("Step author");
	auto author = make_shared<WLineEdit>();
	author->setTooltip("Creator of the chart");
	author->onChange = bind(&Dlg::mySetStepAuthor, this);
	myForm->stepAuthor = author;

	auto nameLabel = make_shared<WLabel>("Chart name");
	auto name = make_shared<WLineEdit>();
	name->setTooltip("Name or title of the chart");
	name->onChange = bind(&Dlg::mySetChartName, this);
	myForm->chartName = name;

	auto styleLabel = make_shared<WLabel>("Chart style");
	auto style = make_shared<WLineEdit>();
	style->setTooltip("Play style (e.g. \"Pad\", \"Keyboard\")");
	style->onChange = bind(&Dlg::mySetChartStyle, this);
	myForm->chartStyle = style;

	auto descLabel = make_shared<WLabel>("Description");
	auto desc = make_shared<WLineEdit>();
	desc->setTooltip("Description of the chart");
	desc->onChange = bind(&Dlg::mySetDescription, this);
	myForm->description = desc;

	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 0);

	grid->addColumn(88, 4);
	grid->addColumn(88, 0, 1);

	grid->addWidget(authorLabel, 0, 0);
	grid->addWidget(author, 0, 1);
	grid->addWidget(nameLabel, 1, 0);
	grid->addWidget(name, 1, 1);
	grid->addWidget(styleLabel, 2, 0);
	grid->addWidget(style, 2, 1);
	grid->addWidget(descLabel, 3, 0);
	grid->addWidget(desc, 3, 1);

	myForm->tabs->addTab(grid, "Metadata");
}

static shared_ptr<WLineEdit> AddNoteInfoRow(
	shared_ptr<WGrid> grid,
	Row row, int col,
	const char* name,
	const char* tooltip,
	int columnSpan = 1)
{
	auto label = make_shared<WLabel>(name);
	grid->addWidget(label, row, col, 1, columnSpan);

	auto edit = make_shared<WLineEdit>();
	edit->setEditable(false);
	edit->setTooltip(tooltip);
	grid->addWidget(edit, row, col + columnSpan);

	return edit;
}

void Dlg::myInitNoteInformationTab()
{
	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(24, 0);

	grid->addColumn(44, 4, 0);
	grid->addColumn(50, 4, 1);
	grid->addColumn(44, 4, 0);
	grid->addColumn(50, 4, 1);
	grid->addColumn(44, 4, 0);
	grid->addColumn(50, 0, 1);

	myForm->numSteps = AddNoteInfoRow(grid, 0, 0, "Steps",
		"Total number of notes excluding mines");

	myForm->numHolds = AddNoteInfoRow(grid, 0, 2, "Jumps",
		"Number of rows with two or more steps");

	myForm->numMines = AddNoteInfoRow(grid, 0, 4, "Mines",
		"Total number of mines");

	myForm->numJumps = AddNoteInfoRow(grid, 1, 0, "Holds",
		"Total number of hold notes");

	myForm->numRolls = AddNoteInfoRow(grid, 1, 2, "Rolls",
		"Total number of roll notes");

	myForm->numWarps = AddNoteInfoRow(grid, 1, 4, "Warps",
		"Total number of notes that are warped,\neither by a negative BPM or warp segment");

	myForm->noteDensity = AddNoteInfoRow(grid, 2, 0, "NPS",
		"Average number of notes per second");

	myForm->numMeasures = AddNoteInfoRow(grid, 2, 2, "Measures of stream",
		"Total number of measures with 16th streams", 3);

	myForm->tabs->addTab(grid, "Note Information");
}

void Dlg::myInitStreamBreakdownTab()
{
	auto simplify = make_shared<WCheckbox>("Simplify long breakdowns");
	simplify->setTooltip("Simplify breakdowns with many parts\nby merging streams and breaks");
	simplify->onChange = bind(&Dlg::myUpdateBreakdown, this);
	myForm->simplifyBreakdown = simplify;

	auto breakdown = make_shared<WBreakdownList>();
	myForm->breakdown = breakdown;

	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(breakdown, 0);

	grid->addColumn(128, 0, 1);

	grid->addWidget(simplify, 0, 0);
	grid->addWidget(breakdown, 1, 0);

	auto tab = myForm->tabs->addTab(grid, "Stream Breakdown");
}

// =====================================================================================================================
// Chart properties.

void Dlg::mySetDifficulty()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto difficulty = (Difficulty)myForm->difficulty->selectedItem();
	chart->difficulty.set(difficulty);
}

void Dlg::mySetMeter()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto meter = myForm->meter->ivalue();
	chart->meter.set(meter);
}

void Dlg::myCalcMeter()
{
	auto chart = Editor::currentChart();

	int rating = int(RatingEstimator::estimateRating(chart) * 10 + 0.5);
	if (rating == 190)
	{
		Hud::info("{g:calculate} Estimated rating: 19 or higher");
	}
	else
	{
		int intPart = rating / 10, decPart = rating % 10;
		const char* prefix = (decPart <= 3) ? "easy" : (decPart <= 6) ? "mid" : "hard";
		Hud::info("{g:calculate} Estimated rating: %i.%i (%s %i)", intPart, decPart, prefix, intPart);
	}
}

void Dlg::mySetChartName()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto name = myForm->chartName->text();
	chart->name.set(name);
}

void Dlg::mySetChartStyle()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto style = myForm->chartStyle->text();
	chart->style.set(style);
}

void Dlg::mySetDescription()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto description = myForm->description->text();
	chart->description.set(description);
}

void Dlg::mySetStepAuthor()
{
	auto chart = Editor::currentChart();
	if (!chart) return;
	auto author = myForm->stepAuthor->text();
	chart->author.set(author);
}

// =====================================================================================================================
// Note information.

static void StringifyNoteInfo(string& out, const char* name, int count)
{
	if (count > 0)
	{
		if (out.length()) out += ", ";
		out += format("{} {}", count, name);
		if (count > 1) out += 's';
	}
}

static void SetText(shared_ptr<WLineEdit> target, int num)
{
	target->setText(String::fromInt(num));
}

static void SetText(shared_ptr<WLineEdit> target, double num, int dec)
{
	target->setText(String::fromDouble(num, dec, dec));
}

void Dlg::myUpdateNoteInfo()
{
	auto chart = Editor::currentChart();
	auto& timing = Editor::currentTempo()->timing;

	int numSteps = 0;
	int numJumps = 0;
	int numHolds = 0;
	int numRolls = 0;
	int numMines = 0;
	int numWarps = 0;

	double density = 0.0;

	if (chart)
	{
		// Warped notes.
		for (auto& col : chart->notes)
		{
			bool insideWarp = false;
			auto note = col.begin();
			auto end = col.end();
			for (auto& event : timing.events())
			{
				if (insideWarp)
				{
					for (; note != end && note->row < event.row; ++note)
					{
						++numWarps;
					}
				}
				else
				{
					while (note != end && note->row <= event.row) ++note;
				}
				insideWarp = (event.spr == 0.0);
			}
		}
	}

	SetText(myForm->numSteps, numSteps);
	SetText(myForm->numJumps, numJumps);
	SetText(myForm->numMines, numMines);
	SetText(myForm->numHolds, numHolds);
	SetText(myForm->numRolls, numRolls);
	SetText(myForm->numWarps, numWarps);

	SetText(myForm->noteDensity, density, 1);
	SetText(myForm->numMeasures, myForm->breakdown->getTotalMeasureCount());
}

// =====================================================================================================================
// Stream breakdown.

void Dlg::myUpdateBreakdown()
{
	//myFormBreakdown->bList.update(Editor::currentChart(), mySimplifyBreakdown);
}

// =====================================================================================================================
// Other functions.

static void EnableWidget(Widget& w)
{
	w.setEnabled(true);
}

static void DisableWidget(Widget& w)
{
	w.setEnabled(false);
}

void Dlg::myUpdateChart()
{
	myForm->playStyle->clearItems();
	myForm->gameMode->clearItems();

	auto chart = Editor::currentChart();
	if (chart)
	{
		myForm->tabs->setEnabled(true);
		myForm->playStyle->addItem(chart->gameMode->mode.data());
		myForm->gameMode->addItem(chart->gameMode->game.data());

		myForm->chartName->setText(chart->name.get());
		myForm->chartStyle->setText(chart->style.get());
		myForm->description->setText(chart->description.get());
		myForm->stepAuthor->setText(chart->author.get());

		myForm->difficulty->setSelectedItem((int)chart->difficulty.get());
		myForm->meter->setValue(chart->meter.get());
	}
	else
	{
		myForm->tabs->setEnabled(false);

		myForm->chartName->setText(string());
		myForm->chartStyle->setText(string());
		myForm->description->setText(string());
		myForm->stepAuthor->setText(string());
		
		myForm->difficulty->setSelectedItem(0);
		myForm->meter->setValue(0);
	}

	// The game mode and play style can never be changed on existing charts.
	myForm->gameMode->setEnabled(false);
	myForm->playStyle->setEnabled(false);
}

} // namespace AV
