#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/FindTempo.h>

namespace Vortex {

class DialogAdjustSync : public EditorDialog {
   public:
    ~DialogAdjustSync();
    DialogAdjustSync();

    void onChanges(int changes) override;
    void onTick() override;

    void onAction(int id);
    void onApplyBPM();
    void onFindBPM();

   private:
    WgSpinner* myCreateWidgetRow(const std::string&, double&, int, int,
                                 const char*, const char*);
    void myCreateWidgets();

    void myResetBPMDetection();

    int mySelectedResult;
    double myOffset, myInitialBPM;
    WgLabel* myBPMLabel;
    WgButton *myApplyBPM, *myFindBPM;
    WgSelectList* myBPMList;
    TempoDetector* myTempoDetector;
    Vector<TempoResult> myDetectionResults;
    int myDetectionRow;
};

};  // namespace Vortex
