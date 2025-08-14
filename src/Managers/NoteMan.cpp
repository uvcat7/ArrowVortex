#include <Managers/NoteMan.h>

#include <algorithm>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/VectorUtils.h>

#include <Managers/TempoMan.h>
#include <Managers/MetadataMan.h>
#include <Managers/ChartMan.h>
#include <Managers/StyleMan.h>
#include <Managers/SimfileMan.h>
#include <Simfile/Parsing.h>
#include <Simfile/TimingData.h>

#include <Editor/Editor.h>
#include <Editor/History.h>
#include <Editor/View.h>
#include <Editor/Common.h>
#include <Editor/Selection.h>
#include <Editor/Editing.h>

namespace Vortex {

#define NOTE_MAN ((NotesManImpl*)gNotes)

const char* NotesMan::clipboardTag = "notes";

struct NotesManImpl : public NotesMan {

// ================================================================================================
// NotesManImpl :: member data.

Vector<ExpandedNote> myNotes;

int myNumSteps, myNumJumps;
int myNumHolds, myNumRolls;
int myNumMines, myNumWarps;

Simfile* mySimfile;
Chart* myChart;

History::EditId myApplyAddNoteId;
History::EditId myApplyRemNoteId;
History::EditId myApplyChangeNotesId;
History::EditId myApplyInsertRowsId;

// ================================================================================================
// NotesManImpl :: constructor and destructor.

~NotesManImpl()
{
}

NotesManImpl()
	: myNumSteps(0)
	, myNumJumps(0)
	, myNumHolds(0)
	, myNumRolls(0)
	, myNumMines(0)
	, myNumWarps(0)
{
	myChart = nullptr;

	myApplyAddNoteId     = gHistory->addCallback(ApplyAddNote);
	myApplyRemNoteId     = gHistory->addCallback(ApplyRemoveNote);
	myApplyChangeNotesId = gHistory->addCallback(ApplyChangeNotes);
	myApplyInsertRowsId  = gHistory->addCallback(ApplyInsertRows);
}

// ================================================================================================
// NotesManImpl :: update functions.

void myUpdateNotes()
{
	int numCols = gStyle->getNumCols();
	int numPlayers = gStyle->getNumPlayers();
	myChart->notes.sanitize(myChart);

	myNotes.resize(myChart->notes.size());
	auto it = myNotes.begin();
	for(auto& note : myChart->notes)
	{
		it->row = note.row;
		it->col = note.col;
		it->endrow = note.endrow;
		it->isMine = note.type == NOTE_MINE;
		it->isRoll = note.type == NOTE_ROLL;
		it->isSelected = 0;
		it->type = note.type;
		it->player = note.player;
		it->quant = note.quant;
		++it;
	}

	myUpdateNoteTimes();
	myUpdateWarpedNotes();
	myUpdateFakedNotes();
	myUpdateNoteStats();
	myUpdateCheckQuants();
}

void myUpdateCheckQuants()
{
	for (auto& note : myNotes)
	{
		if (note.quant < 0 || note.quant > 192)
		{
			HudError("Missing quant at %d, value %d", note.row, note.quant);
			note.quant = 192;
		}
	}
}

void myUpdateNoteTimes()
{
	TempoTimeTracker tracker;
	for(auto& note : myNotes)
	{
		note.time = tracker.advance(note.row);
		if(note.endrow == note.row)
		{
			note.endtime = note.time;
		}
		else
		{
			note.endtime = gTempo->rowToTime(note.endrow);
		}
	}
}

void myUpdateWarpedNotes()
{
	uint32_t insideWarp = 0;
	auto note = myNotes.begin(), noteEnd = myNotes.end();
	auto& events = gTempo->getTimingData().events;
	auto it = events.begin(), end = events.end();
	for(; it != end; ++it)
	{
		if(insideWarp)
		{
			for(; note != noteEnd && note->row < it->row; ++note)
			{
				note->isWarped = 1;
			}
		}
		else
		{
			for(; note != noteEnd && note->row <= it->row; ++note)
			{
				note->isWarped = 0;
			}
		}
		insideWarp = (it->spr == 0.0);
	}
	for(; note != noteEnd; ++note)
	{
		note->isWarped = 0;
	}
}

void myUpdateFakedNotes()
{
	auto note = myNotes.begin(), noteEnd = myNotes.end();
	auto& events = gTempo->getTimingData().fakes;
	auto it = events.begin(), end = events.end();

	for (; it != end; ++it)
	{
		for (; note != noteEnd; ++note)
		{
			note->isFake = note->type == NOTE_FAKE || note->row >= it->row && note->row <= it->row + it->length;
		}
	}
	for (; note != noteEnd; ++note)
	{
		note->isFake = note->type == NOTE_FAKE;
	}
}

void myUpdateNoteStats()
{
	myNumSteps = 0, myNumJumps = 0;
	myNumHolds = 0, myNumRolls = 0;
	myNumMines = 0, myNumWarps = 0;

	int lastRow = -1;
	for(auto& note : myNotes)
	{
		if(!note.isMine)
		{
			int isHoldOrRoll = note.endrow > note.row;
			myNumRolls += isHoldOrRoll & note.isRoll;
			myNumHolds += isHoldOrRoll & (note.isRoll ^ 1);
			myNumJumps += lastRow == note.row;
			lastRow = note.row;
			++myNumSteps;
		}
		else
		{
			++myNumMines;
		}
		myNumWarps += note.isWarped;
	}
}

void update(Simfile* simfile, Chart* chart)
{
	mySimfile = simfile;
	myChart = chart;

	if(myChart)
	{
		myUpdateNotes();
	}
	else
	{
		myNotes.release();
		myUpdateNoteStats();
	}

	gEditor->reportChanges(VCM_NOTES_CHANGED);
}

void updateTempo()
{
	myUpdateNoteTimes();
	myUpdateWarpedNotes();
	myUpdateFakedNotes();
}

// ================================================================================================
// NotesManImpl :: editing helper functions.

static std::string GetNoteName(const Note& note)
{
	switch(note.type)
	{
	case NOTE_STEP_OR_HOLD:
		return (note.row == note.endrow) ? "step" : "hold";
	case NOTE_MINE:
		return "mine";
	case NOTE_ROLL:
		return "roll";
	case NOTE_FAKE:
		return "fake";
	case NOTE_LIFT:
		return "lift";
	};
	return "note";
}

void myApplyNotes(Chart* chart, const NoteList& add, const NoteList& rem, bool firstTime)
{
	// Remove notes before inserting rows.
	chart->notes.remove(rem);
	chart->notes.insert(add);
	chart->notes.sanitize(chart);

	// Jump to the position of the first note that changed.
	bool updated = false;
	if(!firstTime && gEditing->hasUndoRedoJump())
	{
		if(myChart != chart)
		{
			gSimfile->openChart(chart);
			updated = true;
		}

		int row = gSimfile->getEndRow();
		if(rem.size()) row = min(row, rem.begin()->row);
		if(add.size()) row = min(row, add.begin()->row);

		int h = gView->getHeight();
		int y = gView->rowToY(row);
		if(y < 16 || y > h - 16)
		{
			gView->setCursorRow(row);
		}
	}

	if(myChart == chart)
	{
		if(!firstTime) select(SELECT_SET, add.begin(), add.size());

		if(!updated) myUpdateNotes();

		gEditor->reportChanges(VCM_NOTES_CHANGED);
	}
}

// ================================================================================================
// NotesManImpl :: apply add note.

void myQueueAddNote(Note note)
{
	WriteStream stream;
	EncodeNote(stream, note);
	gHistory->addEntry(myApplyAddNoteId, stream.data(), stream.size(), myChart);
}

static std::string ApplyAddNote(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	std::string msg;
	Note note;
	DecodeNote(in, note);
	if(in.success())
	{
		NoteList add, dummy;
		add.append(note);
		if(undo)
		{
			msg = "Removed ";
			NOTE_MAN->myApplyNotes(bound.chart, dummy, add, false);
		}
		else
		{
			msg = "Added ";
			NOTE_MAN->myApplyNotes(bound.chart, add, dummy, !redo);
		}
		msg = msg + GetNoteName(note);
	}
	return msg;
}

// ================================================================================================
// NotesManImpl :: apply remove note.

void myQueueRemoveNote(Note note)
{
	WriteStream stream;
	EncodeNote(stream, note);
	gHistory->addEntry(myApplyRemNoteId, stream.data(), stream.size(), myChart);
}

static std::string ApplyRemoveNote(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	std::string msg;
	Note note;
	DecodeNote(in, note);
	if(in.success())
	{
		NoteList rem, dummy;
		rem.append(note);
		if(undo)
		{
			msg = "Added ";
			NOTE_MAN->myApplyNotes(bound.chart, rem, dummy, false);
		}
		else
		{
			msg = "Removed ";
			NOTE_MAN->myApplyNotes(bound.chart, dummy, rem, !redo);
		}
		msg = msg + GetNoteName(note);
	}
	return msg;
}

// ================================================================================================
// NotesManImpl :: apply change notes.

void myQueueChangeNotes(const NoteEditResult& edit, const EditDescription* desc)
{
	WriteStream stream;
	edit.add.encode(stream, false);
	edit.rem.encode(stream, false);
	stream.write(desc);
	gHistory->addEntry(myApplyChangeNotesId, stream.data(), stream.size(), myChart);
}

static std::string ApplyChangeNotes(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	std::string msg;
	
	NoteList add, rem;

	add.decode(in, 0);
	rem.decode(in, 0);
	auto desc = in.read<const EditDescription*>();
	if(in.success())
	{
		if(desc)
		{
			int numNotes = max(add.size(), rem.size());
			const char* format = (numNotes > 1) ? desc->plural : desc->singular;
			msg = Str::fmt(format).arg(numNotes).str;
		}
		else
		{
			Vector<std::string> info;

			if(add.size() == 1)
			{
				info.push_back("Added " + GetNoteName(*add.begin()));
			}
			else if(add.size() > 1)
			{
				info.push_back(Str::fmt("Added %1 notes").arg(add.size()));
			}

			if(rem.size() == 1)
			{
				info.push_back("Removed " + GetNoteName(*rem.begin()));
			}
			else if(rem.size() > 1)
			{
				info.push_back(Str::fmt("Removed %1 notes").arg(rem.size()));
			}

			msg = Str::join(info, ", ");
		}

		if(undo)
		{
			NOTE_MAN->myApplyNotes(bound.chart, rem, add, false);
		}
		else
		{
			NOTE_MAN->myApplyNotes(bound.chart, add, rem, !redo);
		}
	}
	return msg;
}

// ================================================================================================
// NotesManImpl :: apply insert rows.

void myItemizeInsertRows(WriteStream& stream, Chart* chart, int startRow, int numRows)
{
	NoteEdit edit;
	NoteEditResult result;
	if(numRows < 0)
	{
		int endRow = startRow - numRows;
		NoteList& notes = chart->notes;
		for(auto& note : notes)
		{
			if(note.endrow >= startRow && note.row <= endRow)
			{
				edit.rem.append(note);
			}
		}
		notes.prepareEdit(edit, result, false);
	}
	stream.write(chart);
	result.rem.encode(stream, false);
}

void myQueueInsertRows(int startRow, int numRows, bool curChartOnly)
{
	if(mySimfile && mySimfile->charts.size() && numRows != 0)
	{
		WriteStream stream;
		stream.write<int>(startRow);
		stream.write<int>(numRows);

		if(curChartOnly)
		{
			if(myChart)
			{
				myItemizeInsertRows(stream, myChart, startRow, numRows);
			}
		}
		else
		{
			for(auto chart : mySimfile->charts)
			{
				myItemizeInsertRows(stream, chart, startRow, numRows);
			}
		}
		stream.write((Chart*)nullptr);
		
		gHistory->addEntry(myApplyInsertRowsId, stream.data(), stream.size());
	}	
}

void myApplyInsertRowsOffset(Chart* chart, int startRow, int numRows)
{
	for(auto& n : chart->notes)
	{
		if(n.row >= startRow) n.row += numRows;
		if(n.endrow >= startRow) n.endrow += numRows;
	}
}

std::string myApplyInsertRows(ReadStream& in, bool undo, bool redo)
{
	auto startRow = in.read<int>();
	auto numRows = in.read<int>();
	auto target = in.read<Chart*>();
	while(in.success() && target)
	{
		NoteList rem, dummy;
		rem.decode(in, 0);
		if(!in.success()) break;

		if(undo) numRows = -numRows;

		// Apply positive offsets first, to make room for note insertion.
		if(numRows > 0)
		{
			myApplyInsertRowsOffset(target, startRow, numRows);
		}

		// Then, insert or remove segments.
		if(undo)
		{
			myApplyNotes(target, rem, dummy, false);
		}
		else
		{
			myApplyNotes(target, dummy, rem, !redo);
		}

		// Apply negative offsets after the notes are removed.
		if(numRows < 0)
		{
			myApplyInsertRowsOffset(target, startRow, numRows);
		}

		if(myChart == target)
		{
			myUpdateNotes();
		}

		// Jump to start row.
		gView->setCursorRow(startRow);

		target = in.read<Chart*>();
	}
	return std::string();
}

static std::string ApplyInsertRows(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	return NOTE_MAN->myApplyInsertRows(in, undo, redo);
}

// ================================================================================================
// NotesManImpl :: selection functions.

template <typename Func>
void forAllSelectedNotes(Func f)
{
	if(gSelection->getType() == Selection::REGION)
	{
		auto region = gSelection->getSelectedRegion();
		for(auto& note : myNotes)
		{
			if(note.row >= region.beginRow && note.row <= region.endRow)
			{
				f(note);
			}
		}
	}
	else if(gSelection->getType() == Selection::NOTES)
	{
		for(auto& note : myNotes)
		{
			if(note.isSelected)
			{
				f(note);
			}
		}
	}
}

template <typename Predicate>
int performSelection(SelectModifier mod, Predicate pred)
{
	int numSelected = 0;
	auto note = myNotes.begin();
	auto end = myNotes.end();
	if(mod == SELECT_SET)
	{
		for(; note != end; ++note)
		{
			uint32_t set = pred(note);
			numSelected += set;
			note->isSelected = set;
		}
	}
	else if(mod == SELECT_ADD)
	{
		for(; note != end; ++note)
		{
			uint32_t set = pred(note);
			numSelected += set & (note->isSelected ^ 1);
			note->isSelected |= set;
		}
	}
	else if(mod == SELECT_SUB)
	{
		for(; note != end; ++note)
		{
			uint32_t set = pred(note);
			numSelected += set & note->isSelected;
			note->isSelected &= set ^ 1;
		}
	}
	gSelection->setType(Selection::NOTES);
	return numSelected;
}

void deselectAll()
{
	for(auto& note : myNotes)
	{
		note.isSelected = 0;
	}
	if(gSelection->getType() == Selection::NOTES)
	{
		gSelection->setType(Selection::NONE);
	}
}

int selectAll()
{
	for(auto& note : myNotes)
	{
		note.isSelected = 1;
	}
	if(myNotes.size())
	{
		gSelection->setType(Selection::NOTES);
	}
	return myNotes.size();
}

int selectQuant(int rowType)
{
	int numSelected = 0;
	for(auto& note : myNotes)
	{
		note.isSelected = (ToRowType(note.row) == rowType);
		numSelected += note.isSelected;
	}
	if(numSelected)
	{
		gSelection->setType(Selection::NOTES);
	}
	return numSelected;
}

int selectRows(SelectModifier mod, int firstCol, int lastCol, int firstRow, int lastRow)
{
	return performSelection(mod, [&](const ExpandedNote* note)
	{
		return (note->col >= firstCol && note->col < lastCol &&
		        note->row >= firstRow && note->row < lastRow);
	});
}

int selectTime(SelectModifier mod, int firstCol, int lastCol, double firstTime, double lastTime)
{
	gSelection->setType(Selection::NOTES);
	return performSelection(mod, [&](const ExpandedNote* note)
	{
		return (note->col >= firstCol && note->col < lastCol &&
		        note->time >= firstTime && note->time <= lastTime);
	});
}

int select(SelectModifier mod, const Vector<RowCol>& indices)
{
	auto it = indices.begin(), end = indices.end();
	return performSelection(mod, [&](const ExpandedNote* note)
	{
		while(it != end && LessThanRowCol(*it, *note)) ++it;
		if(it == end) return false;
		return !LessThanRowCol(*note, *it);
	});
}

int select(SelectModifier mod, const Note* notes, int numNotes)
{
	auto it = notes, end = notes + numNotes;
	return performSelection(mod, [&](const ExpandedNote* note)
	{
		while(it != end && LessThanRowCol(*it, *note)) ++it;
		if(it == end) return false;
		return !LessThanRowCol(*note, *it);
	});
}

int select(SelectModifier mod, Filter filter)
{
	auto first = myNotes.begin();
	auto last = myNotes.end() - 1;
	switch(filter)
	{
	case SELECT_STEPS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return !note->isMine;
		});
	case SELECT_JUMPS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			if(note->isMine)
			{
				return false;
			}
			else if(note > first && note[-1].row == note->row)
			{
				return true;
			}
			else if(note < last && note[+1].row == note->row)
			{
				return true;
			}
			return false;
		});
	case SELECT_MINES:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->isMine;
		});
	case SELECT_HOLDS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->endrow != note->row && !note->isRoll;
		});
	case SELECT_ROLLS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->endrow != note->row && note->isRoll;
		});
	case SELECT_WARPS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->isWarped;
		});
	case SELECT_FAKES:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->type == NoteType::NOTE_FAKE;
		});
	case SELECT_LIFTS:
		return performSelection(mod, [&](const ExpandedNote* note)
		{
			return note->type == NoteType::NOTE_LIFT;
		});
	};
	return 0;
}

