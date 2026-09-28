#pragma once

#include <Dialogs/Dialog.h>

#include <vector>

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
    WgLabel* myPeakDensity;
    WgLabel* myStreamMeasureCount;
    WgLabel* my16thMeasureCount;
    WgDroplist* myStyleList;
    std::string myStepArtist;

    int myRating = 1, myDifficulty = 0, myStyle = 0;
};

class DialogChartProperties::BreakdownWidget : public GuiWidget {
   public:
    ~BreakdownWidget() override;
    explicit BreakdownWidget(GuiContext* gui);

    void updateBreakdown(WgLabel* measureCount, WgLabel* measureCount16);
    void selectStream(vec2i rows);

    void onUpdateSize() override;
    void onArrange(recti r) override;
    void onTick() override;
    void onDraw() override;

   private:
    DialogChartProperties* myDialog;
    std::vector<WgButton*> myButtons;
};

class DialogChartProperties::GraphWidget : public GuiWidget {
   public:
    ~GraphWidget() override;
    explicit GraphWidget(GuiContext* gui);
    void updateGraph();
    void onDraw() override;
    double peakNps();

   private:
    DialogChartProperties* myDialog;
    std::vector<int> data;
    std::vector<Vortex::recti> fill_commands;
    double peak = 0.0;
    int scale = 1;
    int endMeasure = 0;
    double endTime = 0.0;
};

};  // namespace Vortex
