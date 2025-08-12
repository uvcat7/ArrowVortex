#pragma once

#include <Simfile/Notes.h>

namespace Vortex {

/// Manages the note data of the active chart.
struct NotesMan {
    static void create();
    static void destroy();

    static const char* clipboardTag;

    /// Determines which notes are selected during a select operation.
    enum Filter {
        SELECT_STEPS,
        SELECT_JUMPS,
        SELECT_MINES,
        SELECT_HOLDS,
        SELECT_ROLLS,
        SELECT_WARPS,
        SELECT_FAKES,
        SELECT_LIFTS,

        NUM_FILTERS
    };

    /// Description of an edit.
    struct EditDescription {
        const char *singular, *plural;
    };

    /// Called by simfile when the active chart or simfile changes.
    virtual void update(Simfile* simfile, Chart* chart) = 0;

    /// Called by tempo when the active tempo changes.
    virtual void updateTempo() = 0;

    // Selection functions.
    virtual void deselectAll() = 0;
    virtual int selectAll() = 0;
    virtual int selectQuant(int rowType) = 0;
    virtual int selectRows(SelectModifier mod, int beginCol, int endCol,
                           int beginRow, int endRow) = 0;
    virtual int selectTime(SelectModifier mod, int beginCol, int endCol,
                           double beginTime, double endTime) = 0;
    virtual int select(SelectModifier mod, const Vector<RowCol>& indices) = 0;
    virtual int select(SelectModifier mod, const Note* notes, int numNotes) = 0;
    virtual int select(SelectModifier mod, Filter filter) = 0;
    virtual bool noneSelected() const = 0;

    // Editing functions.
    virtual void modify(const NoteEdit& edit, bool clearRegion,
                        const EditDescription* desc = nullptr) = 0;
    virtual void removeSelectedNotes() = 0;
    virtual void insertRows(int row, int numRows, bool curChartOnly) = 0;

    // Clipboard functions.
    virtual void copyToClipboard(bool timeBased) = 0;
    virtual void pasteFromClipboard(bool insert) = 0;

    // Statistics functions.
    virtual int getNumSteps() const = 0;  ///< Includes jumps/holds/rolls.
    virtual int getNumJumps() const = 0;
    virtual int getNumMines() const = 0;
    virtual int getNumHolds() const = 0;
    virtual int getNumRolls() const = 0;
    virtual int getNumWarps() const = 0;

    /// Returns a pointer to the first note.
    virtual const ExpandedNote* begin() const = 0;

    /// Returns a pointer to the last note.
    virtual const ExpandedNote* end() const = 0;

    /// Returns true if the number of notes is zero.
    virtual bool empty() const = 0;

    /// Returns a pointer to the note at the given row/column, or null if there
    /// is none.
    virtual const ExpandedNote* getNoteAt(int row, int col) const = 0;

    /// Returns a pointer to the note that contains the given row/column, or
    /// null if there is none.
    virtual const ExpandedNote* getNoteIntersecting(int row, int col) const = 0;

    /// Returns the indices of all notes preceding the given time for each
    /// column.
    virtual Vector<const ExpandedNote*> getNotesBeforeTime(
        double time) const = 0;
};

extern NotesMan* gNotes;

};  // namespace Vortex
