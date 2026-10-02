#include <Dialogs/ChartProperties.h>

#include <Core/Draw.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/Notefield.h>
#include <Editor/Selection.h>
#include <Editor/View.h>

#include <Managers/ChartMan.h>
#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/StyleMan.h>
#include <Managers/TempoMan.h>

#include <System/System.h>
#include <cmath>

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

DialogChartProperties::~DialogChartProperties() = default;

DialogChartProperties::DialogChartProperties() {
    setTitle("CHART PROPERTIES");

    myCreateChartProperties();
    myCreateNoteInfo();
    myCreateGraph();
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
    if (changes & (VCM_NOTES_CHANGED | VCM_TEMPO_CHANGED)) {
        myUpdateGraph();
        myUpdateNoteInfo();
        myUpdateBreakdown();
    }

    if (changes & (VCM_TEMPO_CHANGED | VCM_END_ROW_CHANGED)) {
        myUpdateGraph();
        myUpdateNoteInfo();
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
    diff->onChange.bind(this, &DialogChartProperties::mySetDifficulty);
    for (int i = 0; i < NUM_DIFFICULTIES; ++i) {
        diff->addItem(GetDifficultyName(static_cast<Difficulty>(i)));
    }
    diff->setTooltip("Difficulty type of the chart");

    WgSpinner* meter = myLayout.add<WgSpinner>();
    meter->value.bind(&myRating);
    meter->setRange(1.0, 100000.0);
    meter->onChange.bind(this, &DialogChartProperties::mySetRating);
    meter->setTooltip("Difficulty rating/meter of the chart");

    WgButton* calc = myLayout.add<WgButton>();
    calc->text.set("{g:calculate}");
    calc->onPress.bind(this, &DialogChartProperties::myCalcRating);
    calc->setTooltip("Estimate the chart difficulty by analyzing the notes");

    myLayout.row().col(76).col(260);

    WgLineEdit* artist = myLayout.add<WgLineEdit>("Step artist");
    artist->text.bind(&myStepArtist);
    artist->onChange.bind(this, &DialogChartProperties::mySetStepArtist);
    artist->setTooltip("Author of the chart");
}

void DialogChartProperties::mySetStepArtist() {
    gChart->setStepArtist(myStepArtist);
}

void DialogChartProperties::mySetDifficulty() {
    gChart->setDifficulty(static_cast<Difficulty>(myDifficulty));
}

void DialogChartProperties::mySetRating() { gChart->setMeter(myRating); }

void DialogChartProperties::myCalcRating() {
    int rating = static_cast<int>(gChart->getEstimatedMeter() * 10 + 0.5);
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
    copy->onPress.bind(this, &DialogChartProperties::myCopyNoteInfo);

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
        b->onPress.bind(this, &DialogChartProperties::mySelectNotes, i);
        b->setTooltip(tooltips[i]);
        myNoteInfo[i] = b;
    }

    myLayout.row().col(162).col(162);
    myNoteDensity = myLayout.add<WgLabel>();
    myStreamMeasureCount = myLayout.add<WgLabel>();
    myPeakDensity = myLayout.add<WgLabel>();
    my16thMeasureCount = myLayout.add<WgLabel>();
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
        density =
            static_cast<double>(gNotes->getNumJudge()) /
            std::max(1.0, (gNotes->end() - 1)->time - gNotes->begin()->time);
    }

    myNoteDensity->text.set(
        Str::fmt("Avg density: %1 NPS").arg(density, 1, 1).str);
    myPeakDensity->text.set(
        Str::fmt("Peak density: %1 NPS").arg(myGraph->peakNps(), 1, 1).str);
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
// NPS Graph.

DialogChartProperties::GraphWidget::~GraphWidget() = default;
DialogChartProperties::GraphWidget::GraphWidget(GuiContext* gui)
    : GuiWidget(gui) {
    width_ = 340;
    height_ = 100;
}
double DialogChartProperties::GraphWidget::peakNps() { return peak; };

