#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Enum.h>
#include <Core/Utils/String.h>
#include <Core/Common/Serialize.h>

#include <Core/Graphics/Color.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/NoteUtils.h>
#include <Simfile/GameMode.h>
#include <Simfile/Tempo.h>
#include <Simfile/Simfile.h>
#include <Simfile/History.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Helper functions

static const char* DifficultyNames[(int)Difficulty::Count] =
{
	"Beginner",
	"Easy",
	"Medium",
	"Hard",
	"Challenge",
	"Edit"
};

const char* GetDifficultyName(Difficulty type)
{
	return DifficultyNames[(int)type];
}

static Color sDifficultyColors[(int)Difficulty::Count] =
{
	Color(16, 222, 255, 255),
	Color(99, 220, 99, 255),
	Color(255, 228, 98, 255),
	Color(255, 98, 97, 255),
	Color(109, 142, 210, 255),
	Color(180, 183, 186, 255)
};

Color GetDifficultyColor(Difficulty type)
{
	return sDifficultyColors[(int)type];
}

template <>
const char* Enum::toString<Difficulty>(Difficulty value)
{
	switch (value)
	{
	case Difficulty::Beginner:
		return "beginner";
	case Difficulty::Easy:
		return "easy";
	case Difficulty::Medium:
		return "medium";
	case Difficulty::Hard:
		return "hard";
	case Difficulty::Challenge:
		return "challenge";
	case Difficulty::Edit:
		return "edit";
	}
	return "unknown";
}

template <>
optional<Difficulty> Enum::fromString<Difficulty>(stringref str)
{
	if (str == "beginner")
		return Difficulty::Beginner;
	if (str == "easy")
		return Difficulty::Easy;
	if (str == "medium")
		return Difficulty::Medium;
	if (str == "hard")
		return Difficulty::Hard;
	if (str == "challenge")
		return Difficulty::Challenge;
	if (str == "edit")
		return Difficulty::Edit;
	return std::nullopt;
}

// =====================================================================================================================
// ChartModification :: helpers.

class ModificationHelper
{
public:
	ModificationHelper(int column, int numPlayers, NoteSet& notes, ChartModification& mod);

	NoteCol outAdd;
	NoteCol outRem;

private:
	void addNote(const Note& note);
	void processNotes();

	int column;
	int numPlayers;

	int regionBegin;
	int regionEnd;

	ChartModification::RemovePolicy removePolicy;

	// Next note in the current list.
	const Note* nextNote;
	const Note* notesEnd;

	// Next to-be-added note.
	const Note* nextAddNote;
	const Note* addNotesEnd;
	int nextAddNoteRow;

	// Next to-be-removed note.
	const Note* nextRemNote;
	const Note* remNotesEnd;
	int nextRemNoteRow;

	// Previous note position.
	int prevRow;
	int prevEndRow;

};

ModificationHelper::ModificationHelper(int column, int numPlayers, NoteSet& notes, ChartModification& mod)
	: column(column)
	, numPlayers(numPlayers)
	, regionBegin(INT_MAX)
	, regionEnd(0)
	, removePolicy(mod.removePolicy)
	, nextNote(notes[column].begin())
	, notesEnd(notes[column].end())
	, nextAddNote(mod.notesToAdd[column].begin())
	, addNotesEnd(mod.notesToAdd[column].end())
	, nextRemNote(mod.notesToRemove[column].begin())
	, remNotesEnd(mod.notesToRemove[column].end())
	, prevRow(-1)
	, prevEndRow(-1)
{
	// Determine the region of the to-be-added notes.
	if (nextAddNote != addNotesEnd && nextAddNote->row < (addNotesEnd - 1)->row)
	{
		regionBegin = nextAddNote->row;
		regionEnd = (addNotesEnd - 1)->row;
	}

	// Determine the next position of each note.
	nextAddNoteRow = (nextAddNote != addNotesEnd) ? nextAddNote->row : INT_MAX;
	nextRemNoteRow = (nextRemNote != remNotesEnd) ? nextRemNote->row : INT_MAX;

	processNotes();
}

void ModificationHelper::addNote(const Note& note)
{
	Row row = note.row;
	if (row <= prevRow)
	{
		if (row == prevRow)
		{
			Log::warning("Bug: added note is on the same position as the previous note.");
		}
		else
		{
			Log::warning("Bug: added note is on a position before the previous note.");
		}
	}
	else if (note.endRow < note.row)
	{
		Log::warning("Bug: added note has its end row before its start row.");
	}
	else if (note.row <= prevEndRow)
	{
		Log::warning("Bug: added note has its start row on or before the end of the previous note.");
	}
	else if ((int)note.player < numPlayers)
	{
		outAdd.append(note);
		prevEndRow = note.endRow;
		prevRow = note.row;
	}
}

