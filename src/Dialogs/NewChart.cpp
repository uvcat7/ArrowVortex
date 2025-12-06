#include <Dialogs/NewChart.h>

#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>

#include <Simfile/Chart.h>

#include <Managers/MetadataMan.h>
#include <Managers/StyleMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Common.h>
#include <Editor/Editor.h>

namespace Vortex {

DialogNewChart::~DialogNewChart() = default;

DialogNewChart::DialogNewChart() : myDifficulty(0), myRating(1), myStyle(0) {
    setTitle("NEW CHART");
    myCreateWidgets();
    onChanges(VCM_ALL_CHANGES);
}

void DialogNewChart::myCreateWidgets() {
    myLayout.row().col(76).col(232);

    myStyleList = myLayout.add<WgDroplist>("Chart type");
    myStyleList->value.bind(&myStyle);
    myStyleList->setTooltip("Game style of the chart");
    for (int i = 0; i < gStyle->getNumStyles(); ++i) {
        myStyleList->addItem(gStyle->get(i)->name);
    }

    myLayout.row().col(76).col(148).col(80);

    WgDroplist* diffs = myLayout.add<WgDroplist>("Difficulty");
    diffs->value.bind(&myDifficulty);
    diffs->setTooltip("Difficulty type of the chart");
    for (int i = 0; i < NUM_DIFFICULTIES; ++i) {
        diffs->addItem(GetDifficultyName(static_cast<Difficulty>(i)));
    }

    WgSpinner* rating = myLayout.add<WgSpinner>();
    rating->setTooltip("Difficulty rating/meter of the chart");
    rating->value.bind(&myRating);
    rating->setRange(1.0, 100000.0);

    myLayout.row().col(76).col(232);

    WgLineEdit* artist = myLayout.add<WgLineEdit>("Step artist");
    artist->setTooltip("Author of the chart");
    artist->text.bind(&myStepArtist);

    myLayout.row().col(312);
    myLayout.add<WgSeperator>();

    myLayout.row().col(216).col(92);
    myLayout.add<GuiWidget>();

    WgButton* accept = myLayout.add<WgButton>();
    accept->text.set("Create");
    accept->onPress.bind(this, &DialogNewChart::myCreateChart);
    accept->setTooltip("Create a chart with the current settings");
}

void DialogNewChart::myCreateChart() {
    if (gSimfile->isOpen()) {
        auto difficulty = static_cast<Difficulty>(myDifficulty);
        gSimfile->addChart(gStyle->get(myStyle), myStepArtist, difficulty,
                           myRating);
        requestClose();
    } else {
        HudNote("%s", "Open a simfile or music file first.");
    }
}

};  // namespace Vortex