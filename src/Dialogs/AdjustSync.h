#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/FindTempo.h>

#include <vector>

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

    int mySelectedResult = 0;
    double myOffset = 0.0, myInitialBPM = 0.0;
    WgLabel* myBPMLabel;
    WgButton *myApplyBPM, *myFindBPM;
    WgSelectList* myBPMList;
    TempoDetector* myTempoDetector = nullptr;
    std::vector<TempoResult> myDetectionResults;
    int myDetectionRow = 0;
};

};  // namespace Vortex
