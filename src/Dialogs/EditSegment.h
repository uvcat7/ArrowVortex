#pragma once

#include <Dialogs/Dialog.h>

#include <Core/WidgetsLayout.h>

#include <Simfile/Segments.h>

namespace Vortex {

class SegmentEditor {
   public:
    virtual ~SegmentEditor() = default;

    void setRow(int row) { myRow = row; }

    virtual void createWidgets(RowLayout& layout, EditorDialog& dialog) = 0;
    virtual void onTick() = 0;
    virtual void onChange(int id) = 0;
    virtual int getHeight() = 0;
    virtual void deleteSegment() = 0;

   protected:
    int myRow = 0;
};

// ================================================================================================

class BpmChangeEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double myBPM = 120.0;
};

class StopEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double mySeconds = 0.0;
};

class DelayEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double mySeconds = 0.0;
};

class WarpEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double myBeats = 0.0;
};

class TimeSignatureEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    int myRowsPerMeasure = 4;
    int myBeatNote = 4;
};

class TickCountEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    int myTicks = 0;
};

class ComboEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    int myHit = 1;
    int myMiss = 1;
};

class SpeedEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double myRatio = 1.0;
    double myDelay = 0.0;
    int myUnit = 0;
};

class ScrollEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double myRatio = 1.0;
};

class FakeEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    double myBeats = 0.0;
};

class LabelEditor : public SegmentEditor {
   public:
    void createWidgets(RowLayout& layout, EditorDialog& dialog) override;
    void onTick() override;
    void onChange(int id) override;
    int getHeight() override;
    void deleteSegment() override;

   private:
    std::string myText;
};

// ================================================================================================

class DialogEditSegment : public EditorDialog {
   public:
    ~DialogEditSegment();
    DialogEditSegment();

    void setSegment(Segment::Type type, int row);
    int getFixedWidth();
    int getFixedHeight();

    void onChanges(int changes) override;
    void onTick() override;

   private:
    void myCreateWidgets();

    Segment::Type myType;
    std::unique_ptr<SegmentEditor> myEditor;
};

};  // namespace Vortex