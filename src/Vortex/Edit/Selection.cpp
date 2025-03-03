#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/UiElement.h>
#include <Core/Interface/UiMan.h>

#include <Simfile/NoteUtils.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/GameMode.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>
#include <Vortex/Notefield/SegmentBoxes.h>

#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Static data.

enum class SelectionType { None, Region, Notes, Segments };

struct SelectionUi : public UiElement
{
	void onMousePress(MousePress& press) override;
	void onMouseRelease(MouseRelease& release) override;
	void onSystemCommand(SystemCommand& input) override;
};

namespace Selection
{
	struct State
	{
		Sprite addIcon;
		Sprite subIcon;
	
		int dragStartX = 0;
		ViewOffset dragStartOffset = 0;
		bool isDragging = false;
	
		SelectionRegion region = {};
		bool isSelectingRegion = false;
	
		unordered_set<int> selectedNotes[SimfileConstants::MaxColumns];
		const Chart* selectedNotesChart = nullptr;
	
		// unordered_set<int> selectedSegments[(int)SegmentType::COUNT];
		// const Tempo* selectedSegmentsTempo = nullptr;
		SelectedSegments selectedSegments;
	
		SelectionType type = SelectionType::None;
	};
	static State* state = nullptr;
}
using Selection::state;

// =====================================================================================================================
// Selection dragging.

static void ClearNoteSelection()
{
	for (int col = 0; col < SimfileConstants::MaxColumns; ++col)
	{
		state->selectedNotes[col].clear();
	}
	state->selectedNotesChart = nullptr;
}

static void ClearSegmentSelection()
{
	// TODO: fix
}

static bool HasAnySelectedNotes()
{
	for (auto& col : state->selectedNotes)
	{
		if (col.size()) return true;
	}
	return false;
}

static bool HasAnySelectedSegments()
{
	// TODO: fix
	return false;
}

static bool DragSelectNotes(SelectionModifier mod, double offsetT, double offsetB, int xl, int xr)
{
	// If the selection rectangle is empty, try to select a single note under the mouse.
	if (offsetT == offsetB && xl == xr)
	{
		// TODO: select single note.
		/*
		for (auto& note : Editor::currentChart()->notes->list)
		{
		ViewOffset offset = timeBased ? note.startTime : (double)note.row;
		int dx = xl - View::columnToX(note.col);
		int dy = int(clickY - View::offsetToY(tor));
		if (abs(dy) < mindist)
		{
		int sqrdist = dx * dx + dy * dy;
		if (sqrdist < mindist)
		{
		mindist = sqrdist;
		closestIndex = currentIndex;
		}
		}
		++currentIndex;
		}
		if (closestIndex >= 0)
		{
		selectNotes(mod, &closestIndex, 1);
		return true;
		}
		else
		{
		selectNotes(mod, (int*)nullptr, 0);
		}*/
	}
	else // The selection rectangle is non-empty.
	{
		RowCol begin{ 0, 0 }, end{ 0, 0 };

		// Determine which columns are inside the selected area.
		auto chart = Editor::currentChart();
		int numCols = chart ? chart->gameMode->numCols : 0;
		for (; begin.col < numCols && View::columnToX(begin.col) < xl; ++begin.col);
		for (end.col = begin.col; end.col < numCols && View::columnToX(end.col) < xr; ++end.col);

		// Determine which rows are inside the selected area.
		begin.row = (int)lround(View::offsetToRow(offsetT));
		end.row = (int)lround(View::offsetToRow(offsetB) + 1);

		return (Selection::selectNotes(mod, begin, end) > 0);
	}

	return false;
}

static void DragSelectSegments(SelectionModifier mod, double offsetT, double offsetB, int l, int r)
{
	if (!View::isZoomedIn())
		return;

	int beginRow = (int)lround(View::offsetToRow(offsetT));
	Row endRow = (int)lround(View::offsetToRow(offsetB) + 1);
	auto coords = View::getNotefieldCoords();
	auto segments = SegmentBoxes::getSegmentsInRegion(coords.xl, coords.xr, beginRow, endRow, l, r);
	Selection::selectSegments(mod, segments);
}

