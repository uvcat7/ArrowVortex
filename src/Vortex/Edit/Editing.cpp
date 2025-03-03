#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Xmr.h>
#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Debug.h>

#include <Core/Interface/UiMan.h>

#include <Simfile/Simfile.h>
#include <Simfile/GameMode.h>
#include <Simfile/Tempo/SegmentSet.h>
#include <Simfile/NoteSet.h>
#include <Simfile/History.h>
#include <Simfile/NoteUtils.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

#include <Vortex/Notefield/Notefield.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Clipboard.h>
#include <Vortex/Edit/Editing.h>

#include <Core/Graphics/Draw.h>

namespace AV {
	
using namespace std;
using namespace Util;

enum class PlaceMode { None, New, AfterRemove };

struct PlacingNote { int player, startRow, endRow; PlaceMode mode; };

static int KeyToCol(KeyCode key)
{
	return (key == KeyCode::Digit0) ? 9 : ((int)key - (int)KeyCode::Digit1);
}

static bool IsActive(const PlacingNote& n)
{
	return (n.mode == PlaceMode::New || (n.mode == PlaceMode::AfterRemove && n.startRow != n.endRow));
}

static Note PlacingNoteToNote(const PlacingNote& pnote)
{
	Note out;
	out.row = min(pnote.startRow, pnote.endRow);
	out.endRow = max(pnote.startRow, pnote.endRow);
	out.keysoundId = 0;
	out.player = pnote.player;
	out.type = (uint)((out.endRow > out.row) ? NoteType::Hold : NoteType::Step);
	return out;
}

static int GetCurrentPlacementRow()
{
	Row row = View::getCursorRowI();
	if (!MusicMan::isPaused())
	{
		row = View::snapRow(row, SnapDir::Closest);
	}
	return row;
}

static int GetNumCols()
{
	auto chart = Editor::currentChart();
	return chart ? chart->gameMode->numCols : 0;
}

static int GetNumPlayers()
{
	auto chart = Editor::currentChart();
	return chart ? chart->gameMode->numPlayers : 0;
}

// =====================================================================================================================
// Static data.

struct EditingUi : public UiElement
{
	void onSystemCommand(SystemCommand& input) override;
	void onKeyPress(KeyPress& input) override;
	void onKeyRelease(KeyRelease& input) override;
	void onMousePress(MousePress& input) override;
	void onMouseScroll(MouseScroll& input) override;
};

namespace Editing
{
	struct State
	{
		int curPlayer = 0;
	
