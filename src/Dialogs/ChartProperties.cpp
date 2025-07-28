#include <Dialogs/ChartProperties.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <System/System.h>

#include <Editor/Editor.h>
#include <Managers/MetadataMan.h>
#include <Managers/StyleMan.h>
#include <Editor/Selection.h>
#include <Editor/View.h>
#include <Editor/Notefield.h>
#include <Editor/Common.h>
#include <Managers/ChartMan.h>

namespace Vortex {

enum Result {
    RESULT_DELETE = 1,
    RESULT_ACCEPT,
    RESULT_CANCEL,
};

enum NoteItemType {
    NIT_STEPS = 0,
    NIT_JUMPS,
    NIT_MINES,
    NIT_HOLDS,
    NIT_ROLLS,
    NIT_WARPS,
};

static const char* noteItemLabels[] = {"steps", "jumps", "mines",
                                       "holds", "rolls", "warps"};

DialogChartProperties::~DialogChartProperties() {}

DialogChartProperties::DialogChartProperties()
    : myDifficulty(0), myRating(1), myStyle(0) {
    setTitle("CHART PROPERTIES");

    myCreateChartProperties();
    myCreateNoteInfo();
    myCreateBreakdown();

    onChanges(VCM_ALL_CHANGES);
}

void DialogChartProperties::onChanges(int changes) {
    if (changes & (VCM_CHART_PROPERTIES_CHANGED | VCM_CHART_CHANGED)) {
        myStyleList->clearItems();
        if (gChart->isOpen()) {
            for (auto w : myLayout) w->setEnabled(true);
            myStyleList->setEnabled(false);

            myRating = gChart->getMeter();
            myStepArtist = gChart->getStepArtist();
            myDifficulty = gChart->getDifficulty();
            myStyleList->addItem(gStyle->get()->name.c_str());

        } else {
            for (auto w : myLayout) w->setEnabled(false);

            myRating = 0;
            myStepArtist.clear();
            myDifficulty = -1;
            myStyleList->clearItems();
        }
    }

    // Update the chart breakdown and note counts if notes change.
    if (changes & VCM_NOTES_CHANGED) {
        myUpdateNoteInfo();
        myUpdateBreakdown();
    }
}

// ================================================================================================
// Chart properties.

void DialogChartProperties::myCreateChartProperties() {
    myLayout.row().col(76).col(260);

    myStyleList = myLayout.add<WgDroplist>("Chart type");
    myStyleList->value.bind(&myStyle);
    myStyleList->setEnabled(false);
    myStyleList->setTooltip("Game style of the chart");

    myLayout.row().col(76).col(148).col(80).col(24);

    auto diff = myLayout.add<WgDroplist>("Difficulty");
    diff->value.bind(&myDifficulty);
    diff->onChange.bind(this, &DCP::mySetDifficulty);
    for (int i = 0; i < NUM_DIFFICULTIES; ++i) {
        diff->addItem(GetDifficultyName((Difficulty)i));
    }
    diff->setTooltip("Difficulty type of the chart");

    WgSpinner* meter = myLayout.add<WgSpinner>();
    meter->value.bind(&myRating);
    meter->setRange(1.0, 100000.0);
    meter->onChange.bind(this, &DCP::mySetRating);
    meter->setTooltip("Difficulty rating/meter of the chart");

    WgButton* calc = myLayout.add<WgButton>();
    calc->text.set("{g:calculate}");
    calc->onPress.bind(this, &DCP::myCalcRating);
    calc->setTooltip("Estimate the chart difficulty by analyzing the notes");

    myLayout.row().col(76).col(260);

    WgLineEdit* artist = myLayout.add<WgLineEdit>("Step artist");
    artist->text.bind(&myStepArtist);
    artist->onChange.bind(this, &DCP::mySetStepArtist);
    artist->setTooltip("Author of the chart");
}

void DialogChartProperties::mySetStepArtist() {
    gChart->setStepArtist(myStepArtist);
}

void DialogChartProperties::mySetDifficulty() {
    gChart->setDifficulty((Difficulty)myDifficulty);
}

void DialogChartProperties::mySetRating() { gChart->setMeter(myRating); }

void DialogChartProperties::myCalcRating() {
    int rating = (int)(gChart->getEstimatedMeter() * 10 + 0.5);
    if (rating == 190) {
        HudInfo("{g:calculate} Estimated rating: 19 or higher");
    } else {
        int intPart = rating / 10, decPart = rating % 10;
        const char* prefix = (decPart <= 3)   ? "easy"
                             : (decPart <= 6) ? "mid"
                                              : "hard";
        HudInfo("{g:calculate} Estimated rating: %i.%i (%s %i)", intPart,
                decPart, prefix, intPart);
    }
}

// ================================================================================================
// Note information.

static void StringifyNoteInfo(std::string& out, const char* name, int count) {
    if (count > 0) {
        if (out.length()) out = out + ", ";
        out = out + Str::fmt("%1 %2").arg(count).arg(name).str;
        if (count > 1) out = out + "s";
    }
}

void DialogChartProperties::myCreateNoteInfo() {
    myLayout.row().col(340);
    myLayout.add<WgSeperator>();

    myLayout.row().col(24).col(312);

    WgButton* copy = myLayout.add<WgButton>();
    copy->text.set("{g:copy}");
    copy->setTooltip("Copy note information to clipboard");
    copy->onPress.bind(this, &DCP::myCopyNoteInfo);

    WgLabel* info = myLayout.add<WgLabel>();
    info->text.set("Note information");

    const char* tooltips[] = {
        "Select all steps",      "Select all jump notes",
        "Select all mines",      "Select all hold notes",
        "Select all roll notes", "Select all warped notes",
    };

    myLayout.row().col(53).col(53).col(53).col(53).col(53).col(53);
    for (int i = 0; i < 6; ++i) {
        WgButton* b = myLayout.add<WgButton>(noteItemLabels[i]);
        b->onPress.bind(this, &DCP::mySelectNotes, i);
        b->setTooltip(tooltips[i]);
        myNoteInfo[i] = b;
    }

    myLayout.row().col(162).col(162);
    myNoteDensity = myLayout.add<WgLabel>();
    myStreamMeasureCount = myLayout.add<WgLabel>();
}

void DialogChartProperties::myUpdateNoteInfo() {
    int count[6] = {0, 0, 0, 0, 0, 0};
    count[0] = gNotes->getNumSteps();
    count[1] = gNotes->getNumJumps();
    count[2] = gNotes->getNumMines();
    count[3] = gNotes->getNumHolds();
    count[4] = gNotes->getNumRolls();
    count[5] = gNotes->getNumWarps();
    for (int i = 0; i < 6; ++i) {
        myNoteInfo[i]->setEnabled(count[i] > 0);
        myNoteInfo[i]->text.set(Str::val(count[i]));
    }

    double density = 0.0;
    if (gNotes->begin() < gNotes->end()) {
        density = (double)count[0] /
                  max(1.0, (gNotes->end() - 1)->time - gNotes->begin()->time);
    }

    myNoteDensity->text.set(
        Str::fmt("Note density: %1 NPS").arg(density, 1, 1).str);
}

void DialogChartProperties::myCopyNoteInfo() {
    std::string out;
    if (gChart->isOpen()) {
        StringifyNoteInfo(out, "step", gNotes->getNumSteps());
        StringifyNoteInfo(out, "jump", gNotes->getNumJumps());
        StringifyNoteInfo(out, "mine", gNotes->getNumMines());
        StringifyNoteInfo(out, "hold", gNotes->getNumHolds());
        StringifyNoteInfo(out, "roll", gNotes->getNumRolls());
        StringifyNoteInfo(out, "warp", gNotes->getNumWarps());
    }
    if (out.empty()) {
        HudInfo("%s", "There is no note info to copy...");
    } else {
        gSystem->setClipboardText(out.c_str());
        HudInfo("%s%s", "Note info copied to clipboard: ", out.c_str());
    }
}

void DialogChartProperties::mySelectNotes(int type) {
    NotesMan::Filter f = NotesMan::SELECT_STEPS;
    switch (type) {
        case NIT_STEPS:
            f = NotesMan::SELECT_STEPS;
            break;
        case NIT_JUMPS:
            f = NotesMan::SELECT_JUMPS;
            break;
        case NIT_MINES:
            f = NotesMan::SELECT_MINES;
            break;
        case NIT_HOLDS:
            f = NotesMan::SELECT_HOLDS;
            break;
        case NIT_ROLLS:
            f = NotesMan::SELECT_ROLLS;
            break;
        case NIT_WARPS:
            f = NotesMan::SELECT_WARPS;
            break;
    };
    gSelection->selectNotes(f);
}

// ================================================================================================
// Stream breakdown.

class DialogChartProperties::BreakdownWidget : public GuiWidget {
   public:
    ~BreakdownWidget();
    BreakdownWidget(GuiContext* gui);

