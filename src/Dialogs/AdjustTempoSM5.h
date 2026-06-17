#pragma once

#include <Dialogs/Dialog.h>

#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogAdjustTempoSM5 : public EditorDialog {
   public:
    ~DialogAdjustTempoSM5();
    DialogAdjustTempoSM5();

    void clear();

    void onChanges(int changes) override;
    void onTick() override;
    void onAction(int id);

   private:
    WgSpinner* myCreateWidgetRow(const std::string& label, int y, double& val,
                                 int action);
    void myCreateWidgets();

    double myDelay;
    double myWarp;
    int myTimeSigBpm, myTimeSigNote;
    int myTickCount;
    int myComboHit, myComboMiss;
    double mySpeedRatio, mySpeedDelay;
    int mySpeedUnit;
    double myScrollRatio;
    double myFakeBeats;
    std::string myLabelText;
};

};  // namespace Vortex