void DialogChartProperties::GraphWidget::updateGraph() {
    auto measure_time = [&](int measure) {
        return gTempo->rowToTime(measure * ROWS_PER_MEASURE);
    };
    auto delta_measure_time = [&](int measure, int offset) {
        return measure_time(measure + offset) - measure_time(measure);
    };

    if (gNotes->empty()) {
        return;
    }
    endMeasure = (gSimfile->getEndRow() - 1) / ROWS_PER_MEASURE + 1;
    endTime = gTempo->rowToTime(gSimfile->getEndRow());
    scale = endMeasure / gSystem->applyScaleFactor(width_);
    if (scale < 1) scale = 1;
    int buckets = endMeasure;
    peak = 0;
    data.resize(buckets);
    for (int i = 0; i < buckets; i++) data[i] = 0;
    for (auto& note : *gNotes) {
        int measure = note.row / ROWS_PER_MEASURE;
        if (note.isMine || note.isWarped || note.isFake) continue;
        data[measure]++;
    }
    int slices = 1;

    for (int i = 0; i < buckets; i += slices) {
        slices = 1;
        int notes = data[i];
        // Check 1 second to get good NPS estimates
        while (measure_time(i + slices) - measure_time(i) < 1.0f &&
               i + slices < buckets) {
            notes += data[i + slices];
            slices++;
        }
        peak = std::max(
            peak, static_cast<double>(
                      notes / (measure_time(i + slices) - measure_time(i))));
    }

    if (peak <= 0.0f) return;

    fill_commands.clear();

    int scale_width = gSystem->applyScaleFactor(width_);
    endTime = gTempo->rowToTime(gSimfile->getEndRow());
    buckets = data.size();
    double barWidth = (static_cast<double>(scale_width) / buckets);
    int w = barWidth + 1;
    slices = 1;

    for (int i = 0; i < buckets; i += slices) {
        slices = 1;
        int x = static_cast<int>(measure_time(i) / endTime * scale_width);
        int notes = data[i];
        while (delta_measure_time(i, slices + 1) <= endTime / scale_width &&
               // Averaging looks bad beyond 30 seconds
               delta_measure_time(i, slices + 1) <= 30.f &&
               i + slices < buckets) {
            notes += data[i + slices];
            slices++;
        }
        if (notes <= 0) continue;
        int h = std::min(
            height_,
            static_cast<int>(std::round(notes / delta_measure_time(i, slices) /
                                        peak * height_)));
        w = static_cast<int>(measure_time(i + slices) / endTime * scale_width) -
            x;
        int y = height_ - h;
        if (w == 0 || h == 0) continue;
        fill_commands.push_back({x, y, w, h});
    }
}

void DialogChartProperties::GraphWidget::onDraw() {
    if (gNotes->empty() || peak <= 0.0f) {
        Draw::fill(rect_, Color32(20, 20, 20, 255));
        return;
    }
    int scale_width = gSystem->applyScaleFactor(width_);
    endTime = gTempo->rowToTime(gSimfile->getEndRow());
    auto batch = Renderer::batchC();
    Draw::fill(rect_, Color32(20, 20, 20, 255));

    for (auto recti : fill_commands) {
        Draw::fill(&batch,
                   {recti.x + rect_.x, recti.y + rect_.y, recti.w, recti.h},
                   Color32(80, 80, 80, 255));
    }

    double time = std::min(endTime, gView->getCursorTime());
    int x = static_cast<int>(time / endTime * scale_width);
    Draw::fill(&batch, {rect_.x + x, rect_.y, 1, height_},
               Color32(160, 160, 160, 255));
    batch.flush();
};

void DialogChartProperties::myCreateGraph() {
    myLayout.row().col(340);
    myLayout.add<WgSeperator>();

    myLayout.row().col(340);
    myGraph = new GraphWidget(getGui());
    myLayout.add(myGraph);
}