static void UpdateType(SelectionType type)
{
	state->type = type;

	// Region selection.
	if (state->type == SelectionType::Region)
	{
		if (state->region.beginRow == state->region.endRow)
		{
			state->region = { 0, 0 };
			state->type = SelectionType::None;
		}
	}
	else
	{
		state->region = { 0, 0 };
	}

	// Note selection.
	if (state->type == SelectionType::Notes)
	{
		if (!HasAnySelectedNotes())
		{
			state->selectedNotesChart = nullptr;
			state->type = SelectionType::None;
		}
	}
	else
	{
		ClearNoteSelection();
	}

	// Segment selection.
	if (state->type == SelectionType::Segments)
	{
		// TODO: fix
		/*
		if (!HasAnySelectedSegments())
		{
			state->selectedSegmentsTempo = nullptr;
			state->type = SelectionType::NONE;
		}
		*/
	}
	else
	{
		ClearSegmentSelection();
	}

	EventSystem::publish<Selection::SelectionChanged>();
}

// =====================================================================================================================
// Event handling.

void SelectionUi::onMousePress(MousePress& input)
{
	// Start dragging a selection box.
	if (input.button == MouseButton::LMB && input.unhandled())
	{
		state->isDragging = true;
		state->dragStartX = input.pos.x;
		state->dragStartOffset = View::yToOffset(input.pos.y);
		input.setHandled();
	}

	// Clear selection.
	if (state->type != SelectionType::None && input.button == MouseButton::RMB && input.unhandled())
	{
		UpdateType(SelectionType::None);
		input.setHandled();
	}
}

void SelectionUi::onMouseRelease(MouseRelease& input)
{
	// Finish dragging a selection box (note selection).
	if (input.button == MouseButton::LMB && state->isDragging)
	{
		// Determine the selection boolean operation.
		SelectionModifier mod = SelectionModifier::Set;
		if (input.modifiers == ModifierKeys::Shift)
			mod = SelectionModifier::Add;
		else if (input.modifiers == ModifierKeys::Alt)
			mod = SelectionModifier::Sub;

		// Determine the selected area.
		double t = state->dragStartOffset;
		double b = View::yToOffset(input.pos.y);
		int l = state->dragStartX;
		int r = input.pos.x;
		sort(t, b);
		sort(l, r);

		// Perform the selection.
		if (mod != SelectionModifier::Set && state->type == SelectionType::Notes)
		{
			DragSelectNotes(mod, t, b, l, r);
		}
		else if (mod != SelectionModifier::Set && state->type == SelectionType::Segments)
		{
			DragSelectSegments(mod, t, b, l, r);
		}
		else
		{
			bool selectedSomeNotes = DragSelectNotes(mod, t, b, l, r);
			if (!selectedSomeNotes) DragSelectSegments(mod, t, b, l, r);
		}

		state->isDragging = false;
	}
}

void SelectionUi::onSystemCommand(SystemCommand& input)
{
	if (input.type == SystemCommandType::SelectAll)
	{
		if (Editor::currentChart())
		{
			Selection::selectAllNotes(SelectionModifier::Set);
		}
		else
		{
			Selection::selectAllSegments(SelectionModifier::Set);
		}
		input.setHandled();
	}
}

// =====================================================================================================================
// Initialization

void Selection::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<SelectionUi>());

	Texture icons[2];
	// Texture::createTiles("assets/icons selection.png", 16, 16, 2, icons);
	// state->addIcon = icons[0];
	// state->subIcon = icons[1];
}

void Selection::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Selection type.

bool Selection::hasRegionSelection()
{
	return state->type == SelectionType::Region;
}

bool Selection::hasNoteSelection(const Chart* chart)
{
	return state->type == SelectionType::Notes && chart == state->selectedNotesChart;
}

bool Selection::hasSegmentSelection(const Tempo* tempo)
{
	// TODO: fix
	/*
	return state->type == SelectionType::SEGMENTS && tempo == state->selectedSegmentsTempo;
	*/
	return false;
}