    void updateBreakdown(WgLabel* measureCount);
    void selectStream(vec2i rows);

    void onUpdateSize() override;
    void onArrange(recti r) override;
    void onTick() override;
    void onDraw() override;

   private:
    DialogChartProperties* myDialog;
    Vector<WgButton*> myButtons;
};

DialogChartProperties::BreakdownWidget::~BreakdownWidget() {
    for (auto button : myButtons) {
        delete button;
    }
}

DialogChartProperties::BreakdownWidget::BreakdownWidget(GuiContext* gui)
    : GuiWidget(gui) {}

void DialogChartProperties::BreakdownWidget::updateBreakdown(
    WgLabel* measureCount) {
    int measures = 0;
    auto breakdown = gChart->getStreamBreakdown(&measures);
    for (int i = 0; i < breakdown.size(); ++i) {
        auto& item = breakdown[i];

        Text::arrange(Text::TL, TextStyle(), item.text.c_str());
        int w = max(16, Text::getSize().x + 8);

        if (i >= myButtons.size()) {
            myButtons.push_back(new WgButton(getGui()));
        }

        WgButton* button = myButtons[i];
        button->text.set(item.text.c_str());
        button->setSize(w, 20);
        button->onPress.bind(this, &BreakdownWidget::selectStream,
                             vec2i{item.row, item.endrow});
    }
    while (myButtons.size() > breakdown.size()) {
        delete myButtons.back();
        myButtons.pop_back();
    }
    measureCount->text.set(Str::fmt("Stream measures: %1").arg(measures).str);
}

void DialogChartProperties::BreakdownWidget::selectStream(vec2i rows) {
    gView->setCursorRow(rows.x);
    gSelection->selectRegion(rows.x, rows.y);
}

void DialogChartProperties::BreakdownWidget::onUpdateSize() {
    int x = 0, y = 20;
    for (auto button : myButtons) {
        vec2i size = button->getSize();
        if (x + size.x > 340) {
            x = 0, y += 22;
        }
        x += size.x + 2;
    }
    setSize(340, y);
}

void DialogChartProperties::BreakdownWidget::onArrange(recti r) {
    int x = 0, y = 0;
    for (auto button : myButtons) {
        vec2i size = button->getSize();
        if (x + size.x > r.w) {
            x = 0, y += 22;
        }
        button->arrange({r.x + x, r.y + y, size.x, size.y});
        x += size.x + 2;
    }
}

void DialogChartProperties::BreakdownWidget::onTick() {
    for (auto button : myButtons) {
        button->tick();
    }
}

void DialogChartProperties::BreakdownWidget::onDraw() {
    for (auto button : myButtons) {
        button->draw();
    }
}

void DialogChartProperties::myCreateBreakdown() {
    myLayout.row().col(340);
    myLayout.add<WgSeperator>();

    myLayout.row().col(24).col(312);

    WgButton* copy = myLayout.add<WgButton>();
    copy->text.set("{g:copy}");
    copy->setTooltip("Copy stream breakdown to clipboard");
    copy->onPress.bind(this, &DCP::myCopyBreakdown);

    WgLabel* info = myLayout.add<WgLabel>();
    info->text.set("Stream breakdown");

    myLayout.row().col(340);
    myBreakdown = new BreakdownWidget(getGui());
    myLayout.add(myBreakdown);
}

void DialogChartProperties::myUpdateBreakdown() {
    myBreakdown->updateBreakdown(myStreamMeasureCount);
}

void DialogChartProperties::myCopyBreakdown() {
    auto breakdown = gChart->getStreamBreakdown();
    if (breakdown.empty()) {
        HudInfo("%s", "There is no breakdown to copy...");
    } else {
        std::string out;
        for (auto& item : breakdown) {
            out = out + item.text;
            out = out + "/";
        }
        Str::pop_back(out);
        gSystem->setClipboardText(out);
        HudInfo("%s%s", "Stream breakdown copied to clipboard: ", out.c_str());
    }
}

};  // namespace Vortex
