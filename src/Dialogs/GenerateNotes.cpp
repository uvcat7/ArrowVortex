#include <Dialogs/GenerateNotes.h>

#include <Core/WidgetsLayout.h>

#include <Editor/Common.h>
#include <Editor/Selection.h>
#include <Editor/StreamGenerator.h>

#include <Core/Utils.h>

#include <Core/Core.h>
#include <Core/Widgets.h>
#include <Managers/SimfileMan.h>
#include <Managers/StyleMan.h>

namespace Vortex {

static const char* SpacingStrings[] = {"4th",  "8th",  "12th",
                                       "16th", "24th", "32nd"};

static SnapType SpacingTypes[] = {ST_4TH,  ST_8TH,  ST_12TH,
                                  ST_16TH, ST_24TH, ST_32ND};

static const int IFP_SIZE = 24;
static const int IFP_SPACING = 4;

DialogGenerateNotes::~DialogGenerateNotes() = default;

DialogGenerateNotes::DialogGenerateNotes()
    : footSelectionIndex_(0), spacingValue_(0) {
    setTitle("GENERATE NOTES");
    myCreateWidgets();
}

void DialogGenerateNotes::myCreateWidgets() {
    myLayout.row().col(true);
    myLayout.addBlank();

    myLayout.row().col(60).col(116);
    spacingDroplist_ = myLayout.add<WgDroplist>("Spacing");
    spacingDroplist_->value.bind(&spacingValue_);
    for (auto& SpacingString : SpacingStrings) {
        spacingDroplist_->addItem(SpacingString);
    }

    myLayout.row().col(180);
    WgLabel* repetitions = myLayout.add<WgLabel>();
    repetitions->text.set("Maximum note repetition");

    myLayout.row().col(116).col(60);

    WgSpinner* scol = myLayout.add<WgSpinner>("Single column");
    scol->value.bind(&streamGenerator_.maxColRep);
    scol->setRange(1.0, 16.0);
    scol->setPrecision(0, 0);

    WgSpinner* pcol = myLayout.add<WgSpinner>("Paired columns");
    pcol->value.bind(&streamGenerator_.maxBoxRep);
    pcol->setRange(1.0, 16.0);
    pcol->setPrecision(0, 0);

    myLayout.row().col(60).col(116);

    WgSlider* sdiff = myLayout.add<WgSlider>("Difficulty");
    sdiff->value.bind(&streamGenerator_.patternDifficulty);

    myLayout.row().col(180);

    WgCheckbox* cfoot = myLayout.add<WgCheckbox>();
    cfoot->text.set("Start with right foot");
    cfoot->value.bind(&streamGenerator_.startWithRight);

    WgButton* generate = myLayout.add<WgButton>();
    generate->text.set("Generate notes");
    generate->onPress.bind(this, &DialogGenerateNotes::myGenerateNotes);

    onChanges(VCM_ALL_CHANGES);
}

void DialogGenerateNotes::onChanges(int changes) {
    if (changes & VCM_CHART_CHANGED) {
        int w = 180;
        auto style = gStyle->get();
        if (style && style->padWidth > 0) {
            w = max(w, gStyle->getNumCols() * (IFP_SIZE + IFP_SPACING));
            streamGenerator_.feetCols = style->padInitialFeetCols[0];
        }
    }
}

void DialogGenerateNotes::myGenerateNotes() {
    auto region = gSelection->getSelectedRegion();
    if (gSimfile->isClosed()) {
        HudNote("%s", "Open a simfile or music file first.");
    } else if (region.isOmni()) {
        HudNote("%s", "Select a region first.");
    } else {
        streamGenerator_.generate(region.beginRow, region.endRow,
                                  SpacingTypes[spacingValue_]);
    }
}

};  // namespace Vortex