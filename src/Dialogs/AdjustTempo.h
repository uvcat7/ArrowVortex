#pragma once

#include <Dialogs/Dialog.h>

#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogAdjustTempo : public EditorDialog {
   public:
    ~DialogAdjustTempo();
    DialogAdjustTempo();

    void onChanges(int changes) override;
    void onTick() override;
    void onAction(int id);

   private:
    WgSpinner* myCreateWidgetRow(const std::string& label, int y, double& val,
                                 int action);
    void myCreateWidgets();

    double myBPM, myStop, myOffset, myBeatsToInsert;
    int myInsertTarget;
};

};  // namespace Vortex