		PlacingNote placingNotes[SimfileConstants::MaxColumns] = {};
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using Editing::state;

// =====================================================================================================================
// Note placement.

static void FinishNotePlacement(int col)
{
	auto& pnote = state->placingNotes[col];
	pnote.endRow = GetCurrentPlacementRow();
	bool extendingUpwards = (pnote.startRow > pnote.endRow);
	sort(pnote.startRow, pnote.endRow);
	auto chart = Editor::currentChart();

	if (chart && IsActive(pnote))
	{
		Note addedNote = PlacingNoteToNote(pnote);

		// If the note is extending into another roll/hold, merge them.
		if (extendingUpwards)
		{
			auto n = GetNoteIntersecting(chart, addedNote.row, col);
			if (n && n->endRow > n->row && addedNote.endRow > n->endRow)
			{
				addedNote = *n;
				addedNote.endRow = pnote.endRow;
			}
		}
		else
		{
			auto n = GetNoteIntersecting(chart, addedNote.endRow, col);
			if (n && n->endRow > n->row && addedNote.row < n->row)
			{
				addedNote = *n;
				addedNote.row = pnote.startRow;
			}
		}

		ChartModification mod(chart->gameMode->numCols);
		mod.notesToAdd[col].append(addedNote);
		chart->modify(mod);
	}

	pnote.mode = PlaceMode::None;
}

// =====================================================================================================================
// Event handlers.

void EditingUi::onSystemCommand(SystemCommand& input)
{
	if (!input.unhandled()) return;

	// Copy/pasting.
	if (input.type == SystemCommandType::Cut)
	{
		Clipboard::cut();
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Copy)
	{
		Clipboard::copy();
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Paste)
	{
		Clipboard::paste();
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Delete)
	{
		Editing::deleteSelection();
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Undo || input.type == SystemCommandType::Redo)
	{
		auto chart = Editor::currentChart();
		if (!chart) return;

		if (input.type == SystemCommandType::Undo)
			chart->history->undo();
		else
			chart->history->redo();

		input.setHandled();
	}
}

void EditingUi::onKeyPress(KeyPress& input)
{
	return; // TODO: re-enable.

	if (!input.unhandled()) return;

	auto sim = Editor::currentSimfile();
	auto chart = Editor::currentChart();

	auto kc = input.key.code;

	// Placing notes.
	if (chart && kc >= KeyCode::Digit0 && kc <= KeyCode::Digit9 && !input.isRepeated)
	{
		int numCols = GetNumCols();
		int col = KeyToCol(kc);
		Row row = GetCurrentPlacementRow();
		if (input.key.modifiers == ModifierKeys::Alt) col += GetNumCols() / 2;
		if (col >= 0 && col < GetNumCols())
		{
			ChartModification mod(numCols);
			auto note = GetNoteAt(chart, row, col);
			if (note)
			{
				if (MusicMan::isPaused())
				{
					state->placingNotes[col] = { state->curPlayer, row, row, PlaceMode::AfterRemove };
				}
				mod.notesToRemove[col].append(*note);
				chart->modify(mod);
			}
			else
			{
				mod.notesToAdd[col].append(Note(row, row, NoteType::Step, (uint)state->curPlayer, 0));
				if (input.key.modifiers == ModifierKeys::Shift)
				{
					mod.notesToAdd[col].begin()->type = (uint)NoteType::Mine;
					chart->modify(mod);
				}
				else if (MusicMan::isPaused())
				{
					state->placingNotes[col] = { state->curPlayer, row, row, PlaceMode::New };
				}
				else
				{
					chart->modify(mod);
				}
			}
		}

		input.setHandled();
		return;
	}

	// Finish tweaking.
	auto mode = TempoTweaker::getTweakMode();
	if ((kc == KeyCode::Return || kc == KeyCode::Escape) && mode != TweakMode::None && input.unhandled())
	{
		TempoTweaker::stopTweaking(kc == KeyCode::Return);
		input.setHandled();
	}
}

void EditingUi::onKeyRelease(KeyRelease& input)
{
	return; // TODO: re-enable.

	// Finish placing notes.
	auto chart = Editor::currentChart();
	if (chart && input.key.code >= KeyCode::Digit0 && input.key.code <= KeyCode::Digit9)
	{
		Row row = GetCurrentPlacementRow();
		int col = KeyToCol(input.key.code);
		if (input.key.modifiers == ModifierKeys::Alt) col += GetNumCols() / 2;
		if (col >= 0 && col < GetNumCols())
		{
			FinishNotePlacement(col);
		}
	}
}

void EditingUi::onMousePress(MousePress& input)
{
	return; // TODO: re-enable.

	// Finish tweaking.
	auto mode = TempoTweaker::getTweakMode();
	if ((input.button == MouseButton::LMB || input.button == MouseButton::RMB) && mode != TweakMode::None && input.unhandled())
	{
		TempoTweaker::stopTweaking(input.button == MouseButton::LMB);
		input.setHandled();
	}
}

void EditingUi::onMouseScroll(MouseScroll& input)
{
	return; // TODO: re-enable.

	auto mode = TempoTweaker::getTweakMode();
	if (mode != TweakMode::None && (input.modifiers & (ModifierKeys::Shift | ModifierKeys::Alt)) && input.unhandled())
	{
		double deltas[] = { 0, 0.1, 1.0, 0.1 };
		double d = input.isUp ? (-deltas[(int)mode]) : deltas[(int)mode];
		if (input.modifiers == ModifierKeys::Alt) d *= 0.01;

		double r = fabs(d);
		double v = floor((TempoTweaker::getTweakValue() + d) / r + 0.5) * r;
		TempoTweaker::setTweakValue(v);

		input.setHandled();
	}
}

static void HandleActiveChartChanged()
{
	if (state->curPlayer >= GetNumPlayers())
		state->curPlayer = 0;
}

// =====================================================================================================================
// Initialization

void Editing::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<EditingUi>());