void DialogChartProperties::myUpdateGraph() { myGraph->updateGraph(); }

// ================================================================================================
// Stream breakdown.

DialogChartProperties::BreakdownWidget::~BreakdownWidget() {
    for (auto button : myButtons) {
        delete button;
    }
}

DialogChartProperties::BreakdownWidget::BreakdownWidget(GuiContext* gui)
    : GuiWidget(gui) {}

void DialogChartProperties::BreakdownWidget::updateBreakdown(
    WgLabel* measureCount, WgLabel* measureCount16, bool compressed) {
    int measures = 0;
    int measures16 = 0;
    auto breakdown =
        gChart->getStreamBreakdown(&measures, &measures16, compressed);
    bool asteriskize = compressed && breakdown.size() > 25;

    int total_run = 0;
    bool add_asterisk = false;
    int run_row = 0;
    int buttons = 0;
    int w = 0;
    for (int i = 0; i < breakdown.size(); ++i) {
        auto& item = breakdown[i];

        if (asteriskize) {
            if (item.text[0] == '/' || item.text[0] == '|') {
                if (buttons >= myButtons.size()) {
                    myButtons.emplace_back(new WgButton(getGui()));
                }
                WgButton* button = myButtons[buttons];
                std::string run_text = std::to_string(total_run);
                if (add_asterisk) run_text += "*";
                Text::arrange(Text::TL, TextStyle(), run_text.c_str());
                int w = std::max(16, Text::getSize().x + 8);
                button->text.set(run_text.c_str());
                button->setSize(w, gSystem->applyScaleFactor(20));
                button->onPress.bind(this, &BreakdownWidget::selectStream,
                                     vec2i{run_row, item.row});
                buttons++;
                if (buttons >= myButtons.size()) {
                    myButtons.emplace_back(new WgButton(getGui()));
                }
                WgButton* button2 = myButtons[buttons];
                Text::arrange(Text::TL, TextStyle(), item.text.c_str());
                w = std::max(16, Text::getSize().x + 8);
                button2->text.set(item.text.c_str());
                button2->setSize(w, gSystem->applyScaleFactor(20));
                button2->onPress.bind(this, &BreakdownWidget::selectStream,
                                      vec2i{item.row, item.endrow});
                total_run = 0;
                add_asterisk = false;
                buttons++;
            } else if (item.text[0] == '-') {
                add_asterisk = true;
            } else {
                if (total_run == 0)
                    run_row = item.row;
                else
                    add_asterisk = true;
                total_run += std::stoi(item.text);
            }
        } else {
            Text::arrange(Text::TL, TextStyle(), item.text.c_str());
            int w = std::max(16, Text::getSize().x + 8);
            if (i >= myButtons.size()) {
                myButtons.emplace_back(new WgButton(getGui()));
            }
            WgButton* button = myButtons[i];
            button->text.set(item.text.c_str());
            button->setSize(w, gSystem->applyScaleFactor(20));
            button->onPress.bind(this, &BreakdownWidget::selectStream,
                                 vec2i{item.row, item.endrow});
        }
    }
    if (asteriskize && !breakdown.empty()) {
        if (buttons >= myButtons.size()) {
            myButtons.emplace_back(new WgButton(getGui()));
        }
        WgButton* button = myButtons[buttons];
        std::string run_text = std::to_string(total_run);
        if (add_asterisk) run_text += "*";
        Text::arrange(Text::TL, TextStyle(), run_text.c_str());
        int w = std::max(16, Text::getSize().x + 8);
        button->text.set(run_text.c_str());
        button->setSize(w, gSystem->applyScaleFactor(20));
        button->onPress.bind(
            this, &BreakdownWidget::selectStream,
            vec2i{run_row, breakdown[breakdown.size() - 1].endrow});
        buttons++;
        while (myButtons.size() > buttons) {
            delete myButtons.back();
            myButtons.pop_back();
        }
    }
    while (myButtons.size() > breakdown.size()) {
        delete myButtons.back();
        myButtons.pop_back();
    }
    double percent_measures = 0.0;
    if (breakdown.size() > 0) {
        percent_measures =
            static_cast<double>(measures) * ROWS_PER_MEASURE /
            (breakdown[breakdown.size() - 1].endrow - breakdown[0].row) * 100.0;
    }
    measureCount->text.set(Str::fmt("Total stream: %1 (%2%)")
                               .arg(measures)
                               .arg(percent_measures, 1, 1)
                               .str);
    measureCount16->text.set(
        Str::fmt("16th note stream: %1").arg(measures16).str);
}

