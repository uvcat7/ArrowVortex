#pragma once

#include <Dialogs/Dialog.h>

namespace Vortex {

class DialogLabelBreakdown : public EditorDialog {
   public:
    void onChanges(int changes) override;
    void onUpdateSize() override;
    void onTick() override;

    void clear();

    void onChange();

    ~DialogLabelBreakdown();
    DialogLabelBreakdown();

   private:
    void myCreateWidgets();
    void mySetDisplayType();
    void myCopyLabels();

    struct LabelButton;
    struct LabelList;
    LabelList* myList;
    std::string myLabelText;
    int myDisplayType;
    WgCycleButton* myDisplayTypeList;
};

};  // namespace Vortex