	for (PlacingNote& n : state->placingNotes)
		n.mode = PlaceMode::None;

	state->subscriptions.add<Editor::ActiveChartChanged>(HandleActiveChartChanged);
}

void Editing::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Member functions.

void Editing::deleteSelection()
{
	return; // TODO: re-enable.

	if (Selection::hasSegmentSelection(Editor::currentTempo()))
	{
		// TODO: fix
		// TempoModification mod;
		// Selection::getSelectedSegments(mod.segmentsToRemove);
		// Editor::currentTempo()->modify(mod);
	}
	else if (Editor::currentChart())
	{
		auto chart = Editor::currentChart();
		ChartModification mod(chart->gameMode->numCols);
		auto selection = Selection::getSelectedNotes(mod.notesToRemove);
		Editor::currentChart()->modify(mod);
	}
}

void Editing::changeHoldsToRolls()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	static const ChartModification::Description desc[2] = {
		{"Converted %1 hold to roll.", "Converted %1 holds to rolls."},
		{"Converted %1 roll to hold.", "Converted %1 rolls to holds."},
	};
	
	ChartModification edit(chart->gameMode->numCols);
	Selection::getSelectedNotes(edit.notesToAdd);

	int numHolds = 0, numRolls = 0;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			if (n.endRow > n.row)
			{
				if (n.type == (uint)NoteType::Roll)
				{
					++numRolls;
				}
				else
				{
					++numHolds;
				}
			}
		}
	}

	if (numHolds > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				if (n.endRow > n.row) n.type = (uint)NoteType::Roll;
			}
		}
		edit.description = desc + 0;
		chart->modify(edit);
	}
	else if (numRolls > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				if (n.endRow > n.row) n.type = (uint)NoteType::Hold;
			}
		}
		edit.description = desc + 1;
		chart->modify(edit);
	}
	else
	{
		Hud::note("No holds/rolls selected.");
	}
}

void Editing::changeHoldsToSteps()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	ChartModification edit(chart->gameMode->numCols);

	Selection::getSelectedNotes(edit.notesToAdd);

	int numHolds = 0;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			if (n.endRow > n.row)
			{
				n.endRow = n.row;
				n.type = (uint)NoteType::Step;
				++numHolds;
			}
		}
	}
	if (chart && numHolds > 0)
	{
		static const ChartModification::Description desc = {
			"Converted %1 hold to step.", "Converted %1 holds to steps."
		};
		edit.description = &desc;
		chart->modify(edit);
	}
	else
	{
		Hud::note("There are no holds/rolls selected.");
	}
}

void Editing::changeNotesToMines()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	static const ChartModification::Description desc[2] = {
		{"Converted %1 note to mine.", "Converted %1 notes to mines."},
		{"Converted %1 mine to note.", "Converted %1 mines to notes."},
	};

	ChartModification edit(chart->gameMode->numCols);
	Selection::getSelectedNotes(edit.notesToAdd);

	int numNotes = 0, numMines = 0;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			if (n.type == (uint)NoteType::Mine)
			{
				++numMines;
			}
			else
			{
				++numNotes;
			}
		}
	}

	if (numNotes > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				n.type = (uint)NoteType::Mine;
				n.endRow = n.row;
			}
		}
		edit.description = desc + 0;
		chart->modify(edit);
	}
	else if (numMines > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				if (n.type == (uint)NoteType::Mine)
				{
					n.type = (uint)NoteType::Step;
				}
			}
		}
		edit.description = desc + 1;
		chart->modify(edit);
	}
	else
	{
		Hud::note("No notes/mines selected.");
	}
}

