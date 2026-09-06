#include <Dialogs/TrimAudio.h>

#include <float.h>

#include <algorithm>

#include <Core/Draw.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>
#include <Core/WidgetsLayout.h>

#include <Managers/SimfileMan.h>
#include <Managers/TempoMan.h>

#include <Simfile/Chart.h>
#include <Simfile/Simfile.h>

#include <Editor/Common.h>
#include <Editor/Music.h>

namespace Vortex {

DialogTrimAudio::~DialogTrimAudio() {
    // A result nobody applied is not left lying around.
    if (gMusic->hasTrimPreview()) gMusic->cancelTrimPreview();
}

DialogTrimAudio::DialogTrimAudio() {
    myLeadIn = 2.0;
    myTailOut = 2.0;
    myFadeIn = 0.0;
    myFadeOut = 0.0;
    myPadStart = 0.0;
    myPadEnd = 0.0;
    // Cutting is the one thing here that throws audio away, so it stays
    // off until it is asked for.
    myTrimEnabled = false;
    myTrimEndEnabled = false;
    myFromFirstNote = true;
    myFromLastNote = true;
    // Trimming cannot be undone, so the original file is kept unless the
    // user says otherwise.
    myKeepOriginal = true;
    mySaveAfterwards = true;

    setTitle("TRIM AUDIO");
    myCreateWidgets();
    myUpdateLabels();
}

void DialogTrimAudio::myCreateWidgets() {
    myLayout.row().col(196);
    myFirstNoteLabel = myLayout.add<WgLabel>();

    myLayout.row().col(196);

    WgCheckbox* doTrim = myLayout.add<WgCheckbox>();
    doTrim->text.set("Trim the start");
    doTrim->value.bind(&myTrimEnabled);
    doTrim->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    doTrim->setTooltip(
        "Cut audio off the front of the song. Off, the two fields below "
        "do nothing and only the fades and the silence apply");

    myLayout.row().col(120).col(76);

    WgSpinner* lead = myLayout.add<WgSpinner>("Trim");
    myTrimSpinner = lead;
    lead->value.bind(&myLeadIn);
    lead->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    lead->setRange(0.0, 60.0);
    lead->setStep(0.1);
    lead->setPrecision(3, 3);
    lead->setTooltip(
        "With the box below ticked, how much silence to leave in front of "
        "the first note; without it, how many seconds come off the start "
        "of the song. This one removes audio - to add silence, use the "
        "fields below");

    myLayout.row().col(196);

    WgCheckbox* fromNote = myLayout.add<WgCheckbox>();
    myFromNoteBox = fromNote;
    fromNote->text.set("Measure from the first note");
    fromNote->value.bind(&myFromFirstNote);
    fromNote->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    fromNote->setTooltip(
        "On: the value above is the silence left in front of the first "
        "note. Off: it is how much comes off the start of the song");

    myLayout.row().col(196);
    myLastNoteLabel = myLayout.add<WgLabel>();

    WgCheckbox* doTrimEnd = myLayout.add<WgCheckbox>();
    doTrimEnd->text.set("Trim the end");
    doTrimEnd->value.bind(&myTrimEndEnabled);
    doTrimEnd->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    doTrimEnd->setTooltip(
        "Cut audio off the end of the song, which is how a stretch of it is "
        "taken out to be charted on its own");

    myLayout.row().col(120).col(76);

    WgSpinner* tail = myLayout.add<WgSpinner>("Trim");
    myTrimEndSpinner = tail;
    tail->value.bind(&myTailOut);
    tail->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    tail->setRange(0.0, 600.0);
    tail->setStep(0.1);
    tail->setPrecision(3, 3);
    tail->setTooltip(
        "With the box below ticked, how much song to leave after the last "
        "note; without it, how many seconds come off the end");

    myLayout.row().col(196);

    WgCheckbox* fromLast = myLayout.add<WgCheckbox>();
    myFromLastNoteBox = fromLast;
    fromLast->text.set("Measure from the last note");
    fromLast->value.bind(&myFromLastNote);
    fromLast->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    fromLast->setTooltip(
        "On: the value above is how much song to leave after the last note. "
        "Off: it is how much comes off the end of the song");

    myLayout.add<WgSeperator>();

    myLayout.row().col(120).col(76);

    WgSpinner* fade = myLayout.add<WgSpinner>("Fade in");
    fade->value.bind(&myFadeIn);
    fade->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    fade->setRange(0.0, 30.0);
    fade->setStep(0.1);
    fade->setPrecision(3, 3);
    fade->setTooltip(
        "Length of the fade that eases the song in after the cut, in seconds; "
        "zero turns the fade off");

    WgSpinner* fadeOut = myLayout.add<WgSpinner>("Fade out");
    fadeOut->value.bind(&myFadeOut);
    fadeOut->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    fadeOut->setRange(0.0, 30.0);
    fadeOut->setStep(0.1);
    fadeOut->setPrecision(3, 3);
    fadeOut->setTooltip(
        "Length of the fade that takes the song out, in seconds. It lands on "
        "the end the song is left with, so cutting the end moves the fade "
        "with it; zero turns the fade off");

    myLayout.row().col(196);
    myLayout.add<WgSeperator>();

    myLayout.row().col(120).col(76);

    WgSpinner* padStart = myLayout.add<WgSpinner>("Silence at start");
    padStart->value.bind(&myPadStart);
    padStart->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    padStart->setRange(0.0, 5.0);
    padStart->setStep(0.1);
    padStart->setPrecision(3, 3);
    padStart->setTooltip(
        "Silence to add in front of the song, in seconds; the chart moves "
        "with it, so the notes stay on the music");

    WgSpinner* padEnd = myLayout.add<WgSpinner>("Silence at end");
    padEnd->value.bind(&myPadEnd);
    padEnd->onChange.bind(this, &DialogTrimAudio::onValueChanged);
    padEnd->setRange(0.0, 5.0);
    padEnd->setStep(0.1);
    padEnd->setPrecision(3, 3);
    padEnd->setTooltip(
        "Silence to add after the song, in seconds, for charts that would "
        "otherwise stop dead on the last note");

    myLayout.row().col(196);
    myLayout.add<WgSeperator>();

    WgCheckbox* keep = myLayout.add<WgCheckbox>();
    keep->text.set("Keep the original file");
    keep->value.bind(&myKeepOriginal);
    keep->setTooltip(
        "When enabled the trimmed song is written next to the original as a "
        "second file, instead of taking its place");

    WgCheckbox* save = myLayout.add<WgCheckbox>();
    save->text.set("Save the simfile afterwards");
    save->value.bind(&mySaveAfterwards);
    save->setTooltip(
        "The audio changes on disk right away; saving keeps the offset in the "
        "simfile in step with it");

    myFormatLabel = myLayout.add<WgLabel>();

    myCutLabel = myLayout.add<WgLabel>();

    myTrimButton = myLayout.add<WgButton>();
    myTrimButton->text.set("Preview");
    myTrimButton->onPress.bind(this, &DialogTrimAudio::onTrim);
    myTrimButton->setTooltip(
        "Work out the new audio and play it, without touching anything on "
        "disk yet");

    myLayout.row().col(196);

    myApplyButton = myLayout.add<WgButton>();
    myApplyButton->text.set("Apply");
    myApplyButton->onPress.bind(this, &DialogTrimAudio::onApply);
    myApplyButton->setTooltip(
        "Keep what you are hearing: the audio takes its place on disk and "
        "the timing stays with it");

    myRevertButton = myLayout.add<WgButton>();
    myRevertButton->text.set("Revert");
    myRevertButton->onPress.bind(this, &DialogTrimAudio::onRevert);
    myRevertButton->setTooltip(
        "Throw the result away and put the song back as it was");
}

double DialogTrimAudio::myFirstNoteTime() const {
    if (gSimfile->isClosed()) return -1.0;

    int firstRow = INT_MAX;
    for (int i = 0; i < gSimfile->getNumCharts(); ++i) {
        auto chart = gSimfile->getChart(i);
        if (chart->notes.empty()) continue;
        firstRow = std::min(firstRow, chart->notes.begin()->row);
    }
    if (firstRow == INT_MAX) return -1.0;

    return gTempo->rowToTime(firstRow);
}

double DialogTrimAudio::myLastNoteTime() const {
    if (gSimfile->isClosed()) return -1.0;

    int lastRow = INT_MIN;
    for (int i = 0; i < gSimfile->getNumCharts(); ++i) {
        auto chart = gSimfile->getChart(i);
        if (chart->notes.empty()) continue;
        const Note& note = *(chart->notes.end() - 1);
        lastRow = std::max(lastRow, std::max(note.row, note.endrow));
    }
    if (lastRow == INT_MIN) return -1.0;

    return gTempo->rowToTime(lastRow);
}

std::string DialogTrimAudio::myMusicFormat() const {
    if (gSimfile->isClosed()) return std::string();

    auto sim = gSimfile->get();
    if (!sim) return std::string();

    size_t dot = sim->music.rfind('.');
    if (dot == std::string::npos) return std::string();

    std::string ext = sim->music.substr(dot + 1);
    Str::toLower(ext);
    return ext;
}

double DialogTrimAudio::myCutLength() const {
    if (!myTrimEnabled) return 0.0;

    double first = myFirstNoteTime();
    if (myFromFirstNote) {
        // Leave the asked-for silence in front of the first note.
        if (first < 0.0) return 0.0;
        return std::max(0.0, first - myLeadIn);
    }

    // Take the asked-for seconds off the front, but never so many that
    // the first note would end up before the song starts.
    double cut = std::max(0.0, myLeadIn);
    if (first >= 0.0) cut = std::min(cut, first);
    return cut;
}

double DialogTrimAudio::myCutEndLength() const {
    if (!myTrimEndEnabled) return 0.0;

    const double length = gMusic->getSongLength();
    if (length <= 0.0) return 0.0;

    if (myFromLastNote) {
        // Leave the asked-for stretch of song after the last note.
        double last = myLastNoteTime();
        if (last < 0.0) return 0.0;
        return std::max(0.0, length - (last + myTailOut));
    }

    // Take the asked-for seconds off the end, but never the whole song.
    return std::clamp(myTailOut, 0.0, length);
}

void DialogTrimAudio::myUpdateLabels() {
    double first = myFirstNoteTime();
    if (first < 0.0) {
        myFirstNoteLabel->text.set("No chart has notes");
    } else {
        myFirstNoteLabel->text.set(
            Str::fmt("First note at %1 s").arg(first, 3, 3).str);
    }

    double last = myLastNoteTime();
    if (last < 0.0) {
        myLastNoteLabel->text.set("");
    } else {
        myLastNoteLabel->text.set(
            Str::fmt("Last note at %1 s").arg(last, 3, 3).str);
    }

    // Trimming writes the song again, and what that costs depends on the
    // format it is in.
    std::string format = myMusicFormat();
    if (format.empty()) {
        myFormatLabel->text.set("");
    } else if (format == "wav") {
        myFormatLabel->text.set("The wav is rewritten");
        myFormatLabel->setTooltip(
            "wav holds the samples as they are, so writing the song again "
            "costs nothing in quality");
    } else {
        myFormatLabel->text.set(
            Str::fmt("The %1 is re-encoded").arg(format).str);
        myFormatLabel->setTooltip(
            "Trimming decodes the song and encodes it again. A lossy format "
            "loses a little quality every time it is written, so keep the "
            "original file if you may want to trim again");
    }

    double cut = myCutLength() + myCutEndLength();
    bool pads = myPadStart > 0.0 || myPadEnd > 0.0;
    if (cut > 0.0) {
        myCutLabel->text.set(Str::fmt("Cuts %1 s").arg(cut, 3, 3).str);
    } else if (pads) {
        myCutLabel->text.set(
            Str::fmt("Adds %1 s").arg(myPadStart + myPadEnd, 3, 3).str);
    } else if (myFadeIn > 0.0 || myFadeOut > 0.0) {
        myCutLabel->text.set("Fade only");
    } else {
        myCutLabel->text.set("Nothing to cut");
    }
    // The trim fields only mean something while their end is being cut.
    myTrimSpinner->setEnabled(myTrimEnabled);
    myFromNoteBox->setEnabled(myTrimEnabled);
    myTrimEndSpinner->setEnabled(myTrimEndEnabled);
    myFromLastNoteBox->setEnabled(myTrimEndEnabled);

    const bool waiting = gMusic->hasTrimPreview();
    if (waiting) myCutLabel->text.set("Listening to the result");

    myTrimButton->setEnabled(
        !waiting && (cut > 0.0 || pads || myFadeIn > 0.0 || myFadeOut > 0.0));
    myApplyButton->setEnabled(waiting);
    myRevertButton->setEnabled(waiting);
}

void DialogTrimAudio::onValueChanged() { myUpdateLabels(); }

void DialogTrimAudio::onApply() {
    gMusic->applyTrimPreview();
    myUpdateLabels();
}

void DialogTrimAudio::onRevert() {
    gMusic->cancelTrimPreview();
    myUpdateLabels();
}

void DialogTrimAudio::onChanges(int changes) {
    if (changes & (VCM_CHART_CHANGED | VCM_NOTES_CHANGED | VCM_TEMPO_CHANGED |
                   VCM_FILE_CHANGED)) {
        myUpdateLabels();
    }
}

void DialogTrimAudio::onTick() {
    // The result arrives from a background thread, so the buttons have to
    // notice it on their own.
    if (gMusic->hasTrimPreview() != myWaitingShown) {
        myWaitingShown = gMusic->hasTrimPreview();
        myUpdateLabels();
    }
    EditorDialog::onTick();
}

void DialogTrimAudio::onTrim() {
    double cut = myCutLength();
    double cutEnd = myCutEndLength();
    if (cut <= 0.0 && cutEnd <= 0.0 && myFadeIn <= 0.0 && myFadeOut <= 0.0 &&
        myPadStart <= 0.0 && myPadEnd <= 0.0) {
        return;
    }

    gMusic->startAudioTrim(cut, cutEnd, myFadeIn, myFadeOut, myPadStart,
                           myPadEnd, myKeepOriginal, mySaveAfterwards);
}

};  // namespace Vortex
