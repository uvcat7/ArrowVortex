#include <Editor/Editing.h>

#include <algorithm>
#include <set>
#include <cmath>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Xmr.h>

#include <System/System.h>
#include <System/Debug.h>

#include <Simfile/SegmentGroup.h>

#include <Editor/Music.h>
#include <Editor/Selection.h>
#include <Editor/View.h>
#include <Editor/Notefield.h>
#include <Editor/History.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/Menubar.h>

#include <Managers/MetadataMan.h>
#include <Managers/StyleMan.h>
#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Core/Draw.h>

namespace Vortex {

enum TweakMode { TWEAK_NONE, TWEAK_BPM, TWEAK_OFS };

enum PlaceMode { PLACE_NONE, PLACE_NEW, PLACE_AFTER_REMOVE};

struct PlacingNote { int player, startRow, endRow; PlaceMode mode; };

static int KeyToCol(Key::Code code)
{
	return (code == Key::DIGIT_0) ? 9 : (code - Key::DIGIT_1);
}

static bool IsActive(const PlacingNote& n)
{
	return (n.mode == PLACE_NEW || (n.mode == PLACE_AFTER_REMOVE && n.startRow != n.endRow));
}

static Note PlacingNoteToNote(const PlacingNote& pnote, int col)
{
	Note out;
	out.col = col;
	out.row = min(pnote.startRow, pnote.endRow);
	out.endrow = max(pnote.startRow, pnote.endRow);
	out.player = pnote.player;
	out.type = NOTE_STEP_OR_HOLD;
	return out;
}

int noteKeysHeld = 0;

// ================================================================================================
// EditingImpl :: member data.

struct EditingImpl : public Editing {

int myCurPlayer;
PlacingNote myPlacingNotes[SIM_MAX_COLUMNS];

bool myUseJumpToNextNote;
bool myUseUndoRedoJump;
bool myUseTimeBasedCopy;
VisualSyncAnchor myVisualSyncAnchor;

// ================================================================================================
// EditingImpl :: constructor and destructor.

~EditingImpl()
{
}

EditingImpl()
{
	myCurPlayer = 0;
	for(PlacingNote& n : myPlacingNotes)
	{
		n.mode = PLACE_NONE;
	}

	myUseJumpToNextNote = false;
	myUseUndoRedoJump = true;
	myUseTimeBasedCopy = false;
	myVisualSyncAnchor = VisualSyncAnchor::CURSOR;
}

// ================================================================================================
// StatusbarImpl :: load / save settings.

void loadSettings(XmrNode& settings)
{
	XmrNode* editing = settings.child("editing");
	if(editing)
	{
		editing->get("useJumpToNextNote", &myUseJumpToNextNote);
		editing->get("useUndoRedoJumps", &myUseUndoRedoJump);
		editing->get("useTimeBasedCopy", &myUseTimeBasedCopy);
	}
}

void saveSettings(XmrNode& settings)
{
	XmrNode* editing = settings.addChild("editing");

	editing->addAttrib("useJumpToNextNote", myUseJumpToNextNote);
	editing->addAttrib("useUndoRedoJumps", myUseUndoRedoJump);
	editing->addAttrib("useTimeBasedCopy", myUseTimeBasedCopy);
}

// ================================================================================================
// EditingImpl :: event handling.

void onKeyPress(KeyPress& evt) override
{
	if(evt.handled) return;
	Key::Code kc = evt.key;

	// Copy/pasting.
	if(evt.keyflags & Keyflag::CTRL)
	{
		if(kc == Key::X)
		{
			copySelectionToClipboard(true);
			evt.handled = true;
		}
		else if(kc == Key::C)
		{
			copySelectionToClipboard(false);
			evt.handled = true;
		}
		else if(kc == Key::V)
		{
			pasteFromClipboard();
			evt.handled = true;
		}
	}

	// Deleting notes.
	if(kc == Key::DELETE)
	{
		deleteSelection();
		evt.handled = true;
	}

	// Placing notes.
	if(gChart->isOpen() && kc >= Key::DIGIT_0 && kc <= Key::DIGIT_9 && !evt.repeated)
	{
		int col = KeyToCol(kc);
		int row = gView->snapRow(gView->getCursorRow(), View::SNAP_CLOSEST);
		if(evt.keyflags & Keyflag::ALT) col += gStyle->getNumCols() / 2;
		if(col >= 0 && col < gStyle->getNumCols())
		{
			noteKeysHeld++;
			if (noteKeysHeld > 10) noteKeysHeld = 0;
			if (gMusic->isPaused())
			{
				gView->setCursorRow(row);
			}
			NoteEdit edit;
			auto note = gNotes->getNoteAt(row, col);
			if(note)
			{
				if(gMusic->isPaused())
				{
					myPlacingNotes[col] = {myCurPlayer, row, row, PLACE_AFTER_REMOVE};
				}
				edit.rem.append(CompressNote(*note));
				gNotes->modify(edit, false);
			}
			else
			{
				edit.add.append({row, row, (uint)col, (uint)myCurPlayer, NOTE_STEP_OR_HOLD});
				if(evt.keyflags & Keyflag::SHIFT)
				{
					edit.add.begin()->type = NOTE_MINE;
					gNotes->modify(edit, false);
				}
				else if(gMusic->isPaused())
				{
					myPlacingNotes[col] = {myCurPlayer, row, row, PLACE_NEW};
				}
				else
				{
					gNotes->modify(edit, false);
				}
			}
		}

		evt.handled = true;
		return;
	}

	// Finish tweaking.
	int mode = gTempo->getTweakMode();
	if((kc == Key::RETURN || kc == Key::ESCAPE) && mode && !evt.handled)
	{
		gTempo->stopTweaking(kc == Key::RETURN);
		evt.handled = true;
	}
}

void turnIntoTriplets()
{
	/* TODO?
	// h/h/h/k/k/h/h/h/k/k/

	int delta[5] = {24, 24, 24, 12, 12};
	int deltaIdx = 0;

	Vector<RowCol> rem;
	NoteList add = gSelection->getSelectedNotes();
	for(auto& note : add) rem.push_back({note.row, (int)note.col});

	if(add.empty())
	{
		HudNote("There are no notes selected.");
		return;
	}

	// Scale the rows of the selected notes.
	int row = add[0].row;
	for(Note& n : add)
	{
		n.row = n.endrow = row;
		row += delta[deltaIdx];
		deltaIdx = (deltaIdx + 1) % 5;
	}

	// If we are using row selection, we remove all expanded notes outside the selection range.
	auto region = gSelection->getSelectedRegion();
	if(region.beginRow != region.endRow)
	{
		int i = 0;
		while(i != add.size() && (int)add[i].row <= region.endRow) ++i;
		add.erase(i, add.size());
	}

	// Perform the scale operation.
	static const NotesMan::EditDescription tag = {"Expanded %1 note.", "Expanded %1 notes."};
	gNotes->modify(add, rem, NotesMan::OVERWRITE_REGION, &tag);

	// Reselect the scaled notes.
	if(gSelection->isNotes())
	{
		gNotes->select(SELECT_SET, add.begin(), add.size());
	}*/
}

void onKeyRelease(KeyRelease& evt) override
{
	if (evt.handled) return;
	if(gChart->isOpen() && evt.key >= Key::DIGIT_0 && evt.key <= Key::DIGIT_9)
	{
		// Finish placing notes.
		int row = gView->snapRow(gView->getCursorRow(), View::SNAP_CLOSEST);
		int col = KeyToCol(evt.key);
		if(evt.keyflags & Keyflag::ALT) col += gStyle->getNumCols() / 2;
		if(col >= 0 && col < gStyle->getNumCols())
		{
			noteKeysHeld--;
			if (noteKeysHeld < 0) noteKeysHeld = 0;
			finishNotePlacement(col);
			// Don't advance when we're stepping jumps
			if (hasJumpToNextNote() && noteKeysHeld == 0 && gView->getSnapType() != ST_NONE)
			{
				gView->setCursorRow(gView->snapRow(gView->getCursorRow(), gView->hasReverseScroll() ? View::SNAP_UP : View::SNAP_DOWN));
			}
		}
	}
}

void onMousePress(MousePress& evt) override
{
	// Finish tweaking.
	int mode = gTempo->getTweakMode();
	if((evt.button == Mouse::LMB || evt.button == Mouse::RMB) && mode && evt.unhandled())
	{
		gTempo->stopTweaking(evt.button == Mouse::LMB);
		evt.setHandled();
	}
}

void onMouseRelease(MouseRelease& evt) override
{
}

void onMouseScroll(MouseScroll& evt) override
{
	int m = gTempo->getTweakMode();
	if(m && (evt.keyflags & (Keyflag::SHIFT | Keyflag::ALT)) && !evt.handled)
	{
		double deltas[] = {0, 0.1, 1.0, 0.1};
		double d = evt.up ? (-deltas[m]) : deltas[m];
		if(evt.keyflags & Keyflag::ALT) d *= 0.01;

		double r = fabs(d);
		double v = floor((gTempo->getTweakValue() + d) / r + 0.5) * r;
		gTempo->setTweakValue(v);

		evt.handled = true;
	}
}

void onChanges(int changes)
{
	if(changes & VCM_CHART_CHANGED)
	{
		if(myCurPlayer >= gStyle->getNumPlayers())
		{
			myCurPlayer = 0;
		}
	}
}

// ================================================================================================
// EditingImpl :: member functions.

void finishNotePlacement(int col)
{
	auto& pnote = myPlacingNotes[col];
	pnote.endRow = gView->getCursorRow();
	if(gChart->isOpen() && IsActive(pnote))
	{
		Note note = PlacingNoteToNote(pnote, col);

		// If the note is a hold extending upwards into another hold, merge them.
		if(pnote.endRow < pnote.startRow)
		{
			auto hold = gNotes->getNoteIntersecting(note.row, col);
			if(hold && hold->endrow > hold->row)
			{
				if((int)note.row > hold->row && (int)note.row <= hold->endrow && (int)note.endrow > hold->endrow)
				{
					note.row = (uint)hold->row;
				}
			}
		}

		NoteEdit edit;
		edit.add.append(note);
		gNotes->modify(edit, false);

	}
	pnote.mode = PLACE_NONE;
}

void deleteSelection()
{
	if(gSelection->getType() == Selection::TEMPO)
	{
		gTempo->removeSelectedSegments();
	}
	else
	{
		gNotes->removeSelectedNotes();
	}
}

void changeHoldsToRolls()
{
	if(gChart->isClosed()) return;

	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);