void Editing::changeNotesToFakes()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	static const ChartModification::Description desc[2] = {
		{"Converted %1 note to fake.", "Converted %1 notes to fakes."},
		{"Converted %1 fake to note.", "Converted %1 fakes to notes."},
	};

	ChartModification edit(chart->gameMode->numCols);
	Selection::getSelectedNotes(edit.notesToAdd);

	int numNotes = 0, numFakes = 0;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			if (n.type == (uint)NoteType::Fake)
			{
				++numFakes;
			}
			else
			{
				++numNotes;
			}
		}
	}

	if (numNotes > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				n.type = (uint)NoteType::Fake;
				n.endRow = n.row;
			}
		}
		edit.description = desc + 0;
		chart->modify(edit);
	}
	else if (numFakes > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				if (n.type == (uint)NoteType::Fake) n.type = (uint)NoteType::Step;
			}
		}
		edit.description = desc + 1;
		chart->modify(edit);
	}
	else
	{
		Hud::note("No notes/fakes selected.");
	}
}

void Editing::changeNotesToLifts()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	static const ChartModification::Description desc[2] = {
		{"Converted %1 note to lift.", "Converted %1 notes to lifts."},
		{"Converted %1 lift to note.", "Converted %1 lifts to notes."},
	};

	ChartModification edit(chart->gameMode->numCols);
	Selection::getSelectedNotes(edit.notesToAdd);

	int numNotes = 0, numLifts = 0;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			if (n.type == (uint)NoteType::Lift)
			{
				++numLifts;
			}
			else
			{
				++numNotes;
			}
		}
	}

	if (numNotes > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				n.type = (uint)NoteType::Lift;
				n.endRow = n.row;
			}
		}
		edit.description = desc + 0;
		chart->modify(edit);
	}
	else if (numLifts > 0)
	{
		for (auto& col : edit.notesToAdd)
		{
			for (auto& n : col)
			{
				if (n.type == (uint)NoteType::Lift)
					n.type = (uint)NoteType::Step;
			}
		}
		edit.description = desc + 1;
		chart->modify(edit);
	}
	else
	{
		Hud::note("No notes/lifts selected.");
	}
}

void Editing::changePlayerNumber()
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	auto gameMode = chart->gameMode;
	int numPlayers = gameMode->numPlayers;

	// Check if the current gameMode actually supports more than 1 player.
	if (numPlayers <= 1)
	{
		
		Hud::note("%s only has one player.", gameMode->mode.data());
		return;
	}

	// If we do not have a note selection, we switch player for note placement instead.
	ChartModification edit(chart->gameMode->numCols);
	Selection::getSelectedNotes(edit.notesToAdd);
	if (edit.notesToAdd.numNotes() == 0)
	{
		int newPlayer = (state->curPlayer + 1) % numPlayers;
		if (newPlayer != state->curPlayer)
		{
			Hud::note("Switched to player %i", newPlayer + 1);
			state->curPlayer = newPlayer;
		}
		return;
	}

	// Switch all notes in the selection to the next player.
	set<int> players;
	for (auto& col : edit.notesToAdd)
	{
		for (auto& n : col)
		{
			players.insert(n.player);
			n.player = (n.player + 1) % numPlayers;
		}
	}
	
	// We do have a selection, switch players for all selected notes.
	static const ChartModification::Description descs[4] = {
		{"Converted %1 note to P1.", "Converted %1 notes to P1."},
		{"Converted %1 note to P2.", "Converted %1 notes to P2."},
		{"Converted %1 note to P3.", "Converted %1 notes to P3."},
		{"Switched player for %1 note.", "Switched player for %1 notes."},
	};
	edit.description = descs + (players.size() <= 1 ? min(*players.begin(), 3) : 3);
	chart->modify(edit);
}

template <typename T>
static T readFromBuffer(vector<uchar>& buffer, int& pos)
{
	if (pos + (int)sizeof(T) <= buffer.size())
	{
		pos += sizeof(T);
		return *(T*)(buffer.data() + pos - sizeof(T));
	}
	pos = buffer.size();
	return 0;
}

