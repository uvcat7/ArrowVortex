#include <Dialogs/AdjustTempo.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Selection.h>
#include <Editor/View.h>
#include <Editor/History.h>
#include <Editor/Common.h>
#include <Editor/Editing.h>

namespace Vortex {

enum Actions {
    ACT_BPM_SET,
    ACT_BPM_HALVE,
    ACT_BPM_DOUBLE,
    ACT_BPM_TWEAK,

    ACT_STOP_SET,
    ACT_STOP_CONVERT_REGION_TO_STOP,
    ACT_STOP_CONVERT_REGION_TO_STUTTER,
    ACT_STOP_TWEAK,

    ACT_INSERT_BEATS,
    ACT_REMOVE_BEATS,
};

DialogAdjustTempo::~DialogAdjustTempo() = default;

DialogAdjustTempo::DialogAdjustTempo() {
    setTitle("ADJUST TEMPO");
    myCreateWidgets();
    onChanges(VCM_ALL_CHANGES);
    myBeatsToInsert = 1;
    myInsertTarget = 0;
}

WgSpinner* DialogAdjustTempo::myCreateWidgetRow(const std::string& label, int y,
                                                double& val, int action) {
    bool isBPM = (action == ACT_BPM_SET);

    const char* tooltips[] = {
        "Halve the current BPM",
        "Double the current BPM",
        "Convert the selected region to a stop",
        "Convert the selected region to a stutter gimmick",
    };
    const char* tooltips2[] = {
        "Stop length at the current beat, in seconds",
        "Music tempo at the current beat, in beats per minute"};

    WgSpinner* spinner = myLayout.add<WgSpinner>(label);
    spinner->value.bind(&val);
    spinner->setPrecision(3, 6);
    spinner->onChange.bind(this, &DialogAdjustTempo::onAction, action + 0);
    spinner->setTooltip(tooltips2[isBPM]);

    WgButton* op1 = myLayout.add<WgButton>();
    op1->text.set(isBPM ? "{g:halve}" : "{g:full selection}");
    op1->onPress.bind(this, &DialogAdjustTempo::onAction, action + 1);
    op1->setTooltip(isBPM ? tooltips[0] : tooltips[2]);

    WgButton* op2 = myLayout.add<WgButton>();
    op2->text.set(isBPM ? "{g:double}" : "{g:half selection}");
    op2->onPress.bind(this, &DialogAdjustTempo::onAction, action + 2);
    op2->setTooltip(isBPM ? tooltips[1] : tooltips[3]);

    WgButton* tweak = myLayout.add<WgButton>();
    tweak->text.set("{g:tweak}");
    tweak->onPress.bind(this, &DialogAdjustTempo::onAction, action + 3);
    tweak->setTooltip(isBPM ? "Tweak the current BPM"
                            : "Tweak the current stop");

    return spinner;
}

void DialogAdjustTempo::myCreateWidgets() {
    myLayout.row().col(38).col(116).col(24).col(24).col(24);

    WgSpinner* bpm = myCreateWidgetRow("BPM", 0, myBPM, ACT_BPM_SET);
    bpm->setRange(VC_MIN_BPM, VC_MAX_BPM);
    bpm->setPrecision(3, 6);
    bpm->setStep(1.0);

    WgSpinner* stop = myCreateWidgetRow("Stop", 28, myStop, ACT_STOP_SET);
    stop->setRange(VC_MIN_STOP, VC_MAX_STOP);
    stop->setPrecision(3, 6);
    stop->setStep(0.001);

    myLayout.row().col(242);
    myLayout.add<WgSeperator>();

    myLayout.row().col(119).col(119);

    WgSpinner* spinner = myLayout.add<WgSpinner>("Offset in beats");
    spinner->setRange(0.0, 100000.0);
    spinner->value.bind(&myBeatsToInsert);
    spinner->setPrecision(3, 6);
    spinner->setTooltip("Number of beats to insert or remove");

    WgCycleButton* cycle = myLayout.add<WgCycleButton>("Apply offset to");
    cycle->addItem("This chart");
    cycle->addItem("All charts");
    cycle->value.bind(&myInsertTarget);
    cycle->setTooltip(
        "Determines which notes and/or tempo changes are targeted");

    WgButton* insert = myLayout.add<WgButton>();
    insert->text.set("Insert beats");
    insert->onPress.bind(this, &DialogAdjustTempo::onAction,
                         static_cast<int>(ACT_INSERT_BEATS));
    insert->setTooltip(
        "Insert the above number of beats at the cursor position\n"
        "All notes and tempo changes after the cursor will be shifted down");

    WgButton* remove = myLayout.add<WgButton>();
    remove->text.set("Delete beats");
    remove->onPress.bind(this, &DialogAdjustTempo::onAction,
                         static_cast<int>(ACT_REMOVE_BEATS));
    remove->setTooltip(
        "Delete the above number of beats at the cursor position\n"
        "All notes and tempo changes after the cursor will be shifted up\n"
        "Notes and tempo changes in the deleted region will be removed");
}

void DialogAdjustTempo::onChanges(int changes) {
    if (changes & VCM_FILE_CHANGED) {
        myBPM = 0.0;
        myStop = 0.0;
        myOffset = 0.0;
        if (gSimfile->isOpen()) {
            for (auto w : myLayout) w->setEnabled(true);
        } else {
            for (auto w : myLayout) w->setEnabled(false);
        }
    }
}

void DialogAdjustTempo::onTick() {
    if (gSimfile->isOpen()) {
        int row = gView->getCursorRow();
        auto segments = gTempo->getSegments();
        myBPM = segments->getRecent<BpmChange>(row).bpm;
        myStop = segments->getRow<Stop>(row).seconds;
    }
    EditorDialog::onTick();
}

void DialogAdjustTempo::onAction(int id) {
    if (gSimfile->isClosed()) return;
    int row = gView->getCursorRow();
    switch (id) {
        case ACT_BPM_SET: {
            if (myBPM != 0.0) {
                gTempo->addSegment(BpmChange(row, myBPM));
            }
        } break;
        case ACT_BPM_HALVE:
        case ACT_BPM_DOUBLE: {
            double bpm =
                gTempo->getBpm(row) * ((id == ACT_BPM_DOUBLE) ? 2 : 0.5);
            gTempo->addSegment(BpmChange(row, bpm));
        } break;
        case ACT_BPM_TWEAK:
            gTempo->startTweakingBpm(row);
            break;
        case ACT_STOP_SET: {
            gTempo->addSegment(Stop(row, myStop));
        } break;
        case ACT_STOP_CONVERT_REGION_TO_STUTTER:
        case ACT_STOP_CONVERT_REGION_TO_STOP: {
            auto region = gSelection->getSelectedRegion();
            double t1 = gTempo->rowToTime(region.beginRow);
            double t2 = gTempo->rowToTime(region.endRow);
            if (t2 > t1) {
                if (id == ACT_STOP_CONVERT_REGION_TO_STOP) {
                    gTempo->addSegment(Stop(region.beginRow, t2 - t1));
                } else if (region.endRow - region.beginRow >= 2) {
                    double halfTime = (t2 - t1) * 0.5;
                    int rows = region.endRow - region.beginRow;

                    SegmentEdit edit;
                    edit.add.append(Stop(region.beginRow, halfTime));

                    double stutterBpm = BeatsPerMin(halfTime / rows);
                    edit.add.append(BpmChange(region.beginRow, stutterBpm));

                    double endBpm = gTempo->getBpm(region.endRow);
                    edit.add.append(BpmChange(region.endRow, endBpm));

                    gTempo->modify(edit);
                }
            }
        } break;
        case ACT_STOP_TWEAK:
            gTempo->startTweakingStop(row);
            break;
        case ACT_INSERT_BEATS:
        case ACT_REMOVE_BEATS:
            int numRows =
                static_cast<int>(ROWS_PER_BEAT * myBeatsToInsert + 0.5);
            if (id == ACT_REMOVE_BEATS) numRows *= -1;
            gEditing->insertRows(gView->getCursorRow(), numRows,
                                 (myInsertTarget == 0));
            break;
    };
}

};  // namespace Vortex
