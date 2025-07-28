#include <Dialogs/AdjustSync.h>

#include <Core/StringUtils.h>
#include <Core/WidgetsLayout.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>
#include <Simfile/SegmentGroup.h>

#include <Editor/View.h>
#include <Editor/Selection.h>
#include <Editor/Common.h>
#include <Editor/Editing.h>
#include <Editor/History.h>

namespace Vortex {

enum Actions {
    ACT_SET_BPM,
    ACT_SET_OFS,
    ACT_TWEAK_BPM,
    ACT_TWEAK_OFS,

    ACT_INC_OFS_ONE,
    ACT_INC_OFS_HALF,
    ACT_DEC_OFS_HALF,
    ACT_DEC_OFS_ONE,
};

DialogAdjustSync::~DialogAdjustSync() { delete myTempoDetector; }

DialogAdjustSync::DialogAdjustSync()
    : mySelectedResult(0),
      myOffset(0),
      myInitialBPM(0),
      myTempoDetector(nullptr),
      myDetectionRow(0) {
    setTitle("ADJUST SYNC");
    myCreateWidgets();
    onChanges(VCM_ALL_CHANGES);
}

WgSpinner* DialogAdjustSync::myCreateWidgetRow(const std::string& label,
                                               double& val, int setAction,
                                               int tweakAction,
                                               const char* tooltip1,
                                               const char* tooltip2) {
    WgSpinner* spinner = myLayout.add<WgSpinner>(label);
    spinner->value.bind(&val);
    spinner->setPrecision(3, 6);
    spinner->onChange.bind(this, &DialogAdjustSync::onAction, setAction);
    spinner->setTooltip(tooltip1);

    WgButton* tweak = myLayout.add<WgButton>();
    tweak->text.set("{g:tweak}");
    tweak->setTooltip(tooltip2);
    tweak->onPress.bind(this, &DialogAdjustSync::onAction, tweakAction);

    return spinner;
}

void DialogAdjustSync::myCreateWidgets() {
    myLayout.row().col(84).col(124).col(24);

    WgSpinner* offset = myCreateWidgetRow(
        "Music offset", myOffset, ACT_SET_OFS, ACT_TWEAK_OFS,
        "Music start time relative to the first beat, in seconds",
        "Tweak the music offset");
    offset->setRange(-100.0, 100.0);
    offset->setStep(0.001);

    WgSpinner* bpm = myCreateWidgetRow(
        "Initial BPM", myInitialBPM, ACT_SET_BPM, ACT_TWEAK_BPM,
        "Music tempo at the first beat, in beats per minute",
        "Tweak the initial BPM");
    bpm->setRange(VC_MIN_BPM, VC_MAX_BPM);
    bpm->setStep(1.0);

    myLayout.row().col(128).col(24).col(24).col(24).col(24);
    WgLabel* move = myLayout.add<WgLabel>();
    move->text.set("Move first beat");

    const char* moveText[] = {"{g:up one}", "{g:up half}", "{g:down half}",
                              "{g:down one}"};
    const char* moveTooltip[] = {
        "Increase the music offset by one beat",
        "Increase the music offset by half a beat",
        "Decrease the music offset by half a beat",
        "Decrease the music offset by one beat",
    };
    for (int i = 0, x = 132; i < 4; ++i, x += 28) {
        WgButton* button = myLayout.add<WgButton>();
        button->text.set(moveText[i]);
        button->onPress.bind(this, &DialogAdjustSync::onAction,
                             (int)ACT_INC_OFS_ONE + i);
        button->setTooltip(moveTooltip[i]);
    }

    myLayout.row().col(240);
    myLayout.add<WgSeperator>();

    myBPMLabel = myLayout.add<WgLabel>();
    myBPMList = myLayout.addH<WgSelectList>(62);
    myBPMList->value.bind(&mySelectedResult);
    myBPMList->setTooltip("BPM estimates calculated by the editor");

    myLayout.row().col(118).col(118);
    myApplyBPM = myLayout.add<WgButton>();
    myApplyBPM->text.set("Apply BPM");
    myApplyBPM->onPress.bind(this, &DialogAdjustSync::onApplyBPM);
    myApplyBPM->setTooltip("Apply the selected BPM estimate");
    myApplyBPM->setEnabled(false);

    myFindBPM = myLayout.add<WgButton>();
    myFindBPM->text.set("{g:calculate} Find BPM");
    myFindBPM->onPress.bind(this, &DialogAdjustSync::onFindBPM);
    myFindBPM->setTooltip("Estimate the music BPM by analyzing the audio");
}

void DialogAdjustSync::onChanges(int changes) {
    if (changes & VCM_TEMPO_CHANGED) {
        myOffset = gTempo->getOffset();
        myInitialBPM = gTempo->getBpm(0);
    }
    if (changes & VCM_FILE_CHANGED) {
        if (gSimfile->isOpen()) {
            for (auto w : myLayout) w->setEnabled(true);
        } else {
            for (auto w : myLayout) w->setEnabled(false);
        }
        myResetBPMDetection();
    }
}

void DialogAdjustSync::onTick() {
    EditorDialog::onTick();

    if (myTempoDetector) {
        // Update the progress text.
        const char* progress = myTempoDetector->getProgress();
        if (strcmp(myBPMLabel->text.get(), progress) != 0)
            myBPMLabel->text.set(progress);

        // Check if the BPM detector has finished.
        if (myTempoDetector->hasResult()) {
            myDetectionResults = myTempoDetector->getResult();
            double scalar = 0.0;
            for (auto& t : myDetectionResults) {
                scalar += t.fitness;
            }
            for (auto& t : myDetectionResults) {
                t.fitness /= scalar;
                t.offset = -t.offset;
            }
            for (int i = 0; i < myDetectionResults.size(); ++i) {
                auto& t = myDetectionResults[i];

                Str::fmt fmt("#%1 :: %2 BPM :: %3%");
                fmt.arg(i + 1).arg(t.bpm, 2, 2).arg(t.fitness * 100, 0, 0);
                myBPMList->addItem(fmt);
            }
            myApplyBPM->setEnabled(true);
            delete myTempoDetector;
            myTempoDetector = nullptr;
        }
    }
}

void DialogAdjustSync::onAction(int id) {
    switch (id) {
        case ACT_SET_OFS: {
            gTempo->setOffset(myOffset);
        } break;
        case ACT_SET_BPM: {
            if (myInitialBPM != 0.0) {
                gTempo->addSegment(BpmChange(0, myInitialBPM));
            }
        } break;
        case ACT_TWEAK_BPM: {
            gTempo->startTweakingBpm(0);
        } break;
        case ACT_TWEAK_OFS: {
            gTempo->startTweakingOffset();
        } break;
        case ACT_DEC_OFS_HALF:
        case ACT_DEC_OFS_ONE:
        case ACT_INC_OFS_ONE:
        case ACT_INC_OFS_HALF: {
            int rows = ROWS_PER_BEAT;
            if (id == ACT_DEC_OFS_HALF || id == ACT_INC_OFS_HALF) rows /= 2;
            if (id == ACT_DEC_OFS_HALF || id == ACT_DEC_OFS_ONE) rows *= -1;
            double seconds = (double)rows * SecPerRow(gTempo->getBpm(0));
            gTempo->setOffset(gTempo->getOffset() + seconds);
        } break;
    };
}

void DialogAdjustSync::onApplyBPM() {
    if (mySelectedResult >= 0 && mySelectedResult < myDetectionResults.size()) {
        auto& v = myDetectionResults[mySelectedResult];
        if (myDetectionRow == 0) {
            gHistory->startChain();
            gTempo->setOffset(v.offset);
            gTempo->addSegment(BpmChange(0, v.bpm));
            gHistory->finishChain("Changed offset/BPM");
        } else {
            gTempo->addSegment(BpmChange(myDetectionRow, v.bpm));
        }
    }
}

void DialogAdjustSync::onFindBPM() {
    if (!myTempoDetector) {
        myResetBPMDetection();
        auto region = gSelection->getSelectedRegion();
        if (region.beginRow < region.endRow) {
            myDetectionRow = region.beginRow;
            double time = gTempo->rowToTime(region.beginRow);
            double len = gTempo->rowToTime(region.endRow) - time;
            myTempoDetector = TempoDetector::New(time, len);
        } else {
            myDetectionRow = 0;
            myTempoDetector = TempoDetector::New(0, 600.0);
        }
    }
}

void DialogAdjustSync::myResetBPMDetection() {
    if (myTempoDetector) {
        delete myTempoDetector;
        myTempoDetector = nullptr;
    }
    myDetectionResults.clear();
    myBPMLabel->text.set("Automatic BPM Detection");
    myApplyBPM->setEnabled(false);
    myBPMList->clearItems();
    mySelectedResult = 0;
}

};  // namespace Vortex