// =====================================================================================================================
// Note selection.

typedef function<bool(int col, const Note&)> NoteSelectionPredicate;

int SetSelectedNotes(bool showHudMessage, SelectionModifier mod, NoteSelectionPredicate predicate)
{
	auto chart = Editor::currentChart();
	if (chart != state->selectedNotesChart)
	{
		ClearNoteSelection();
		state->selectedNotesChart = chart;
	}

	int numChanges = 0;
	int typeMask = 0;

	if (chart)
	{
		for (int i = 0; i < chart->gameMode->numCols; ++i)
		{
			auto& col = chart->notes[i];
			auto& sel = state->selectedNotes[i];
			switch (mod)
			{
			case SelectionModifier::Set:
				sel.clear();
				for (auto& note : col)
				{
					if (predicate(i, note))
					{
						sel.insert(note.row);
						typeMask |= 1 << note.type;
						++numChanges;
					}
				}
				break;
			case SelectionModifier::Add:
				for (auto& note : col)
				{
					if (predicate(i, note))
					{
						if (sel.insert(note.row).second)
						{
							typeMask |= 1 << note.type;
							++numChanges;
						}
					}
				}
				break;
			case SelectionModifier::Sub:
				for (auto& note : col)
				{
					if (predicate(i, note))
					{
						if (sel.erase(note.row))
						{
							typeMask |= 1 << note.type;
							++numChanges;
						}
					}
				}
				break;
			}
		}
	}

	if (numChanges >= 1 && showHudMessage)
	{
		const char* name = "note";
		const char* op = (mod == SelectionModifier::Sub) ? "Deselected" : "Selected";
		bool isPlural = (numChanges != 1);
		for (int i = 0; i < (int)NoteType::Count; ++i)
		{
			if (typeMask == (1 << i))
			{
				name = GetNoteName(i, isPlural);
			}
		}

		Hud::note("%s %i %s.", op, numChanges, name);
	}

	UpdateType(SelectionType::Notes);
	return numChanges;
}

int Selection::selectAllNotes(SelectionModifier mod)
{
	return SetSelectedNotes(true, mod, [](int col, const Note& note){
		return true;
	});
}

int Selection::selectNotes(SelectionModifier mod, NoteType type)
{
	return SetSelectedNotes(true, mod, [=](int col, const Note& note){
		return (note.type == (uint)type);
	});
}

int Selection::selectNotes(SelectionModifier mod, Quantization quantization)
{
	return SetSelectedNotes(true, mod, [=](int col, const Note& note){
		return (ToQuantization(note.row) == quantization);
	});
}

int Selection::selectNotes(SelectionModifier mod, RowCol topLeft, RowCol bottomRight)
{
	return SetSelectedNotes(true, mod, [=](int col, const Note& note){
		return (col >= topLeft.col && col < bottomRight.col &&
			note.row >= topLeft.row && note.row < bottomRight.row);
	});
}

int Selection::selectNotes(SelectionModifier mod, const NoteSet& set)
{
	int numCols = set.numColumns();

	vector<unordered_set<int>> rows;
	for (int i = 0; i < numCols; ++i)
	{
		auto& target = rows[i];
		for (auto& note : set[i])
		{
			target.insert(note.row);
		}
	}

	return SetSelectedNotes(true, mod, [&](int col, const Note& note) {
		return rows[col].count(note.row) == 1;
	});
}

int Selection::getSelectedNotes(NoteSet& out)
{
	int numNotes = 0;
	auto chart = Editor::currentChart();
	if (chart != nullptr && chart == state->selectedNotesChart)
	{
		int numCols = min(chart->gameMode->numCols, out.numColumns());
		switch (state->type)
		{
		case SelectionType::Notes: {
			for (int i = 0; i < numCols; ++i)
			{
				auto& col = chart->notes[i];
				auto& sel = state->selectedNotes[i];
				for (auto& note : col)
				{
					if (sel.count(note.row))
					{
						out[i].append(note);
						++numNotes;
					}
				}
			}
		} break;
		case SelectionType::Region: {
			for (int i = 0; i < numCols; ++i)
			{
				auto note = chart->notes[i].begin();
				auto end = chart->notes[i].end();
				for (; note != end && note->row < state->region.beginRow; ++note);
				for (; note != end && note->row <= state->region.endRow; ++note)
				{
					out[i].append(*note);
					++numNotes;
				}
			}
		} break;
		}
	}
	return numNotes;
}