bool noneSelected() const
{
	for(auto& note : myNotes)
	{
		if(note.isSelected) return false;
	}
	return true;
}

// ================================================================================================
// NotesManImpl :: editing functions.

void modify(const NoteEdit& edit, bool clearRegion, const EditDescription* desc)
{
	NoteEditResult result;
	myChart->notes.prepareEdit(edit, result, clearRegion);

	auto& add = result.add;
	auto& rem = result.rem;

	if(desc == nullptr && add.size() == 1 && rem.empty())
	{
		myQueueAddNote(*add.begin());
	}
	else if(desc == nullptr && rem.size() == 1 && add.empty())
	{
		myQueueRemoveNote(*rem.begin());
	}
	else if(add.size() + rem.size() > 0)
	{
		myQueueChangeNotes(result, desc);
	}
}

void removeSelectedNotes()
{
	NoteEdit edit;
	Vector<RowCol> indices;
	forAllSelectedNotes([&](const ExpandedNote& note)
	{
		edit.rem.append(CompressNote(note));
	});
	modify(edit, false, nullptr);
}

void insertRows(int startRow, int numRows, bool curChartOnly)
{
	myQueueInsertRows(startRow, numRows, curChartOnly);
}

// ================================================================================================
// NotesManImpl :: clipboard functions.

