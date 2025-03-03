#include <Precomp.h>

#include <Vortex/View/Dialogs/GenerateNotes.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/Slider.h>
#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Checkbox.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Droplist.h>
#include <Core/Interface/Widgets/Label.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Managers/GameMan.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/StreamGenerator.h>

#include <Vortex/View/Hud.h>

#define Dlg DialogGenerateNotes

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// DialogGenerateNotes :: FormBase.

struct Dlg::Form
{
	shared_ptr<WDroplist> spacing;
	shared_ptr<WSpinner> single;
	shared_ptr<WSpinner> paired;
	shared_ptr<WSlider> difficulty;
	shared_ptr<WCheckbox> startingFoot;
	shared_ptr<WButton> generate;

	shared_ptr<WGrid> grid;
};

// =====================================================================================================================
// Constructor and destructor.

Dlg::~Dlg()
{
	delete myForm;
}

Dlg::Dlg()
{
	setTitle("GENERATE NOTES");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitWidgets();
}

// =====================================================================================================================
// Widget functions.

static const char* SpacingStrings[] =
{
	"4th",
	"8th",
	"12th",
	"16th",
	"20th",
	"24th",
	"32nd"
};

static SnapType SpacingTypes[] =
{
	SnapType::S4,
	SnapType::S8,
	SnapType::S12,
	SnapType::S16,
	SnapType::S20,
	SnapType::S24,
	SnapType::S32
};

static constexpr int IFPSize = 24;
static constexpr int IFPSpacing = 4;

void Dlg::myInitWidgets()
{
	// ==========================================
	// FormBase

	myForm = new Form;

	/*main->lSpacing = make_shared<WLabel>();
	main->dSpacing = make_shared<WDroplist>();
	main->lRepetition = make_shared<WLabel>();
	main->lSingle = make_shared<WLabel>();
	main->sSingle = make_shared<WSpinner>();
	main->lPaired = make_shared<WLabel>();
	main->sPaired = make_shared<WSpinner>();
	main->lDifficulty = make_shared<WLabel>();
	main->sDifficulty = make_shared<WSlider>();
	main->cStartingFoot = make_shared<WCheckbox>();
	main->bGenerate = make_shared<WButton>();

	main->lSpacing->setText("Spacing");

	main->dSpacing->value.bind(&mySpacing);
	for (int i = 0; i < 6; ++i)
	{
		main->dSpacing->addItem(SpacingStrings[i]);
	}

	main->lRepetition->setText("Maximum note repetition");

	main->lSingle->setText("Single column");
	main->sSingle->value.bind(&myStream.maxColRep);
	main->sSingle->setRange(1.0, 16.0);
	main->sSingle->setPrecision(0, 0);

	main->lPaired->setText("Paired columns");
	main->sPaired->value.bind(&myStream.maxBoxRep);
	main->sPaired->setRange(1.0, 16.0);
	main->sPaired->setPrecision(0, 0);

	main->lDifficulty->setText("Difficulty");

	main->sDifficulty->value.bind(&myStream.patternDifficulty);

	main->cStartingFoot->text.set("Start with right foot");
	main->cStartingFoot->value.bind(&myStream.startWithRight);

	main->bGenerate->text.set("Generate notes");
	main->bGenerate->onPress.bind(this, &Dlg::myGenerateNotes);

	onChanges(0xFFFFFFFF);

	myContent = main->grid;*/
}

void Dlg::myGenerateNotes()
{
	auto region = Selection::getRegion();
	auto chart = Editor::currentChart();
	if (!chart)
	{
		Hud::note("%s", "Open a chart first.");
	}
	else if (region.beginRow == region.endRow)
	{
		Hud::note("%s", "Select a region first.");
	}
	else
	{
		auto feet = chart->gameMode->padInitialFeetCols[0];
		auto spacing = myForm->spacing->selectedItem();
		myStream.generate(region.beginRow, region.endRow, feet, SpacingTypes[spacing]);
	}
}

} // namespace AV