	int numHolds = 0, numRolls = 0;
	for(auto& n : edit.add)
	{
		if(n.endrow > n.row)
		{
			if(n.type == NOTE_ROLL)
			{
				++numRolls;
				n.type = NOTE_STEP_OR_HOLD;
			}
			else
			{
				++numHolds;
				n.type = NOTE_ROLL;
			}
		}
	}

	static const NotesMan::EditDescription descs[3] = {
		{"Converted %1 hold to roll.", "Converted %1 holds to rolls."},
		{"Converted %1 roll to hold.", "Converted %1 rolls to holds."},
		{"Converted %1 hold/roll.", "Converted %1 holds/rolls."},
	};
	if(numHolds > 0 || numRolls > 0)
	{
		auto* desc = descs + (numRolls ? (numHolds ? 2 : 1) : 0);
		gNotes->modify(edit, true, desc);
	}
	else
	{
		HudNote("There are no holds/rolls selected.");
	}
}

void changeHoldsToSteps()
{
	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);

	int numHolds = 0;
	for(auto& n : edit.add)
	{
		if(n.endrow > n.row)
		{
			n.type = NOTE_STEP_OR_HOLD;
			n.endrow = n.row;
			++numHolds;
		}
	}
	if(numHolds > 0)
	{
		static const NotesMan::EditDescription desc = {
			"Converted %1 hold to step.", "Converted %1 holds to steps."
		};
		gNotes->modify(edit, false, &desc);
	}
	else
	{
		HudNote("There are no holds/rolls selected.");
	}
}

