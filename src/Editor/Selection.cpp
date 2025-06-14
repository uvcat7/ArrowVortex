#include <Editor/Selection.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Draw.h>

#include <System/System.h>

#include <Editor/View.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/TempoBoxes.h>

#include <Managers/NoteMan.h>
#include <Managers/TempoMan.h>
#include <Managers/StyleMan.h>

#include <vector>

namespace Vortex {

// ================================================================================================
// SelectionImpl :: member data.

struct SelectionImpl : public Selection {

Texture myAddIcon, mySubIcon;

int myDragSelectionX;
double myDragSelectionTor;
bool myIsDraggingSelection;

SelectionRegion myRegion;
bool myIsSelectingRegion;
Type myType;

// ================================================================================================
// SelectionImpl :: constructor and destructor.

~SelectionImpl()
{
}

SelectionImpl()
{
	myDragSelectionX = 0;
	myDragSelectionTor = 0;
	myIsDraggingSelection = false;
	myRegion = {0, 0};
	myIsSelectingRegion = false;

	Texture icons[2];
	Texture::createTiles("assets/icons selection.png", 16, 16, 2, icons, false, Texture::RGBA);
	myAddIcon = icons[0];
	mySubIcon = icons[1];

	myType = NONE;
}

// ================================================================================================
// SelectionImpl :: event handling.

void onMousePress(MousePress& evt) override
{
	// Start dragging a selection box.
	if(evt.button == Mouse::LMB && evt.unhandled())
	{
		myIsDraggingSelection = true;
		myDragSelectionX = evt.x;
		myDragSelectionTor = gView->yToOffset(evt.y);
		evt.setHandled();
	}

	// Clear selection.
	if(myType != NONE && evt.button == Mouse::RMB && evt.unhandled())
	{
		setType(NONE);
		evt.setHandled();
	}
}

void onMouseRelease(MouseRelease& evt) override
{
	// Finish dragging a selection box (note selection).
	if(evt.button == Mouse::LMB && myIsDraggingSelection)
	{
		// Determine the selection boolean operation.
		SelectModifier mod = SELECT_SET;
		if(evt.keyflags & Keyflag::SHIFT) mod = SELECT_ADD;
		else if(evt.keyflags & Keyflag::ALT) mod = SELECT_SUB;

		// Determine the selected area.
		double t = myDragSelectionTor;
		double b = gView->yToOffset(evt.y);
		int l = evt.x;
		int r = myDragSelectionX;

		if(t > b) swapValues(t, b);
		if(l > r) swapValues(l, r);

		if(myType == TEMPO && mod != SELECT_SET)
		{
			selectTempoBoxes(mod, t, b, l, r);
		}
		else if(myType == NOTES && mod != SELECT_SET)
		{
			selectNotes(mod, t, b, l, r);
		}
		else
		{
			bool done = selectNotes(mod, t, b, l, r);
			if(!done) selectTempoBoxes(mod, t, b, l, r);
		}
			
		myIsDraggingSelection = false;
	}
}

void onKeyPress(KeyPress& evt) override
{
	if(evt.key == Key::A && (evt.keyflags & Keyflag::CTRL) && !evt.handled)
	{
		if(gChart->isOpen())
		{
			gNotes->selectAll();
		}
		else
		{
			gTempoBoxes->selectAll();
		}
		evt.handled = true;
	}
}

// ================================================================================================
// SelectionImpl :: member functions.

static bool SegmentsIntersect(int l1, int r1, int l2, int r2)
{
	return r1 >= l2 && r2 >= l1;
}

void selectTempoBoxes(SelectModifier mod, double t, double b, int l, int r)
{
	if(gView->getScaleLevel() >= 2)
	{
		if(gView->isTimeBased())
		{
			gTempoBoxes->selectTime(mod, t, b, l, r);
		}
		else
		{
			gTempoBoxes->selectRows(mod, (int)(t + 0.5), (int)(b + 0.5), l, r);
		}
	}
}

bool selectNotes(SelectModifier mod, double torT, double torB, int xl, int xr)
{
	// If the selection rectangle is empty, we will select a single note under the mouse.
	bool timeBased = gView->isTimeBased();
	if(xl == xr && torT == torB)
	{
		double clickY = gView->offsetToY(torT);
		const ExpandedNote* closest = nullptr;
		int mindist = gView->applyZoom(32);
		mindist *= mindist;

		for(auto& note : *gNotes)
		{
			double tor = timeBased ? note.time : (double)note.row;
			int dx = xl - gView->columnToX(note.col);
			int dy = (int)(clickY - gView->offsetToY(tor));
			if(abs(dy) < mindist)
			{
				int sqrdist = dx * dx + dy * dy;
				if(sqrdist < mindist)
				{
					mindist = sqrdist;
					closest = &note;
				}
			}
		}
		if(closest)
		{
			selectNotes(mod, std::vector<RowCol>(1, {closest->row, closest->col}));
			return true;
		}
		else
		{
			selectNotes(mod, {0, 0}, {0, 0});
		}
	}
	else // The selection rectangle is non-empty.
	{
		RowCol begin{0, 0}, end{0, 0};

		// Determine the columns that fall within the selection.
		int cols = gStyle->getNumCols();
		for(; begin.col < cols && gView->columnToX(begin.col) < xl; ++begin.col);
		for(end.col = begin.col; end.col < cols && gView->columnToX(end.col) < xr; ++end.col);

		// Determine the rows that fall within the selection.
		if(timeBased)
		{
			begin.row = gTempo->timeToRow(torT);
			end.row = gTempo->timeToRow(torB);
		}
		else
		{
			begin.row = (int)torT;
			end.row = (int)torB + 1;
		}
		return selectNotes(mod, begin, end) > 0;
	}
	return false;
}

void setType(Type type)
{
	myType = type;

	// Region selection.
	if(myType == REGION)
	{
		if(myRegion.beginRow == myRegion.endRow)
		{
			myRegion = {0, 0};
			myType = NONE;
		}
	}
	else
	{
		myRegion = {0, 0};
	}

	// Note selection.
	if(myType == NOTES)
	{
		if(gNotes->noneSelected())
		{
			myType = NONE;
		}
	}
	else
	{
		gNotes->deselectAll();
	}

	// Tempo box selection.
	if(myType == TEMPO)
	{
		if(gTempoBoxes->noneSelected())
		{
			myType = NONE;
		}
	}
	else
	{
		gTempoBoxes->deselectAll();
	}

	gEditor->reportChanges(VCM_SELECTION_CHANGED);
}

Type getType() const
{
	return myType;
}

void drawRegionSelection()
{
	// Draw area selection box.
	if(myIsSelectingRegion || myRegion.beginRow != myRegion.endRow)
	{
		color32 outline = RGBAtoColor32(153, 255, 153, 153);
		auto coords = gView->getReceptorCoords();
		int x = coords.xl, w = coords.xr - coords.xl;
		if(myIsSelectingRegion)
		{
			int y = gView->rowToY(myRegion.beginRow);
			Draw::fill({x, y - 1, w, 2}, outline);
		}
		else if(myRegion.beginRow != myRegion.endRow)
		{
			int t = gView->rowToY(myRegion.beginRow);
			int b = gView->rowToY(myRegion.endRow);
			Draw::fill({x, t, w, b - t}, RGBAtoColor32(153, 255, 153, 90));
			Draw::outline({x, t, w, b - t}, outline);
		}
	}
}

void drawSelectionBox()
{
	// Draw dragging selection box (for note/tempo selection).
	if(myIsDraggingSelection)
	{
		vec2i mpos = gSystem->getMousePos();
		vec2i start = {myDragSelectionX, gView->offsetToY(myDragSelectionTor)};

		// Selection rectangle.
		color32 outline = RGBAtoColor32(255, 191, 128, 128);
		color32 fill = RGBAtoColor32(255, 191, 128, 89);

		int x = start.x, x2 = mpos.x;
		int y = start.y, y2 = mpos.y;
		if(x > x2) swapValues(x, x2);
		if(y > y2) swapValues(y, y2);

		Draw::fill({x, y, x2 - x, y2 - y}, fill);
		Draw::outline({x, y, x2 - x, y2 - y}, outline);

		// Hotkey tips for subtractive/additive selection.
		int keyFlags = gSystem->getKeyFlags();
		if(keyFlags & (Keyflag::SHIFT | Keyflag::ALT))
		{
			Texture& tex = (keyFlags & Keyflag::SHIFT) ? myAddIcon : mySubIcon;
			Draw::sprite(tex, {start.x, start.y});
		}
	}
}

// ================================================================================================
// Note selection.

static void showSelectionResult(SelectModifier mod, int numSelected, const char* noteName = nullptr)
{
	if(numSelected == 0 && noteName)
	{
		const char* typeName = (mod == SELECT_SUB ? "deselect" : "select");
		HudNote("There are no %ss to %s.", noteName, typeName);
	}
	else if(numSelected >= 1)
	{
		if(!noteName) noteName = "note";

		const char* typeName = (mod == SELECT_SUB ? "Deselected" : "Selected");

		if(numSelected == 1)
		{
			HudNote("%s 1 %s.", typeName, noteName);
		}
		else
		{
			HudNote("%s %i %ss.", typeName, numSelected, noteName);
		}
	}
}

void selectAllNotes()
{
	showSelectionResult(SELECT_SET, gNotes->selectAll());
}

int selectNotes(NotesMan::Filter filter)
{
	const char* names[NotesMan::NUM_FILTERS] =
	{
		"step", "jump note", "mine", "hold", "roll", "warped note"
	};
	int numSelected = gNotes->select(SELECT_SET, filter);
	showSelectionResult(SELECT_SET, numSelected, names[filter]);
	return numSelected;
}

int selectNotes(RowType rowType)
{
	int numSelected = gNotes->selectQuant(rowType);
	showSelectionResult(SELECT_SET, numSelected);
	return numSelected;
}

int selectNotes(SelectModifier mod, RowCol begin, RowCol end)
{
	int numSelected = gNotes->selectRows(mod, begin.col, end.col, begin.row, end.row);
	showSelectionResult(mod, numSelected);
	return numSelected;
}

int selectNotes(SelectModifier mod, const std::vector<RowCol>& indices)
{
	int numSelected = gNotes->select(mod, indices);
	showSelectionResult(mod, numSelected);
	return numSelected;
}

int getSelectedNotes(NoteList& out)
{
	if(myRegion.beginRow == myRegion.endRow)
	{
		for(auto& note : *gNotes)
		{
			if(note.isSelected) out.append(CompressNote(note));
		}
	}
	else
	{
		auto note = gNotes->begin(), end = gNotes->end();
		for(; note != end && note->row < myRegion.beginRow; ++note);
		for(; note != end && note->row <= myRegion.endRow; ++note)
		{
			out.append(CompressNote(*note));
		}
	}
	return out.size();
}

// ================================================================================================
// Region selection.

void selectRegion()
{
	if(!myIsSelectingRegion)
	{
		gTempoBoxes->deselectAll();
		gNotes->deselectAll();
		
		int row = gView->getCursorRow();
		myRegion = {row, row};
		myIsSelectingRegion = true;
	}
	else
	{
		myIsSelectingRegion = false;
		selectRegion(myRegion.beginRow, gView->getCursorRow());
	}
}

void selectRegion(int row, int endrow)
{
	if(row != endrow)
	{
		if(row > endrow) swapValues(row, endrow);
		myRegion = {row, endrow};

		double m1 = gTempo->beatToMeasure(row * BEATS_PER_ROW);
		double m2 = gTempo->beatToMeasure(endrow * BEATS_PER_ROW);

		Str::fmt fmt("Selected measure %1 to %2 (%3 measures)");
		fmt.arg(m1, 0, 2).arg(m2, 0, 2).arg(m2 - m1, 0, 2);
		HudNote("%s", fmt);

		setType(REGION);
	}
	else
	{
		setType(NONE);
	}
}

SelectionRegion getSelectedRegion()
{
	return myRegion;
}

}; // SelectionImpl

// ================================================================================================
// Selection API.


Selection* gSelection = nullptr;

void Selection::create()
{
	gSelection = new SelectionImpl;
}

void Selection::destroy()
{
	delete (SelectionImpl*)gSelection;
	gSelection = nullptr;
}

}; // namespace Vortex