void DialogChartProperties::BreakdownWidget::selectStream(vec2i rows) {
    gView->setCursorRow(rows.x);
    gSelection->selectRegion(rows.x, rows.y);
}

void DialogChartProperties::BreakdownWidget::onUpdateSize() {
    int x = 0;
    int w = gSystem->applyScaleFactor(340);
    int y_spacing = gSystem->applyScaleFactor(20);
    int x_spacing = gSystem->applyScaleFactor(2);
    int y = y_spacing;
    for (auto button : myButtons) {
        vec2i size = button->getSize();
        if (x + size.x > w) {
            x = 0, y += y_spacing + x_spacing;
        }
        x += size.x + x_spacing;
    }
    setSize(w, y);
}

void DialogChartProperties::BreakdownWidget::onArrange(recti r) {
    int x = 0, y = 0;
    int y_spacing = gSystem->applyScaleFactor(20);
    int x_spacing = gSystem->applyScaleFactor(2);
    for (auto button : myButtons) {
        vec2i size = button->getSize();
        if (x + size.x > r.w) {
            x = 0, y += y_spacing + x_spacing;
        }
        button->arrange({r.x + x, r.y + y, size.x, size.y});
        x += size.x + x_spacing;
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

std::string DialogChartProperties::BreakdownWidget::buttonText(bool compressed) { 
    if (myButtons.empty()) return "";
    std::string out = "";
    for (auto& item : myButtons) {
        out = out + item->text.get();
        if (!compressed) out = out + " ";
    }
    if (!compressed) out.pop_back();
    return out;
}

void DialogChartProperties::myCreateBreakdown() {
    myLayout.row().col(340);
    myLayout.add<WgSeperator>();

    myLayout.row().col(24).col(312);

    WgButton* copy = myLayout.add<WgButton>();
    copy->text.set("{g:copy}");
    copy->setTooltip("Copy stream breakdown to clipboard");
    copy->onPress.bind(this, &DialogChartProperties::myCopyBreakdown);

    WgLabel* info = myLayout.add<WgLabel>();
    info->text.set("Stream breakdown");

    myLayout.row().col(312);
    WgCheckbox* small_breakdowns = myLayout.add<WgCheckbox>();
    small_breakdowns->text.set("Compressed breakdown");
    small_breakdowns->setTooltip(
        "Compressed breakdowns are more parseable but have less detail.");
    small_breakdowns->value.bind(&myCompressedBreakdown);
    small_breakdowns->onChange.bind(this,
                                    &DialogChartProperties::myUpdateBreakdown);

    myLayout.row().col(340);
    myBreakdown = new BreakdownWidget(getGui());
    myLayout.add(myBreakdown);
}

void DialogChartProperties::myUpdateBreakdown() {
    myBreakdown->updateBreakdown(myStreamMeasureCount, my16thMeasureCount,
                                 myCompressedBreakdown);
}

void DialogChartProperties::myCopyBreakdown() {
    std::string breakdown = myBreakdown->buttonText(myCompressedBreakdown);
    if (breakdown == "") {
        HudInfo("%s", "There is no breakdown to copy...");
    } else {
        gSystem->setClipboardText(breakdown);
        HudInfo("%s%s", "Stream breakdown copied to clipboard: ", breakdown.c_str());
    }
}

};  // namespace Vortex