void changeNotesToMines()
{
	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);

	int numNotes = 0;
	for(auto& n : edit.add)
	{
		if(n.type != NOTE_MINE)
		{
			n.type = NOTE_MINE;
			n.endrow = n.row;
			++numNotes;
		}
	}
	if(numNotes > 0)
	{
		static const NotesMan::EditDescription desc = {
			"Converted %1 note to mine.", "Converted %1 notes to mines."
		};
		gNotes->modify(edit, false, &desc);
	}
	else
	{
		HudNote("There are no notes selected.");
	}
}

void changePlayerNumber()
{
	int numPlayers = gStyle->getNumPlayers();

	// Check if the current style actually supports more than 1 player.
	if(numPlayers <= 1)
	{
		HudNote("%s only has one player.", gStyle->get()->name.str());
		return;
	}

	// If we do not have a note selection, we switch player for note placement instead.
	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);
	if(edit.add.empty())
	{
		int newPlayer = (myCurPlayer + 1) % numPlayers;
		if(newPlayer != myCurPlayer)
		{
			HudInfo("Switched to player %i", newPlayer + 1);
			myCurPlayer = newPlayer;
		}
		return;
	}

	// Switch all notes in the selection to the next player.
	int curPlayer = edit.add.begin()->player;
	int newPlayer = (curPlayer + 1) % numPlayers;
	bool samePlayer = true;
	for(auto& n : edit.add)
	{
		samePlayer &= (n.player == curPlayer);
		n.player = (n.player + 1) % numPlayers;
	}
	
	// We do have a selection, switch players for all selected notes.
	static const NotesMan::EditDescription descs[4] = {
		{"Converted %1 note to P1.", "Converted %1 notes to P1."},
		{"Converted %1 note to P2.", "Converted %1 notes to P2."},
		{"Converted %1 note to P3.", "Converted %1 notes to P3."},
		{"Switched player for %1 note.", "Switched player for %1 notes."},
	};
	auto* desc = descs + (samePlayer ? min(newPlayer, 3) : 3);
	gNotes->modify(edit, false, desc);
}

