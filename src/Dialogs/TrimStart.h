#pragma once

#include <Dialogs/Dialog.h>

namespace Vortex {

class DialogTrimStart : public EditorDialog {
   public:
    void onChanges(int changes) override;
    void onTick() override;

    ~DialogTrimStart();
    DialogTrimStart();

   private:
    void myCreateWidgets();
    void onValueChanged();
    void onTrim();
    void onApply();
    void onRevert();

    /// Time of the earliest note of any chart, in seconds, or a negative
    /// number when no chart has notes.
    double myFirstNoteTime() const;

    /// How much would come off the front with the current settings.
    double myCutLength() const;

    void myUpdateLabels();

    WgSpinner* myTrimSpinner;
    WgCheckbox* myFromNoteBox;
    WgLabel* myFirstNoteLabel;
    WgLabel* myCutLabel;
    WgButton* myTrimButton;
    WgButton* myApplyButton;
    WgButton* myRevertButton;

    double myLeadIn;
    double myFadeIn;
    double myFadeOut;
    double myPadStart;
    double myPadEnd;
    bool myWaitingShown = false;
    bool myTrimEnabled;
    bool myFromFirstNote;
    bool myKeepOriginal;
    bool mySaveAfterwards;
};

};  // namespace Vortex
