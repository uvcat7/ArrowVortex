#pragma once

#include <Core/Input.h>

#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>

#include <Editor/Common.h>

namespace Vortex {

struct SelectionRegion {
    int beginRow;
    int endRow;

    const bool isEmpty() const { return this->beginRow == this->endRow; }

    const bool rowIsInRegion(int row) const {
        return this->isEmpty() || this->beginRow <= row && row <= this->endRow;
    }
};

struct Selection : public InputHandler {
    enum Type { NONE, REGION, NOTES, TEMPO };

    static void create();
    static void destroy();

    virtual void drawRegionSelection() = 0;
    virtual void drawSelectionBox() = 0;

    // Note selection.
    virtual void selectAllNotes() = 0;
    virtual int selectNotes(NotesMan::Filter filter,
                            bool ignoreRegion = false) = 0;
    virtual int selectNotes(RowType rowType, bool ignoreRegion = false) = 0;
    virtual int selectNotes(SelectModifier t, RowCol begin, RowCol end,
                            bool ignoreRegion = false) = 0;
    virtual int selectNotes(SelectModifier t, const Vector<RowCol>& indices,
                            bool ignoreRegion = false) = 0;
    virtual int getSelectedNotes(NoteList& out) = 0;

    // Region selection.
    virtual void selectRegion() = 0;
    virtual void selectRegion(int row, int endrow) = 0;
    virtual SelectionRegion getSelectedRegion() = 0;
};

extern Selection* gSelection;

};  // namespace Vortex