template <typename T>
static T readFromBuffer(Vector<uchar>& buffer, int& pos)
{
	if(pos + (int)sizeof(T) <= buffer.size())
	{
		pos += sizeof(T);
		return *(T*)(buffer.data() + pos - sizeof(T));
	}
	pos = buffer.size();
	return 0;
}

void pasteNotePatterns()
{
	/* TODO?
	NoteList out = gSelection->getSelectedNotes();

	Vector<uchar> buffer = GetClipboardData("notes");
	if(buffer.empty()) return;

	int readPos = 0;
	bool timeBased = readFromBuffer<bool>(buffer, readPos);

	// Check if there is at least one note in the decoded data.
	int numNotes = readFromBuffer<int>(buffer, readPos);
	if(numNotes <= 0)
	{
		HudWarning("Clipboard has invalid note data, bad header.");
		return;
	}

	// Check if the size of the decoded data makes sense.
	int headerSize = sizeof(bool) + sizeof(int);
	int notesSize = numNotes * sizeof(Note);
	if(buffer.size() != headerSize + notesSize)
	{
		HudWarning("Clipboard has invalid note data, bad size.");
		return;
	}

	// Offset the rows of the notes so they start at the cursor position.
	auto notes = (Note*)(buffer.data() + headerSize);
	int offset = notes[0].row - gView->getCursorRow();

	// Read the notes from the buffer.
	for(int i = 0; i < numNotes && i < out.size(); ++i)
	{
		out[i].col = notes[i].col;
	}

	static const NotesMan::EditDescription tag = {"Pasted %1 note", "Pasted %1 notes"};
	gNotes->add(out, NotesMan::OVERWRITE_ROWS, &tag);*/
}

static void switchColumns(NoteList& notes, const int* table)
{
	if(table)
	{
		for(auto& note : notes)
		{
			note.col = table[note.col];
		}
	}
}

void mirrorNotes(MirrorType type)
{
	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);

	if(edit.add.empty())
	{
		HudNote("There are no notes selected.");
		return;
	}
	edit.rem = edit.add;

	// Mirror the selected notes.
	auto style = gStyle->get();
	switch(type)
	{
	case MIRROR_H:
		switchColumns(edit.add, style->mirrorTableH); break;
	case MIRROR_V:
		switchColumns(edit.add, style->mirrorTableV); break;
	case MIRROR_HV:
		switchColumns(edit.add, style->mirrorTableH);
		switchColumns(edit.add, style->mirrorTableV); break;
	};

	// Resort the notes per row.
	auto ptr = edit.add.begin();
	for(int i = 0, size = edit.add.size(); i < size;)
	{
		int row = (ptr + i)->row, begin = i;
		while(i != size && (ptr + i)->row == row) ++i;
		std::sort(ptr + begin, ptr + i, LessThanRowCol<Note, Note>);
	}

	// Perform the mirror operation.
	static const NotesMan::EditDescription descs[3] = {
		{"Horizontally mirrored %1 note.", "Horizontally mirrored %1 notes."},
		{"Vertically mirrored %1 note.", "Vertically mirrored %1 notes."},
		{"Fully mirrored %1 note.", "Fully mirrored %1 notes."},
	};
	gNotes->modify(edit, false, descs + type);

	// Reselect the mirrored notes.
	if(gSelection->getType() == Selection::NOTES)
	{
		gNotes->select(SELECT_SET, edit.add.begin(), edit.rem.size());
	}
}