static void SwitchColumns(ChartModification mod, const NoteSet& selection, const int* tableA, const int* tableB)
{
	int numCols = mod.notesToAdd.numColumns();
	for (int i = 0; i < numCols; ++i)
	{
		int j = i;
		if (tableA && tableB)
		{
			j = tableB[tableA[i]];
		}
		else if (tableA)
		{
			j = tableA[i];
		}
		if (j != i)
		{
			mod.notesToRemove[i].assign(selection[i]);
			mod.notesToAdd[j].assign(selection[i]);
		}
	}
}

void Editing::mirrorNotes(Mirror type)
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	ChartModification edit(chart->gameMode->numCols);

	NoteSet selection(chart->gameMode->numCols);
	Selection::getSelectedNotes(selection);

	if (edit.notesToAdd.numNotes() == 0)
	{
		Hud::note("There are no notes selected.");
		return;
	}

	edit.removePolicy = ChartModification::RemovePolicy::OverlappingRows;

	// Mirror the selected notes.
	auto gameMode = chart->gameMode;
	switch (type)
	{
	case Mirror::Horizontal:
		SwitchColumns(edit, selection, gameMode->mirrorTableH, nullptr); break;
	case Mirror::Vertical:
		SwitchColumns(edit, selection, gameMode->mirrorTableV, nullptr); break;
	case Mirror::Both:
		SwitchColumns(edit, selection, gameMode->mirrorTableH, gameMode->mirrorTableV); break;
	}

	// Perform the mirror operation.
	static const ChartModification::Description descs[3] = {
		{"Horizontally mirrored %1 note.", "Horizontally mirrored %1 notes."},
		{"Vertically mirrored %1 note.", "Vertically mirrored %1 notes."},
		{"Fully mirrored %1 note.", "Fully mirrored %1 notes."},
	};
	edit.description = descs + (size_t)type;
	Editor::currentChart()->modify(edit);

	// Reselect the mirrored notes.
	if (Selection::hasNoteSelection(chart))
	{
		Selection::selectNotes(SelectionModifier::Set, edit.notesToAdd);
	}
}

void Editing::scaleNotes(int numerator, int denominator)
{
	return; // TODO: re-enable.

	auto chart = Editor::currentChart();
	if (!chart) return;

	int numCols = chart->gameMode->numCols;
	ChartModification edit(numCols);
	Selection::getSelectedNotes(edit.notesToAdd);

	edit.notesToRemove = edit.notesToAdd;
	edit.removePolicy = ChartModification::RemovePolicy::EntireRegion;

	if (edit.notesToAdd.numNotes() == 0)
	{
		Hud::note("There are no notes selected.");
		return;
	}

	// Find the first row.
	int topRow = INT_MAX;
	for (auto& col : edit.notesToAdd)
	{
		if (col.size())
		{
			topRow = min(topRow, col.begin()->row);
		}
	}

	for (int col = 0; col < numCols; ++col)
	{
		// Scale the rows of the selected notes.
		for (Note& n : edit.notesToAdd[col])
		{
			n.row    = (n.row    - topRow) * numerator / denominator + topRow;
			n.endRow = (n.endRow - topRow) * numerator / denominator + topRow;
		}

		// If we are using region selection, we remove all expanded notes outside the selection range.
		if (Selection::hasRegionSelection())
		{
			auto region = Selection::getRegion();
			for (int col = 0; col < numCols; ++col)
			{
				for (auto& note : edit.notesToAdd[col])
				{
					if (note.row > region.endRow) note.row = -1;
				}
			}
			edit.notesToAdd.removeMarkedNotes();
		}
	}

	// Perform the scale operation.
	static const ChartModification::Description tExp = {"Expanded %1 note.", "Expanded %1 notes."};
	static const ChartModification::Description tCom = {"Compressed %1 note.", "Compressed %1 notes."};
	const ChartModification::Description* desc = (numerator > denominator) ? &tExp : &tCom;
	chart->modify(edit);

	// Reselect the scaled notes.
	if (Selection::hasNoteSelection(chart))
	{
		Selection::selectNotes(SelectionModifier::Set, edit.notesToAdd);
	}
}

