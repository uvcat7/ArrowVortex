#pragma once

#include <Core/Input.h>

#include <Managers/ChartMan.h>
#include <Simfile/Segments.h>

namespace Vortex {

struct Editing : public InputHandler {
    enum MirrorType {
        MIRROR_H,
        MIRROR_V,
        MIRROR_HV,
    };

    enum class EditingAnchor {
        /// Target receptor's row
        RECEPTORS,
        /// Target closest snapping row at mouse cursor
        CURSOR,
    };

    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;

    virtual void drawGhostNotes() = 0;

    virtual void deleteSelection() = 0;

    virtual void changeNotesToType(NoteType type) = 0;
    virtual void changeMinesToType(NoteType type) = 0;
    virtual void changeFakesToType(NoteType type) = 0;
    virtual void changeLiftsToType(NoteType type) = 0;
    virtual void changeHoldsToType(NoteType type) = 0;
    virtual void changeHoldsToRolls() = 0;
    virtual void changePlayerNumber() = 0;
    virtual void changeNoteSide() = 0;

    virtual void mirrorNotes(MirrorType type) = 0;
    virtual void scaleNotes(int numerator, int denominator) = 0;

    virtual void insertRows(int row, int numRows, bool curChartOnly) = 0;

    virtual void convertCouplesToRoutine() = 0;
    virtual void convertRoutineToCouples() = 0;

    virtual void exportNotesAsLuaTable() = 0;

    virtual void toggleJumpToNextNote() = 0;
    virtual bool hasJumpToNextNote() = 0;

    virtual void toggleUndoRedoJump() = 0;
    virtual bool hasUndoRedoJump() = 0;

    virtual void toggleTimeBasedCopy() = 0;
    virtual bool hasTimeBasedCopy() = 0;

    virtual void setTempoEditAnchor(EditingAnchor anchor) = 0;
    virtual EditingAnchor getTempoEditAnchor() = 0;
    virtual void openTempoEdit(Segment::Type type) = 0;

    virtual void setVisualSyncAnchor(EditingAnchor anchor) = 0;
    virtual EditingAnchor getVisualSyncAnchor() = 0;
    virtual void injectBoundingBpmChange() = 0;
    virtual void shiftAnchorRowToMousePosition(bool destructive) = 0;

    virtual void requantizeNotes() = 0;
};

extern Editing* gEditing;

};  // namespace Vortex