bool Selection::hasSelectedNote(int col, const Note& note)
{
	return state->selectedNotes[col].count(note.row) == 1;
}

// =====================================================================================================================
// Segment selection.

/*
typedef function<bool(SegmentType, const Segment*)> SegmentSelectionPredicate;

int SetSelectedSegments(bool showHudMessage, SelectionModifier mod, SegmentSelectionPredicate predicate)
{
	auto tempo = Editor::currentTempo();
	if (tempo != state->selectedSegmentsTempo)
	{
		ClearSegmentSelection();
		state->selectedSegmentsTempo = tempo;
	}

	int numChanges = 0;
	int typeMask = 0;

	if (tempo)
	{
		for (auto& segments : tempo->segments)
		{
			auto type = segments.type();
			auto& selection = state->selectedSegments[(int)type];
			switch (mod)
			{
			case SelectionModifier::SET:
				for (auto& seg : segments)
				{
					if (predicate(type, &seg))
					{
						selection.insert(seg.row);
						typeMask |= 1 << (size_t)type;
						++numChanges;
					}
					else
					{
						selection.erase(seg.row);
					}
				}
				break;
			case SelectionModifier::ADD:
				for (auto& seg : segments)
				{
					if (predicate(type, &seg))
					{
						if (selection.insert(seg.row).second)
						{
							typeMask |= 1 << (size_t)type;
							++numChanges;
						}
					}
				}
				break;
			case SelectionModifier::SUB:
				for (auto& seg : segments)
				{
					if (predicate(type, &seg))
					{
						if (selection.erase(seg.row))
						{
							typeMask |= 1 << (size_t)type;
							++numChanges;
						}
					}
				}
				break;
			}
		}
	}

	if (numChanges >= 1 && showHudMessage)
	{
		const char* name = (numChanges > 1) ? "segments" : "segment";
		const char* op = (mod == SelectionModifier::SUB) ? "Deselected" : "Selected";

		for (size_t i = 0; i < (size_t)SegmentType::COUNT; ++i)
		{
			if (typeMask == (1 << i))
			{
				auto meta = Segment::getTypeData((SegmentType)i);
				name = (numChanges > 1) ? meta->plural : meta->singular;
			}
		}

		HudNote("%s %i %s.", op, numChanges, name);
	}

	UpdateType(SelectionType::SEGMENTS);
	return numChanges;
}
*/

int Selection::selectAllSegments(SelectionModifier mod)
{
	// TODO: Fix
	/*
	return SetSelectedSegments(true, mod, [](SegmentType type, const Segment* seg){
		return true;
	});
	*/
	return 0;
}

int Selection::selectSegments(SelectionModifier mod, const SelectedSegments& segments)
{
	// TODO: Fix
	/*
	SegmentSet::SegmentList::ConstIterator next[(int)SegmentType::COUNT];
	SegmentSet::SegmentList::ConstIterator end[(int)SegmentType::COUNT];

	for (auto& typedList : segments)
	{
		auto type = typedList.type();
		next[(int)type] = typedList.begin();
		end[(int)type] = typedList.end();
	}

	return SetSelectedSegments(true, mod, [&](SegmentType type, const Segment* seg) {
		if (next[(int)type] == end[(int)type]) return false;
		int diff = next[(int)type]->row - seg->row;
		while (diff < 0)
		{
			++next[(int)type];
			if (next[(int)type] == end[(int)type]) return false;
			diff = next[(int)type]->row - seg->row;
		}
		return (diff == 0);
	});
	*/
	return 0;
}

