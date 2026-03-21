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
const int HEIGHT_1_ROW = 24 * 2 + 4 * 1;
const int HEIGHT_2_ROW = 24 * 3 + 4 * 2;
const int HEIGHT_3_ROW = 24 * 4 + 4 * 3;

const int LABEL = 108;
const int FIELD = 132;
const int FULL = 244;

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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &BpmChangeEditor::deleteSegment);
}

void BpmChangeEditor::onTick() {
    myBPM = gTempo->getSegments()->getRecent<BpmChange>(myRow).bpm;
}

void BpmChangeEditor::onChange() {
    gTempo->addSegment(BpmChange(myRow, myBPM));
}

void BpmChangeEditor::deleteSegment() {
    gTempo->removeSegment(Segment::BPM, myRow);
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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &StopEditor::deleteSegment);
}

void StopEditor::onTick() {
    mySeconds = gTempo->getSegments()->getRow<Stop>(myRow).seconds;
}

void StopEditor::onChange() { gTempo->addSegment(Stop(myRow, mySeconds)); }

void StopEditor::deleteSegment() {
    gTempo->removeSegment(Segment::STOP, myRow);
}

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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &DelayEditor::deleteSegment);
}

void DelayEditor::onTick() {
    mySeconds = gTempo->getSegments()->getRow<Delay>(myRow).seconds;
}

void DelayEditor::onChange() { gTempo->addSegment(Delay(myRow, mySeconds)); }

void DelayEditor::deleteSegment() {
    gTempo->removeSegment(Segment::DELAY, myRow);
}

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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &WarpEditor::deleteSegment);
}

void WarpEditor::onTick() {
    int numRows = gTempo->getSegments()->getRow<Warp>(myRow).numRows;
    myBeats = numRows / static_cast<double>(ROWS_PER_BEAT);
}

void WarpEditor::onChange() {
    int numRows = max(0, static_cast<int>(ROWS_PER_BEAT * myBeats));
    gTempo->addSegment(Warp(myRow, numRows));
}

void WarpEditor::deleteSegment() {
    gTempo->removeSegment(Segment::WARP, myRow);
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

    spinner = layout.add<WgSpinner>("Lower");
    spinner->value.bind(&myBeatNote);
    spinner->setPrecision(0, 0);
    spinner->setRange(1, 1000);
    spinner->setTooltip("Beat note type");
    spinner->onChange.bind(this, &TimeSignatureEditor::onChange);

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &TimeSignatureEditor::deleteSegment);
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

void TimeSignatureEditor::deleteSegment() {
    gTempo->removeSegment(Segment::TIME_SIG, myRow);
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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &TickCountEditor::deleteSegment);
}

void TickCountEditor::onTick() {
    myTicks = gTempo->getSegments()->getRecent<TickCount>(myRow).ticks;
}

void TickCountEditor::onChange() {
    int ticks = max(0, myTicks);
    gTempo->addSegment(TickCount(myRow, ticks));
}

void TickCountEditor::deleteSegment() {
    gTempo->removeSegment(Segment::TICK_COUNT, myRow);
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

    spinner = layout.add<WgSpinner>("Miss Multiplier");
    spinner->value.bind(&myMiss);
    spinner->setPrecision(0, 0);
    spinner->setRange(0, 1000);
    spinner->setTooltip("Combo miss multiplier");
    spinner->onChange.bind(this, &ComboEditor::onChange);

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &ComboEditor::deleteSegment);
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

void ComboEditor::deleteSegment() {
    gTempo->removeSegment(Segment::COMBO, myRow);
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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &SpeedEditor::deleteSegment);
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

void SpeedEditor::deleteSegment() {
    gTempo->removeSegment(Segment::SPEED, myRow);
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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &ScrollEditor::deleteSegment);
}

void ScrollEditor::onTick() {
    myRatio = gTempo->getSegments()->getRecent<Scroll>(myRow).ratio;
}

void ScrollEditor::onChange() { gTempo->addSegment(Scroll(myRow, myRatio)); }

void ScrollEditor::deleteSegment() {
    gTempo->removeSegment(Segment::SCROLL, myRow);
}

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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &FakeEditor::deleteSegment);
}

void FakeEditor::onTick() {
    int numRows = gTempo->getSegments()->getRow<Fake>(myRow).numRows;
    myBeats = numRows / static_cast<double>(ROWS_PER_BEAT);
}

void FakeEditor::onChange() {
    int numRows = max(0, static_cast<int>(ROWS_PER_BEAT * myBeats));
    gTempo->addSegment(Fake(myRow, numRows));
}

void FakeEditor::deleteSegment() {
    gTempo->removeSegment(Segment::FAKE, myRow);
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

    layout.row().col(FULL);
    layout.add<WgSeperator>();
    WgButton* button = layout.add<WgButton>();
    button->text.set("Delete");
    button->onPress.bind(this, &LabelEditor::deleteSegment);
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

void LabelEditor::deleteSegment() {
    gTempo->removeSegment(Segment::LABEL, myRow);
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
    setMinimumWidth(FULL);
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

int DialogEditSegment::getFixedWidth() { return FULL + 8; }
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