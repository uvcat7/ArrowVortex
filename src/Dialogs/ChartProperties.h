#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <vector>

namespace Vortex {

class DialogChartProperties : public EditorDialog {
   public:
    typedef DialogChartProperties DCP;

    ~DialogChartProperties();
    DialogChartProperties();

    void onChanges(int changes) override;

   private:
    void myCreateChartProperties();
    void mySetStepArtist();
    void mySetDifficulty();
    void mySetRating();
    void myCalcRating();

    void myCreateNoteInfo();
    void myUpdateNoteInfo();
    void myCopyNoteInfo();
    void mySelectNotes(int type);

    void myCreateBreakdown();
    void myUpdateBreakdown();
    void myCopyBreakdown();

    class BreakdownWidget;
    BreakdownWidget* myBreakdown;

    WgButton* myNoteInfo[6];
    WgLabel* myNoteDensity;
    WgLabel* myStreamMeasureCount;
    WgDroplist* myStyleList;
    std::string myStepArtist;

    int myRating, myDifficulty, myStyle;
};

};  // namespace Vortex