void Editing::insertRows(Row row, int numRows, bool curChartOnly)
{
	return; // TODO: re-enable.

	auto sim = Editor::currentSimfile();
	if (sim)
	{
		// TODO: reimplement
		/*
		gHistory->startChain();
		if (curChartOnly)
		{
			auto chart = Editor::currentChart();
			if (chart->tempo) chart->tempo->insertRows(row, numRows);
			chart->notes->insertRows(row, numRows);
		}
		else
		{
			for (auto chart : Editor::currentSimfile()->charts)
			{
				if (chart->tempo) chart->tempo->insertRows(row, numRows);
				chart->notes->insertRows(row, numRows);
			}
		}
		gHistory->finishChain((numRows > 0) ? "Insert beats" : "Delete beats");
		*/
	}
}

/*
void Editing::convertStreamToTriplets()
{
	Notes::Modification mod;

	Selection::getSelectedNotes(mod.notesToRemove);
	auto region = Selection::getRegion();

	// Use the rows of every 3 out of 4 notes.
	for (int colI = 0, rowI = 0; rowI < mod.notesToRemove.size(); ++colI, ++rowI)
	{
		Note n = mod.notesToRemove.begin()[colI];
		n.row = n.endRow = mod.notesToRemove.begin()[rowI].row;
		mod.notesToAdd.append(n);
		if (colI % 3 == 2) ++rowI;
	}

	Editor::currentChart()->notes->modify(mod);
}
*/

// =====================================================================================================================
// Routine functions.

