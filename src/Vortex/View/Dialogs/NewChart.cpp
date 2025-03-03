#include <Precomp.h>

#include <Vortex/View/Dialogs/NewChart.h>

#include <Core/Graphics/Draw.h>
#include <Core/Utils/Util.h>

#include <Core/Interface/Widgets/LineEdit.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/Droplist.h>
#include <Core/Interface/Widgets/ScrollRegion.h>
#include <Core/Interface/Widgets/Label.h>

#include <Core/System/Debug.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>

#include <Vortex/Managers/GameMan.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>

#include <Vortex/View/Hud.h>

#define Dlg DialogNewChart

namespace AV {

using namespace std;
using namespace Util;

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
};

// =====================================================================================================================
// Constructor and destructor.

Dlg::~Dlg()
{
	delete myForm;
}

Dlg::Dlg()
{
	setTitle("NEW CHART");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitMainTab();
	myInitMetadataTab();
	myInitCreateTab();
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
	meter->setTooltip("Difficulty rating of the chart");
	myForm->meter = meter;

	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(22, 0);

	grid->addColumn(88, 4);
	grid->addColumn(88, 4, 1);
	grid->addColumn(16, 4);
	grid->addColumn(82, 0);

	grid->addWidget(typeLabel, 0, 0);
	grid->addWidget(gameMode, 0, 1);
	grid->addWidget(playStyle, 0, 2, 1, 2);
	grid->addWidget(diffLabel, 1, 0);
	grid->addWidget(diff, 1, 1, 1, 2);
	grid->addWidget(meter, 1, 3);

	myForm->tabs->addTab(grid);
}

void Dlg::myInitMetadataTab()
{
	auto authorLabel = make_shared<WLabel>("Step author");
	auto author = make_shared<WLineEdit>();
	author->setTooltip("Creator of the chart");
	myForm->stepAuthor = author;

	auto nameLabel = make_shared<WLabel>("Chart name");
	auto name = make_shared<WLineEdit>();
	name->setTooltip("Name or title of the chart");
	myForm->chartName = name;

	auto styleLabel = make_shared<WLabel>("Chart style");
	auto style = make_shared<WLineEdit>();
	style->setTooltip("Play style (e.g. \"Pad\", \"Keyboard\")");
	myForm->chartStyle = style;

	auto descLabel = make_shared<WLabel>("Description");
	auto desc = make_shared<WLineEdit>();
	desc->setTooltip("Description of the chart");
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

void Dlg::myInitCreateTab()
{
	auto create = make_shared<WButton>("Create");
	create->setTooltip("Create a chart with the current settings");
	create->onPress = bind(&Dlg::myCreateChart, this);

	auto grid = make_shared<WGrid>();

	grid->addRow(0, 0, 1);
	grid->addRow(24, 4);

	grid->addColumn(32, 4, 1);
	grid->addColumn(128, 4);

	grid->addWidget(create, 1, 1);

	myForm->tabs->addTab(grid);
}

void Dlg::myCreateChart()
{
	if (auto sim = Editor::currentSimfile())
	{
		GameMode* gameMode = nullptr;

		int game = myForm->gameMode->selectedItem();
		int mode = myForm->gameMode->selectedItem();
		auto games = GameModeMan::getGames();
		if (game >= 0 && game < (int)games.size())
		{
			auto modes = GameModeMan::getModes(games[game]);
			if (mode >= 0 && mode < (int)modes.size())
			{
				gameMode = modes[mode];
			}
		}

		if (gameMode)
		{
			auto chart = make_shared<Chart>(sim, gameMode);

			chart->name.set(myForm->chartName->text());
			chart->style.set(myForm->chartName->text());
			chart->author.set(myForm->stepAuthor->text());
			chart->description.set(myForm->description->text());
			chart->difficulty.set((Difficulty)myForm->difficulty->selectedItem());
			chart->meter.set(myForm->meter->ivalue());

			sim->addChart(chart);

			if (!isPinned()) requestClose();
		}
	}
	else
	{
		Hud::note("%s", "Open a simfile or music file first.");
	}
}

void Dlg::myUpdateModes()
{
	myForm->playStyle->clearItems();
	int game = myForm->gameMode->selectedItem();
	auto games = GameModeMan::getGames();
	if (game >= 0 && game < (int)games.size())
	{
		auto modes = GameModeMan::getModes(games[game]);
		for (auto mode : modes)
			myForm->playStyle->addItem(mode->mode);

		auto mode = myForm->gameMode->selectedItem();
		myForm->gameMode->setSelectedItem(clamp(mode, 0, (int)modes.size() - 1));
	}
}

} // namespace AV