void scaleNotes(int numerator, int denominator)
{
	NoteEdit edit;
	gSelection->getSelectedNotes(edit.add);
	edit.rem = edit.add;

	if(edit.add.empty())
	{
		HudNote("There are no notes selected.");
		return;
	}

	// Scale the rows of the selected notes.
	int top = edit.add.begin()->row;
	for(Note& n : edit.add)
	{
		n.row    = (n.row    - top) * numerator / denominator + top;
		n.endrow = (n.endrow - top) * numerator / denominator + top;
	}

	// If we are using region selection, we remove all expanded notes outside the selection range.
	if(gSelection->getType() == Selection::REGION)
	{
		auto region = gSelection->getSelectedRegion();
		for(auto it = edit.add.begin(), end = edit.add.end(); it != end; ++it)
		{
			if(it->row > region.endRow) it->row = -1;
		}
		edit.add.cleanup();
	}

	// Perform the scale operation.
	static const NotesMan::EditDescription tExp = {"Expanded %1 note.", "Expanded %1 notes."};
	static const NotesMan::EditDescription tCom = {"Compressed %1 note.", "Compressed %1 notes."};
	const NotesMan::EditDescription* desc = (numerator > denominator) ? &tExp : &tCom;
	gNotes->modify(edit, true, desc);

	// Reselect the scaled notes.
	if(gSelection->getType() == Selection::NOTES)
	{
		gNotes->select(SELECT_SET, edit.add.begin(), edit.add.size());
	}
}

void insertRows(int row, int numRows, bool curChartOnly)
{
	if(gSimfile->isOpen())
	{
		gHistory->startChain();
		gTempo->insertRows(row, numRows, curChartOnly);
		gNotes->insertRows(row, numRows, curChartOnly);
		gHistory->finishChain((numRows > 0) ? "Insert beats" : "Delete beats");
	}
}

int getAnchorRow() {
	Vortex::vec2i mouse_pos = gSystem->getMousePos();
	Vortex::ChartOffset chart_offset = gView->yToOffset(mouse_pos.y);

	switch (this->myVisualSyncAnchor) {
	case VisualSyncAnchor::RECEPTORS:
		return gView->getCursorRow();
	case VisualSyncAnchor::CURSOR:
		return gView->snapRow(gView->offsetToRow(chart_offset), Vortex::View::SnapDir::SNAP_CLOSEST);
	default:
		HudError("Unknown anchor row type");
		return -1;
	}
}

void injectBoundingBpmChange() {
	if (gSimfile == nullptr || !gView->isTimeBased()) {
		return;
	}

	int anchor_row = this->getAnchorRow();

	gTempo->injectBoundingBpmChange(anchor_row);
}

void shiftAnchorRowToMousePosition(bool is_destructive) {
	if (gSimfile == nullptr || !gView->isTimeBased()) {
		return;
	}
	Vortex::vec2i mouse_pos = gSystem->getMousePos();
	Vortex::ChartOffset chart_offset = gView->yToOffset(mouse_pos.y);

	double target_time = gView->offsetToTime(chart_offset);
	int anchor_row = this->getAnchorRow();

	if (is_destructive) {
		gTempo->destructiveShiftRowToTime(anchor_row, target_time);
	}
	else {
		gTempo->nonDestructiveShiftRowToTime(anchor_row, target_time);
	}
}

/*
void streamToTriplets()
{
	NoteList rem = gSelection->getSelectedNotes(), add;

	// Use the rows of every 3 out of 4 notes.
	for(int colI = 0, rowI = 0; rowI < rem.size(); ++colI, ++rowI)
	{
		Note n = rem[colI];
		n.row = n.endrow = rem[rowI].row;
		add.push_back(n);
		if(colI % 3 == 2) ++rowI;
	}

	// If we are using row selection, we remove all expanded notes outside the selection range.
	vec2i area = gSelection->getSelectedArea();
	if(area.x != area.y)
	{
		int i = 0;
		while(i != add.size() && add[i].row <= area.y) ++i;
		add.erase(i, add.size());
	}

	static const NotesMan::EditDescription tag = {"Expanded %1 note.", "Expanded %1 notes."};
	gChart->modify(add, rem, NotesMan::OVERWRITE_REGION, &tag);
	if(gSelection->isNotes()) gSelection->selectNotes(SELECT_ADD, add);
}*/

