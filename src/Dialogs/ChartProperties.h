#pragma once

#include <Dialogs/Dialog.h>

#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogChartProperties : public EditorDialog {
   public:
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

    void myCreateGraph();
    void myUpdateGraph();

    void myCreateBreakdown();
    void myUpdateBreakdown();
    void myCopyBreakdown();

    class GraphWidget;
    GraphWidget* myGraph;

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