void copyToClipboard(bool timeBased)
{
	// Get the note selection.
	NoteList notes;
	int numNotes = gSelection->getSelectedNotes(notes);
	if(notes.empty()) return;

	// Encode the note data and send it to the clipboard.
	if(numNotes > 0)
	{
		WriteStream stream;
		stream.write<uint8_t>(timeBased);
		if(timeBased)
		{
			notes.encode(stream, gTempo->getTimingData(), true);
		}
		else
		{
			notes.encode(stream, true);
		}	
		SetClipboardData(clipboardTag, stream.data(), stream.size());
		HudInfo("Copied %i notes", numNotes);
	}
}

void pasteFromClipboard(bool insert)
{
	Vector<uint8_t> buffer = GetClipboardData(clipboardTag);
	ReadStream stream(buffer.data(), buffer.size());

	// Check if the note data is time-based.
	bool timeBased = (stream.read<uint8_t>() != 0);

	// Read the note data.
	NoteEdit edit;
	if(timeBased)
	{
		edit.add.decode(stream, gTempo->getTimingData(), gView->getCursorTime());
	}
	else
	{
		edit.add.decode(stream, gView->getCursorRow());
	}

	if(stream.bytesleft() > 0)
	{
		HudWarning("Clipboard has invalid note data, bad size.");
		return;
	}
	else if(!stream.success())
	{
		HudWarning("Clipboard has invalid note data, bad data.");
		return;
	}

	if(edit.add.empty()) return;

	// Perform the changes.
	static const NotesMan::EditDescription desc = {"Pasted %1 note", "Pasted %1 notes"};
	modify(edit, !insert, &desc);
	select(SELECT_SET, edit.add.begin(), edit.add.size());
}

