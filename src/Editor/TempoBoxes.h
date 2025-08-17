#pragma once

#include <Simfile/Tempo.h>
#include <Simfile/Segments.h>

namespace Vortex {

struct TempoBox {
    std::string str;
    int row;
    Segment::Type type;
    signed int x : 16;
    uint32_t width : 15;
    uint32_t isSelected : 1;
};

struct TempoBoxes {
    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;

    // Selection functions.
    virtual void deselectAll() = 0;
    virtual int selectAll() = 0;
    virtual int selectType(Segment::Type) = 0;
    virtual int selectSegments(const Tempo* tempo) = 0;
    virtual int selectRows(SelectModifier mod, int begin, int end, int xl,
                           int xr) = 0;
    virtual int selectTime(SelectModifier mod, double begin, double end, int xl,
                           int xr) = 0;
    virtual bool noneSelected() const = 0;

    virtual void toggleShowBoxes() = 0;
    virtual void toggleShowHelp() = 0;

    virtual bool hasShowBoxes() = 0;
    virtual bool hasShowHelp() = 0;

    virtual void tick() = 0;
    virtual void draw() = 0;

    virtual const Vector<TempoBox>& getBoxes() = 0;
};

extern TempoBoxes* gTempoBoxes;

};  // namespace Vortex