// ================================================================================================
// EditingImpl :: routine functions.

void convertRoutineToCouples()
{
	const char* title = "Convert Routine to ITG Couples";

	// Find all routine charts.
	Vector<const Chart*> charts;
	for(int i = 0; i < gSimfile->getNumCharts(); ++i)
	{
		auto chart = gSimfile->getChart(i);
		if(chart->style->id == "dance-routine")
		{
			charts.push_back(chart);
		}
	}
	if(charts.empty())
	{
		HudNote("There are no routine charts to convert.");
		return;
	}

	// Make a list of all rows that contain P2 notes.
	std::set<int> p2rows;
	for(auto& c : charts)
	{
		for(auto& n : c->notes)
		{
			if(n.player != 0)
			{
				p2rows.insert(n.row);
			}
		}
	}
	if(p2rows.empty())
	{
		HudNote("There are no player 2 notes, nothing was converted.");
		return;
	}

	gHistory->startChain();

	// Find the game mode dance double.
	auto style = gStyle->findStyle("dance-double", 8, 1);

	// Bump all player 2 notes down one row.
	Chart* newChart = nullptr;
	for(auto& c : charts)
	{
		NoteEdit edit;
		for(auto& note : c->notes)
		{
			Note n = note;
			if(note.player != 0)
			{
				++n.row, ++n.endrow;
				n.player = 0;
			}
			edit.add.append(n);
		}
		std::stable_sort(edit.add.begin(), edit.add.end(), LessThanRowCol<Note, Note>);

		gSimfile->addChart(style, c->artist, c->difficulty, c->meter);
		gNotes->modify(edit, true, nullptr);
	}

	// Apply negative BPM skips.
	auto segments = gTempo->getSegments();
	for(int row : p2rows)
	{
		double base = segments->getRow<Stop>(row).seconds;
		double len = 60.0 / (gTempo->getBpm(row) * ROWS_PER_BEAT);

		SegmentEdit edit;
		edit.add.append(Stop(row, -len));
		edit.add.append(Stop(row + 1, base + len));
		gTempo->modify(edit);
	}

	gHistory->finishChain(title);
}

void convertCouplesToRoutine()
{
	auto segments = gTempo->getSegments();

	// Find all doubles charts.
	Vector<const Chart*> charts;
	for(int i = 0; i < gSimfile->getNumCharts(); ++i)
	{
		auto chart = gSimfile->getChart(i);
		if(chart->style->id == "dance-double")
		{
			charts.push_back(chart);
		}
	}
	if(charts.empty())
	{
		HudNote("There are no doubles charts to convert.");
		return;
	}

	// Make a list of all rows that have negative BPM skips.
	auto it = segments->begin<Stop>();
	auto end = segments->end<Stop>();

	std::set<int> p2rows;
	int prevRow = 0;
	double prevStop = 0.0;
	for(; it != end; ++it)
	{
		if(prevStop < 0 && it->seconds > 0 && it->row == prevRow + 1)
		{
			p2rows.insert(prevRow);
		}
		prevStop = it->seconds;
		prevRow = it->row;
	}
	if(p2rows.empty())
	{
		HudNote("There are no player 2 notes, nothing was converted.");
		return;
	}

	gHistory->startChain();

	// Find the game mode dance routine.
	auto danceRoutine = gStyle->findStyle("dance-routine", 8, 2);
	if(!danceRoutine)
	{
		HudError("Could not find the dance-routine style.");
		return;
	}

	// Bump all player 2 notes down one row.
	Chart* newChart = nullptr;
	for(auto& c : charts)
	{
		NoteEdit edit;
		for(auto& note : c->notes)
		{
			Note n = note;
			if(p2rows.find(n.row - 1) != p2rows.end())
			{
				n.player = 1;
				--n.row, --n.endrow;
			}
			edit.add.append(n);
		}
		std::stable_sort(edit.add.begin(), edit.add.end(), LessThanRowCol<Note, Note>);

		gSimfile->addChart(danceRoutine, c->artist, c->difficulty, c->meter);
		gNotes->modify(edit, true, nullptr);
	}

	// Remove negative BPM skips.
	for(int row : p2rows)
	{
		double len = segments->getRow<Stop>(row).seconds;
		len += segments->getRow<Stop>(row + 1).seconds;

		SegmentEdit edit;
		edit.add.append(Stop(row, 0));
		edit.add.append(Stop(row + 1, len));
		gTempo->modify(edit);
	}

	gHistory->finishChain("Convert ITG Couples to Routine");
}