// ================================================================================================
// NotesManImpl :: get functions

int getNumSteps() const
{
	return myNumSteps;
}

int getNumJumps() const
{
	return myNumJumps;
}

int getNumMines() const
{
	return myNumMines;
}

int getNumHolds() const
{
	return myNumHolds;
}

int getNumRolls() const
{
	return myNumRolls;
}

int getNumWarps() const
{
	return myNumWarps;
}

const ExpandedNote* begin() const
{
	return myNotes.begin();
}

const ExpandedNote* end() const
{
	return myNotes.end();
}

const ExpandedNote* getNoteAt(int row, int col) const
{
	RowCol key = {row, col};
	auto it = std::lower_bound(myNotes.begin(), myNotes.end(), key, [](const ExpandedNote& a, const RowCol& b)
	{
		return CompareRowCol(a, b) < 0;
	});
	return (it != myNotes.end() && CompareRowCol(*it, key) == 0) ? it : nullptr;
}

const ExpandedNote* getNoteIntersecting(int row, int col) const
{
	auto it = myNotes.begin(), end = myNotes.end();
	for(; it != end && it->endrow < row; ++it);
	for(; it != end && it->row <= row; ++it)
	{
		if(it->col == col && it->endrow >= row) return it;
	}
	return nullptr;
}

Vector<const ExpandedNote*> getNotesBeforeTime(double time) const
{
	Vector<const ExpandedNote*> out(gStyle->getNumCols(), nullptr);
	auto cols = out.begin();
	for(auto& n : myNotes)
	{
		if(n.time > time) break;
		if(n.isMine | n.isWarped | n.isFake) continue;
		cols[n.col] = &n;
	}
	return out;
}

bool empty() const
{
	return myNotes.empty();
}

}; // NotesManImpl

// ================================================================================================
// Chart :: create and destroy.

NotesMan* gNotes = nullptr;

void NotesMan::create()
{
	gNotes = new NotesManImpl;
}

void NotesMan::destroy()
{
	delete NOTE_MAN;
	gNotes = nullptr;
}

}; // namespace Vortex