void ModificationHelper::processNotes()
{
	while (nextNote != notesEnd)
	{
		const Note& note = *nextNote;
		++nextNote;

		int noteRow = note.row;
		bool keepNote = true;

		// Add all to-be-added notes up to the current note position.
		while (nextAddNoteRow < noteRow)
		{
			addNote(*nextAddNote);
			++nextAddNote;
			nextAddNoteRow = (nextAddNote != addNotesEnd) ? nextAddNote->row : INT_MAX;
		}

		// Skip all to-be-removed notes up to the current note position.
		while (nextRemNoteRow < noteRow)
		{
			++nextRemNote;
			nextRemNoteRow = (nextRemNote != remNotesEnd) ? nextRemNote->row : INT_MAX;
		}

		// Check if the current note is next on the to-be-removed list.
		if (noteRow == nextRemNoteRow)
		{
			keepNote = false;
		}
		else // Check if the note needs to be removed based on the removal policy.
		{
			switch (removePolicy)
			{
			case ChartModification::RemovePolicy::EntireRegion:
				if (noteRow >= regionBegin && noteRow < regionEnd)
				{
					keepNote = false;
				}
				break;
			case ChartModification::RemovePolicy::OverlappingRows:
				if (prevRow == noteRow || (nextAddNote != addNotesEnd && nextAddNote->row == noteRow))
				{
					keepNote = false;
				}
				break;
			}
		}

		// Check if the current note has the same position as the next to-be-added note.
		if (nextAddNoteRow == noteRow)
		{
			if (IsNoteIdentical(note, *nextAddNote))
			{
				keepNote = true; // Added note is identical to the current note, keep it.
				++nextAddNote;
				nextAddNoteRow = (nextAddNote != addNotesEnd) ? nextAddNote->row : INT_MAX;
			}
			else
			{
				keepNote = false;
			}
		}

		// Check if the current note intersects the tail of a previous added note.
		if (prevEndRow >= noteRow)
		{
			keepNote = false;
		}

		// Check if the tail of this note intersects the upcoming added note.
		if (nextAddNoteRow <= note.endRow && keepNote)
		{
			Note trimmed(note);
			trimmed.endRow = max(trimmed.row, nextAddNoteRow - 24);
			addNote(trimmed);
			keepNote = false;
		}

		// After all this work, we finally know if the current note is going to be removed or not.
		if (keepNote)
		{
			prevRow = noteRow;
			prevEndRow = note.endRow;
		}
		else
		{
			outRem.append(note);
		}
	}

	// Remaining notes on the to-be-added list are appended at the end.
	for (; nextAddNote != addNotesEnd; ++nextAddNote)
	{
		addNote(*nextAddNote);
	}
}

// =====================================================================================================================
// ChartModification.

ChartModification::ChartModification(int numCols)
	: removePolicy(RemovePolicy::OverlappingNotes)
	, description(nullptr)
	, notesToAdd(numCols)
	, notesToRemove(numCols)
{
}

// =====================================================================================================================
// ChartModification :: history helper functions.

static void ModifyNotes(Chart* chart, const NoteSet& add, const NoteSet& rem)
{
	chart->notes.remove(rem);
	chart->notes.insert(add);
	chart->notes.sanitize(chart->gameMode, chart->getFullDifficulty().data());
}

// =====================================================================================================================
// Chart.

Chart::Chart(Simfile* owner, const GameMode* mode)
	: simfile(owner)
	, gameMode(mode)
	, notes(mode->numCols)
	, name        (EventSystem::getId<NameChangedEvent>())
	, style       (EventSystem::getId<StyleChangedEvent>())
	, author      (EventSystem::getId<AuthorChangedEvent>())
	, description (EventSystem::getId<DescriptionChangedEvent>())
	, difficulty  (EventSystem::getId<DifficultyChangedEvent>(), Difficulty::Edit)
	, meter       (EventSystem::getId<MeterChangedEvent>(), 1)
{
	history = new ChartHistory(this);
}

Chart::~Chart()
{
	delete history;
}

string Chart::getFullDifficulty() const
{
	return format("{} {}", GetDifficultyName(difficulty.get()), meter.get());
}

Tempo& Chart::getTempo()
{
	return *(tempo ? tempo.get() : simfile->tempo.get());
}

const Tempo& Chart::getTempo() const
{
	return *(tempo ? tempo.get() : simfile->tempo.get());
}

int Chart::getStepCount() const
{
	int count = 0;
	for (auto& col : notes)
	{
		for (auto& note : col)
		{
			count += (note.type != (uint)NoteType::Mine);
		}
	}
	return count;
}

void Chart::sanitize()
{
	if (!gameMode)
	{
		Log::warning(format("{} is missing a game mode.", getFullDifficulty()));
	}

	if (meter.get() < 1)
	{
		Log::warning(format("{} has an invalid meter value {}.",
			getFullDifficulty(), meter.get()));
		meter.set(1);
	}

	if (tempo)
	{
		tempo->sanitize();
	}
		
	auto description = getFullDifficulty();
	notes.sanitize(gameMode, description.data());
}

// =====================================================================================================================
// Chart :: history functions.

void Chart::modify(ChartModification& mod)
{
	for (int col = 0; col < gameMode->numCols; ++col)
	{
		ModificationHelper helper(col, gameMode->numPlayers, notes, mod);

		mod.notesToAdd[col].swap(helper.outAdd);
		mod.notesToRemove[col].swap(helper.outRem);
	}

	auto& add = mod.notesToAdd;
	auto& rem = mod.notesToRemove;

	int numAdded = add.numNotes();
	int numRemoved = rem.numNotes();

	// TODO: fix

	if (numAdded == 1 && numRemoved == 0 && !mod.description)
	{
		for (int col = 0; col < gameMode->numCols; ++col)
		{
			// if (add[col].size())
			//	QueueAddNote(this, col, add[col].begin());
		}
	}
	else if (numAdded == 0 && numRemoved == 1 && !mod.description)
	{
		for (int col = 0; col < gameMode->numCols; ++col)
		{
			// if (rem[col].size())
			//	QueueRemoveNote(this, col, rem[col].begin());
		}
	}
	else if (add.numNotes() + rem.numNotes() > 0)
	{
		// QueueModifyNotes(this, mod);
	}
}

} // namespace AV
