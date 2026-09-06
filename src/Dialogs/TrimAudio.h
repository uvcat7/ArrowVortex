#pragma once

#include <Dialogs/Dialog.h>

namespace Vortex {

class DialogTrimAudio : public EditorDialog {
   public:
    void onChanges(int changes) override;
    void onTick() override;

    ~DialogTrimAudio();
    DialogTrimAudio();

   private:
    void myCreateWidgets();
    void onValueChanged();
    void onTrim();
    void onApply();
    void onRevert();

    /// Time of the earliest note of any chart, in seconds, or a negative
    /// number when no chart has notes.
    double myFirstNoteTime() const;

    /// Time the latest note of any chart ends, in seconds, or a negative
    /// number when no chart has notes.
    double myLastNoteTime() const;

    /// How much would come off the front with the current settings.
    double myCutLength() const;

    /// How much would come off the end with the current settings.
    double myCutEndLength() const;

    /// The format the song is in, as its extension without the dot.
    std::string myMusicFormat() const;

    void myUpdateLabels();

    WgSpinner* myTrimSpinner;
    WgSpinner* myTrimEndSpinner;
    WgCheckbox* myFromNoteBox;
    WgCheckbox* myFromLastNoteBox;
    WgLabel* myFirstNoteLabel;
    WgLabel* myLastNoteLabel;
    WgLabel* myFormatLabel;
    WgLabel* myCutLabel;
    WgButton* myTrimButton;
    WgButton* myApplyButton;
    WgButton* myRevertButton;

    double myLeadIn;
    double myTailOut;
    double myFadeIn;
    double myFadeOut;
    double myPadStart;
    double myPadEnd;
    bool myWaitingShown = false;
    bool myTrimEnabled;
    bool myTrimEndEnabled;
    bool myFromFirstNote;
    bool myFromLastNote;
    bool myKeepOriginal;
    bool mySaveAfterwards;
};

};  // namespace Vortex