void exportNotesAsLuaTable()
{
	const Chart* chart = gChart->get();
	if(!chart)
	{
		HudInfo("No notes to export, open a chart first.");
		return;
	}

	Debug::logBlankLine();
	Debug::log("arrowtable = {");
	for(auto it = chart->notes.begin(), end = chart->notes.end(), last = end - 1; it != end; ++it)
	{
		String beat = Str::val(it->row * BEATS_PER_ROW, 0, 3);
		const char* fmt = (it == last) ? "{%s,%i}};\n" : "{%s,%i},";
		Debug::log(fmt, beat, it->col);
	}
	Debug::logBlankLine();
	HudNote("Note table written to log.");
}

// ================================================================================================
// EditingImpl :: clipboard functions.

void copySelectionToClipboard(bool remove)
{
	if(gSelection->getType() == Selection::TEMPO)
	{
		gTempo->copyToClipboard();
		if(remove) gTempo->removeSelectedSegments();
	}
	else
	{
		gNotes->copyToClipboard(myUseTimeBasedCopy);
		if(remove) gNotes->removeSelectedNotes();
	}
}

void pasteFromClipboard()
{
	if(gChart->isOpen() && HasClipboardData(NotesMan::clipboardTag))
	{
		gNotes->pasteFromClipboard();
	}
	else if(HasClipboardData(TempoMan::clipboardTag))
	{
		gTempo->pasteFromClipboard();
	}
}

void drawGhostNotes()
{
	for(int col = 0; col < SIM_MAX_COLUMNS; ++col)
	{
		auto& pnote = myPlacingNotes[col];
		pnote.endRow = gView->getCursorRow();
		if(IsActive(myPlacingNotes[col]))
		{
			gNotefield->drawGhostNote(PlacingNoteToNote(pnote, col));
		}
	}
}

void toggleJumpToNextNote()
{
	myUseJumpToNextNote = !myUseJumpToNextNote;
	gMenubar->update(Menubar::USE_JUMP_TO_NEXT_NOTE);
}

bool hasJumpToNextNote()
{
	return myUseJumpToNextNote;
}

void toggleUndoRedoJump()
{
	myUseUndoRedoJump = !myUseUndoRedoJump;
	gMenubar->update(Menubar::USE_UNDO_REDO_JUMP);
}

bool hasUndoRedoJump()
{
	return myUseUndoRedoJump;
}

void toggleTimeBasedCopy()
{
	myUseTimeBasedCopy = !myUseTimeBasedCopy;
	gMenubar->update(Menubar::USE_TIME_BASED_COPY);
}

bool hasTimeBasedCopy()
{
	return myUseTimeBasedCopy;
}

void setVisualSyncAnchor(VisualSyncAnchor anchor) {
	myVisualSyncAnchor = anchor;
	switch (myVisualSyncAnchor) {
	case VisualSyncAnchor::RECEPTORS:
		HudInfo("Visual sync will use current row");
		break;
	case VisualSyncAnchor::CURSOR:
		HudInfo("Visual sync will use mouse cursor's closest row of selected snap");
		break;
	}
	gMenubar->update(Menubar::VISUAL_SYNC_ANCHOR);
}

VisualSyncAnchor getVisualSyncMode() {
	return myVisualSyncAnchor;
}

}; // EditingImpl

// ================================================================================================
// Editing API.

Editing* gEditing = nullptr;

void Editing::create(XmrNode& settings)
{
	gEditing = new EditingImpl();
	((EditingImpl*)gEditing)->loadSettings(settings);
}

void Editing::destroy()
{
	delete (EditingImpl*)gEditing;
	gEditing = nullptr;
}

}; // namespace Vortex
