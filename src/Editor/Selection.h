#pragma once

#include <Core/Input.h>

#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>

#include <Editor/Common.h>

#include <vector>

namespace Vortex {

struct SelectionRegion { int beginRow, endRow; };

struct Selection : public InputHandler
{
	enum Type { NONE, REGION, NOTES, TEMPO };

	static void create();
	static void destroy();

	virtual void setType(Type type) = 0;
	virtual Type getType() const = 0;

	virtual void drawRegionSelection() = 0;
	virtual void drawSelectionBox() = 0;

	// Note selection.
	virtual void selectAllNotes() = 0;
	virtual int selectNotes(NotesMan::Filter filter) = 0;
	virtual int selectNotes(RowType rowType) = 0;
	virtual int selectNotes(SelectModifier t, RowCol begin, RowCol end) = 0;
	virtual int selectNotes(SelectModifier t, const std::vector<RowCol>& indices) = 0;
	virtual int getSelectedNotes(NoteList& out) = 0;

	// Region selection.
	virtual void selectRegion() = 0;
	virtual void selectRegion(int row, int endrow) = 0;
	virtual SelectionRegion getSelectedRegion() = 0;
};

extern Selection* gSelection;

}; // namespace Vortex