const SelectedSegments& Selection::getSelectedSegments()
{
	// TODO: Fix
	/*
	int count = 0;
	auto tempo = Editor::currentTempo();
	if (tempo == state->selectedSegmentsTempo)
	{
		switch (state->type)
		{
		case SelectionType::SEGMENTS: {
			for (auto& segments : tempo->segments)
			{
				auto type = segments.type();
				auto& selection = state->selectedSegments[(int)type];
				for (auto& seg : segments)
				{
					if (selection.count(seg.row) == 1)
					{
						out.append(type, &seg);
						++count;
					}
				}
			}
		} break;
		case SelectionType::REGION: {
			for (auto& segments : tempo->segments)
			{
				auto type = segments.type();
				auto seg = segments.begin();
				auto end = segments.end();
				for (; seg != end && seg->row < state->region.beginRow; ++seg);
				for (; seg != end && seg->row <= state->region.endRow; ++seg)
				{
					out.append(type, seg.ptr);
					++count;
				}
			}
		} break;
		}
	}
	return count;
	*/
	return state->selectedSegments;
}

// =====================================================================================================================
// Region selection.

void Selection::selectRegion()
{
	if (!state->isSelectingRegion)
	{
		UpdateType(SelectionType::None);
		Row row = View::getCursorRowI();
		state->region = {row, row};
		state->isSelectingRegion = true;
	}
	else
	{
		state->isSelectingRegion = false;
		selectRegion(state->region.beginRow, View::getCursorRowI());
	}
}

void Selection::selectRegion(Row row, Row endRow)
{
	if (row != endRow)
	{
		sort(row, endRow);
		state->region = {row, endRow};

		auto& timing = Editor::currentTempo()->timing;
		double m1 = timing.rowToMeasure(row);
		double m2 = timing.rowToMeasure(endRow);

		auto fmt = format("Selected measure {:.2f} to {:.2f} ({:.2f} measures)", m1, m2, m2 - m1);
		Hud::note("%s", fmt.data());
	}
	UpdateType(SelectionType::Region);
}

SelectionRegion Selection::getRegion()
{
	return state->region;
}

// =====================================================================================================================
// Drawing functions.

void Selection::drawRegionSelection()
{
	if (state->isSelectingRegion || state->region.beginRow != state->region.endRow)
	{
		Color outline = Color(153, 255, 153, 153);
		auto coords = View::getReceptorCoords();
		if (state->isSelectingRegion)
		{
			int y = (int)View::rowToY(state->region.beginRow);
			Draw::fill(Rect((int)coords.xl, y - 1, (int)coords.xr, y + 1), outline);
		}
		else if (state->region.beginRow != state->region.endRow)
		{
			int t = (int)View::rowToY(state->region.beginRow);
			int b = (int)View::rowToY(state->region.endRow);
			Rect rect((int)coords.xl, t, (int)coords.xr, b);
			Draw::fill(rect, Color(153, 255, 153, 90));
			Draw::outline(rect, outline);
		}
	}
}

void Selection::drawSelectionBox()
{
	if (state->isDragging)
	{
		Vec2 mpos = Window::getMousePos();
		Vec2 start(state->dragStartX, (int)View::offsetToY(state->dragStartOffset));

		// Selection rectangle.
		Color outline = Color(255, 191, 128, 128);
		Color fill = Color(255, 191, 128, 89);

		int x = start.x, x2 = mpos.x;
		int y = start.y, y2 = mpos.y;

		sort(x, x2);
		sort(y, y2);

		Rect rect(x, y, x2, y2);
		Draw::fill(rect, fill);
		Draw::outline(rect, outline);

		// Indicator icon for subtractive/additive selection.
		ModifierKeys modifiers = Window::getKeyFlags();
		if (modifiers & (ModifierKeys::Shift | ModifierKeys::Alt))
		{
			auto& spr = (modifiers == ModifierKeys::Shift) ? state->addIcon : state->subIcon;
			spr.draw(start);
		}
	}
}

} // namespace AV
