#include <Dialogs/EditSegment.h>

#include <Core/StringUtils.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Simfile/SegmentGroup.h>

#include <Editor/View.h>
#include <Editor/Common.h>

namespace Vortex {

// Height needs to be hardcoded due to flicker from the first tick of it
// showing, due to the bounds not being calculated yet and is initially
// positioned wrong.
const int HEIGHT_1_ROW = 24;
const int HEIGHT_2_ROW = 24 + 4 + 24;
const int HEIGHT_3_ROW = 24 + 4 + 24 + 4 + 24;

const int LABEL = 108;
const int FIELD = 132;

// ================================================================================================
// BpmChange.

void BpmChangeEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Tempo");
    spinner->value.bind(&myBPM);
    spinner->setPrecision(3, 6);
    spinner->setStep(0.001);
    spinner->setRange(0.001, 10000);
    spinner->setTooltip("Beats per minute");
    spinner->onChange.bind(this, &BpmChangeEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void BpmChangeEditor::onTick() {
    myBPM = gTempo->getSegments()->getRow<BpmChange>(myRow).bpm;
}

void BpmChangeEditor::onChange() {
    gTempo->addSegment(BpmChange(myRow, myBPM));
}

int BpmChangeEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Stop.

void StopEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Seconds");
    spinner->value.bind(&mySeconds);
    spinner->setPrecision(3, 6);
    spinner->setStep(0.001);
    spinner->setRange(0, 10000);
    spinner->setTooltip("Stop duration in seconds");
    spinner->onChange.bind(this, &StopEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void StopEditor::onTick() {
    mySeconds = gTempo->getSegments()->getRow<Stop>(myRow).seconds;
}

void StopEditor::onChange() { gTempo->addSegment(Stop(myRow, mySeconds)); }

int StopEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Delay.

void DelayEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Seconds");
    spinner->value.bind(&mySeconds);
    spinner->setPrecision(3, 6);
    spinner->setStep(0.001);
    spinner->setRange(0, 10000);
    spinner->setTooltip("Delay duration in seconds");
    spinner->onChange.bind(this, &DelayEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void DelayEditor::onTick() {
    mySeconds = gTempo->getSegments()->getRow<Delay>(myRow).seconds;
}

void DelayEditor::onChange() { gTempo->addSegment(Delay(myRow, mySeconds)); }

int DelayEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Warp.

void WarpEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Beats");
    spinner->value.bind(&myBeats);
    spinner->setPrecision(3, 6);
    spinner->setRange(0, 10000);
    spinner->setTooltip("Warp length in beats");
    spinner->onChange.bind(this, &WarpEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void WarpEditor::onTick() {
    int numRows = gTempo->getSegments()->getRow<Warp>(myRow).numRows;
    myBeats = numRows / static_cast<double>(ROWS_PER_BEAT);
}

void WarpEditor::onChange() {
    int numRows = max(0, static_cast<int>(ROWS_PER_BEAT * myBeats));
    gTempo->addSegment(Warp(myRow, numRows));
}

int WarpEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// TimeSignature.

void TimeSignatureEditor::createWidgets(RowLayout& layout,
                                        EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Upper");
    spinner->value.bind(&myRowsPerMeasure);
    spinner->setPrecision(0, 0);
    spinner->setRange(1, 1000);
    spinner->setTooltip("Beats per measure");
    spinner->onChange.bind(this, &TimeSignatureEditor::onChange);
    dialog.setWidgetId(spinner, "initial");

    spinner = layout.add<WgSpinner>("Lower");
    spinner->value.bind(&myBeatNote);
    spinner->setPrecision(0, 0);
    spinner->setRange(1, 1000);
    spinner->setTooltip("Beat note type");
    spinner->onChange.bind(this, &TimeSignatureEditor::onChange);
}

void TimeSignatureEditor::onTick() {
    auto sig = gTempo->getSegments()->getRecent<TimeSignature>(myRow);
    myRowsPerMeasure = sig.rowsPerMeasure / ROWS_PER_BEAT;
    myBeatNote = sig.beatNote;
}

void TimeSignatureEditor::onChange() {
    int rowsPerMeasure = ROWS_PER_BEAT * max(1, myRowsPerMeasure);
    int beatNote = max(1, myBeatNote);
    gTempo->addSegment(TimeSignature(myRow, rowsPerMeasure, beatNote));
}

int TimeSignatureEditor::getHeight() { return HEIGHT_2_ROW; }

// ================================================================================================
// TickCount.

void TickCountEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Ticks");
    spinner->value.bind(&myTicks);
    spinner->setPrecision(0, 0);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Hold combo ticks per beat");
    spinner->onChange.bind(this, &TickCountEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void TickCountEditor::onTick() {
    myTicks = gTempo->getSegments()->getRecent<TickCount>(myRow).ticks;
}

void TickCountEditor::onChange() {
    int ticks = max(0, myTicks);
    gTempo->addSegment(TickCount(myRow, ticks));
}

int TickCountEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Combo.

void ComboEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Hit Multiplier");
    spinner->value.bind(&myHit);
    spinner->setPrecision(0, 0);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Combo hit multiplier");
    spinner->onChange.bind(this, &ComboEditor::onChange);
    dialog.setWidgetId(spinner, "initial");

    spinner = layout.add<WgSpinner>("Miss Multiplier");
    spinner->value.bind(&myMiss);
    spinner->setPrecision(0, 0);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Combo miss multiplier");
    spinner->onChange.bind(this, &ComboEditor::onChange);
}

void ComboEditor::onTick() {
    auto combo = gTempo->getSegments()->getRecent<Combo>(myRow);
    myHit = combo.hitCombo;
    myMiss = combo.missCombo;
}

void ComboEditor::onChange() {
    int hit = max(1, myHit);
    int miss = max(1, myMiss);
    gTempo->addSegment(Combo(myRow, hit, miss));
}

int ComboEditor::getHeight() { return HEIGHT_2_ROW; }

// ================================================================================================
// Speed.

void SpeedEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Multiplier");
    spinner->value.bind(&myRatio);
    spinner->setPrecision(2, 6);
    spinner->setStep(0.1);
    spinner->setRange(-1000, 1000);
    spinner->setTooltip("Stretch ratio");
    spinner->onChange.bind(this, &SpeedEditor::onChange);
    dialog.setWidgetId(spinner, "initial");

    spinner = layout.add<WgSpinner>("Delay Time");
    spinner->value.bind(&myDelay);
    spinner->setPrecision(2, 6);
    spinner->setStep(0.1);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Delay time");
    spinner->onChange.bind(this, &SpeedEditor::onChange);

    WgCycleButton* cycler = layout.add<WgCycleButton>("Delay Unit");
    cycler->value.bind(&myUnit);
    cycler->setTooltip("Delay unit (beats/time)");
    cycler->addItem("Beats");
    cycler->addItem("Seconds");
    cycler->onChange.bind(this, &SpeedEditor::onChange);
}

void SpeedEditor::onTick() {
    auto speed = gTempo->getSegments()->getRecent<Speed>(myRow);
    myRatio = speed.ratio;
    myDelay = speed.delay;
    myUnit = speed.unit;
}

void SpeedEditor::onChange() {
    double ratio = myRatio;
    double delay = max(0.0, myDelay);
    int unit = clamp(myUnit, 0, 1);
    gTempo->addSegment(Speed(myRow, ratio, delay, unit));
}

int SpeedEditor::getHeight() { return HEIGHT_3_ROW; }

// ================================================================================================
// Scroll.

void ScrollEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Multiplier");
    spinner->value.bind(&myRatio);
    spinner->setPrecision(2, 6);
    spinner->setStep(0.1);
    spinner->setRange(-100000, 100000);
    spinner->setTooltip("Scroll rate multiplier");
    spinner->onChange.bind(this, &ScrollEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void ScrollEditor::onTick() {
    myRatio = gTempo->getSegments()->getRecent<Scroll>(myRow).ratio;
}

void ScrollEditor::onChange() { gTempo->addSegment(Scroll(myRow, myRatio)); }

int ScrollEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Fake.

void FakeEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL).col(FIELD);
    WgSpinner* spinner = layout.add<WgSpinner>("Beats");
    spinner->value.bind(&myBeats);
    spinner->setPrecision(3, 6);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Fake region length in beats");
    spinner->onChange.bind(this, &FakeEditor::onChange);
    dialog.setWidgetId(spinner, "initial");
}

void FakeEditor::onTick() {
    int numRows = gTempo->getSegments()->getRow<Fake>(myRow).numRows;
    myBeats = numRows / static_cast<double>(ROWS_PER_BEAT);
}

void FakeEditor::onChange() {
    int numRows = max(0, static_cast<int>(ROWS_PER_BEAT * myBeats));
    gTempo->addSegment(Fake(myRow, numRows));
}

int FakeEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// Label.

void LabelEditor::createWidgets(RowLayout& layout, EditorDialog& dialog) {
    layout.row().col(LABEL + FIELD);
    WgLineEdit* text = layout.add<WgLineEdit>();
    text->text.bind(&myText);
    text->setMaxLength(1000);
    text->setTooltip("Label text");
    text->onChange.bind(this, &LabelEditor::onChange);
    dialog.setWidgetId(text, "initial");
}

void LabelEditor::onTick() {
    myText = gTempo->getSegments()->getRow<Label>(myRow).str;
}

void LabelEditor::onChange() {
    if (strpbrk(myText.c_str(), ";,=") != nullptr) {
        HudWarning(
            "A Label cannot contain commas, semicolons, or equal signs; "
            "they will be replaced with underscores.");
        Str::replace(myText, ",", "_");
        Str::replace(myText, ";", "_");
        Str::replace(myText, "=", "_");
    }
    gTempo->addSegment(Label(myRow, myText));
}

int LabelEditor::getHeight() { return HEIGHT_1_ROW; }

// ================================================================================================
// DialogEditSegment

static std::unique_ptr<SegmentEditor> createEditor(Segment::Type type) {
    switch (type) {
        case Segment::BPM:
            return std::make_unique<BpmChangeEditor>();
        case Segment::STOP:
            return std::make_unique<StopEditor>();
        case Segment::DELAY:
            return std::make_unique<DelayEditor>();
        case Segment::WARP:
            return std::make_unique<WarpEditor>();
        case Segment::TIME_SIG:
            return std::make_unique<TimeSignatureEditor>();
        case Segment::TICK_COUNT:
            return std::make_unique<TickCountEditor>();
        case Segment::COMBO:
            return std::make_unique<ComboEditor>();
        case Segment::SPEED:
            return std::make_unique<SpeedEditor>();
        case Segment::SCROLL:
            return std::make_unique<ScrollEditor>();
        case Segment::FAKE:
            return std::make_unique<FakeEditor>();
        case Segment::LABEL:
            return std::make_unique<LabelEditor>();
        default:
            return nullptr;
    }
}

static std::string createTitle(Segment::Type type) {
    std::string title = Segment::meta[static_cast<int>(type)]->singular;
    Str::toUpper(title);
    return title;
}

DialogEditSegment::~DialogEditSegment() = default;

DialogEditSegment::DialogEditSegment() : myType(Segment::BPM) {
    setMinimumHeight(HEIGHT_1_ROW);
    setMinimumWidth(getFixedWidth());
    setPinnable(false);
    setMinimizable(false);
    setDraggable(false);
}

void DialogEditSegment::setSegment(Segment::Type type, int row) {
    if (type != myType || !myEditor) {
        myType = type;
        myEditor = createEditor(myType);
        myEditor->setRow(row);
        myCreateWidgets();
        setTitle(createTitle(myType).c_str());
        onChanges(VCM_ALL_CHANGES);
    }
}

int DialogEditSegment::getFixedWidth() { return LABEL + FIELD + 12; }
int DialogEditSegment::getFixedHeight() {
    return myEditor ? myEditor->getHeight() : HEIGHT_1_ROW;
}

void DialogEditSegment::myCreateWidgets() {
    if (!myEditor) return;
    myEditor->createWidgets(myLayout, *this);
}

void DialogEditSegment::onChanges(int changes) {
    if (!myEditor) return;
    if (changes & VCM_FILE_CHANGED) {
        if (gSimfile->isOpen()) {
            for (auto w : myLayout) w->setEnabled(true);
        } else {
            for (auto w : myLayout) w->setEnabled(false);
        }
    }
}

void DialogEditSegment::onTick() {
    if (myEditor && gSimfile->isOpen()) {
        myEditor->onTick();
    }
    EditorDialog::onTick();
}
};  // namespace Vortex