/*
void Editing::convertRoutineToCouples()
{
	const char* title = "Convert Routine to ITG Couples";

	// Find all routine charts.
	vector<const Chart*> charts;
	for (int i = 0; i < Editor::getNumCharts(); ++i)
	{
		auto chart = Editor::getChart(i);
		if (chart->gameMode->id == "dance-routine")
		{
			charts.push_back(chart);
		}
	}
	if (charts.empty())
	{
		HudNote("There are no routine charts to convert.");
		return;
	}

	// Make a list of all rows that contain P2 notes.
	set<int> p2rows;
	for(auto& chart : charts)
	{
		for (auto& n : chart->notes->list)
		{
			if (n.player != 0)
			{
				p2rows.insert(n.row);
			}
		}
	}
	if (p2rows.empty())
	{
		HudNote("There are no player 2 notes, nothing was converted.");
		return;
	}

	// TODO: reimplement
	/*
	gHistory->startChain();

	// Find the game mode dance double.
	auto danceDouble = GameMan::findMode("dance-double", 8, 1);

	// Bump all player 2 notes down one row.
	for(auto& chart : charts)
	{
		Notes::Modification edit;
		for(auto& note : chart->notes->list)
		{
			Note n = note;
			if (note.player != 0)
			{
				++n.row, ++n.endRow;
				n.player = 0;
			}
			edit.notesToAdd.append(n);
		}
		stable_sort(edit.notesToAdd.begin(), edit.notesToAdd.end(), LessThanRowCol<Note, Note>);

		Chart* chart = new Chart(chart);
		chart->gameMode = danceDouble;
		Editor::currentSimfile()->addChart(chart);

		edit.removePolicy = Notes::Modification::REMOVE_ENTIRE_REGION;

		Editor::currentChart()->notes->modify(edit);
	}

	// Apply negative BPM skips.
	auto segments = TempoTweaker::getSegments();
	for (Row row : p2rows)
	{
		double base = segments->getRow<Stop>(row).seconds;
		double len = 60.0 / (tempo->getBpm(row) * RowsPerBeat);

		TempoModification mod;
		mod.segmentsToAdd.append(Stop(row, -len));
		mod.segmentsToAdd.append(Stop(row + 1, base + len));
		Editor::currentTempo()->modify(mod);
	}

	gHistory->finishChain(title);
}

void Editing::convertCouplesToRoutine()
{
	auto tempo = Editor::currentTempo();

	// Find all doubles charts.
	vector<const Chart*> charts;
	for (int i = 0; i < Editor::getNumCharts(); ++i)
	{
		auto chart = Editor::getChart(i);
		if (chart->gameMode->id == "dance-double")
		{
			charts.push_back(chart);
		}
	}
	if (charts.empty())
	{
		HudNote("There are no doubles charts to convert.");
		return;
	}

	// Make a list of all rows that have negative BPM skips.
	auto it = tempo->begin<Stop>();
	auto end = tempo->end<Stop>();

	set<int> p2rows;
	int prevRow = 0;
	double prevStop = 0.0;
	for (; it != end; ++it)
	{
		if (prevStop < 0 && it->seconds > 0 && it->row == prevRow + 1)
		{
			p2rows.insert(prevRow);
		}
		prevStop = it->seconds;
		prevRow = it->row;
	}
	if (p2rows.empty())
	{
		HudNote("There are no player 2 notes, nothing was converted.");
		return;
	}

	// TODO: reimplement
	/*
	gHistory->startChain();

	// Find the game mode dance routine.
	auto danceRoutine = GameMan::findMode("dance-routine", 8, 2);
	if (!danceRoutine)
	{
		HudError("Could not find the dance-routine gameMode.");
		return;
	}

	// Bump all player 2 notes down one row.
	Chart* newChart = nullptr;
	for (auto chart : charts)
	{
		Notes::Modification edit;
		for (auto& note : chart->notes->list)
		{
			Note n = note;
			if (p2rows.find(n.row - 1) != p2rows.end())
			{
				n.player = 1;
				--n.row, --n.endRow;
			}
			edit.notesToAdd.append(n);
		}
		stable_sort(edit.notesToAdd.begin(), edit.notesToAdd.end(), LessThanRowCol<Note, Note>);

		Chart* chart = new Chart(chart);
		chart->gameMode = danceRoutine;
		Editor::currentSimfile()->addChart(chart);

		edit.removePolicy = Notes::Modification::REMOVE_ENTIRE_REGION;

		Editor::currentChart()->notes->modify(edit);
	}

	// Remove negative BPM skips.
	for (Row row : p2rows)
	{
		double len = segments->getRow<Stop>(row).seconds;
		len += segments->getRow<Stop>(row + 1).seconds;

		TempoModification mod;
		mod.segmentsToAdd.append(Stop(row, 0));
		mod.segmentsToAdd.append(Stop(row + 1, len));
		Editor::currentTempo()->modify(mod);
	}

	gHistory->finishChain("Convert ITG Couples to Routine");
}

void Editing::exportNotesAsLuaTable()
{
	const Chart* chart = Editor::currentChart();
	if (!chart)
	{
		HudInfo("No notes to export, open a chart first.");
		return;
	}

	Debug::logBlankLine();
	Debug::log("arrowtable = {");
	for (auto it = chart->notes->list.begin(), end = chart->notes->list.end(), last = end - 1; it != end; ++it)
	{
		string beat = String::val(it->row * BeatsPerRow, 0, 3);
		const char* fmt = (it == last) ? "{%s,%i}};\n" : "{%s,%i},";
		Debug::log(fmt, beat, it->col);
	}
	Debug::logBlankLine();
	HudNote("Note table written to log.");
}
*/

// =====================================================================================================================
// Clipboard functions.

void Editing::drawGhostNotes()
{
	for (int col = 0; col < SimfileConstants::MaxColumns; ++col)
	{
		auto& pnote = state->placingNotes[col];
		pnote.endRow = GetCurrentPlacementRow();
		if (IsActive(state->placingNotes[col]))
		{
			Notefield::drawGhostNote(col, PlacingNoteToNote(pnote));
		}
	}
}

} // namespace AV