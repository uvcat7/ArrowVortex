#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/Log.h>

#include <Core/Common/Serialize.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteCol.h>
#include <Simfile/GameMode.h>
#include <Simfile/NoteUtils.h>
#include <Simfile/Tempo/TimingData.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// NoteCol :: destructor and constructors.

NoteCol::~NoteCol()
{
	clear();
	free(myNotes);
}

NoteCol::NoteCol()
	: myNotes(nullptr)
	, myNum(0)
	, myCap(0)
{
}

NoteCol::NoteCol(NoteCol&& list)
	: myNotes(list.myNotes)
	, myNum(list.myNum)
	, myCap(list.myCap)
{
	list.myNotes = nullptr;
}

NoteCol::NoteCol(const NoteCol& list)
	: myNotes(nullptr)
	, myNum(0)
	, myCap(0)
{
	assign(list);
}

NoteCol& NoteCol::operator = (NoteCol&& list)
{
	myNotes = list.myNotes;
	myNum = list.myNum;
	myCap = list.myCap;

	list.myNotes = nullptr;

	return *this;
}

NoteCol& NoteCol::operator = (const NoteCol& list)
{
	assign(list);
	return *this;
}

// =====================================================================================================================
// NoteCol :: manipulation.

void NoteCol::reserve(size_t capacity)
{
	auto numBytes = capacity * sizeof(Note);
	if (myCap < numBytes)
	{
		myCap = max(numBytes, myCap << 1);
		myNotes = (Note*)realloc(myNotes, myCap);
	}
}

void NoteCol::clear()
{
	myNum = 0;
}

void NoteCol::assign(const NoteCol& list)
{
	myNum = list.myNum;
	reserve(myNum);
	memcpy(myNotes, list.myNotes, myNum * sizeof(Note));
}

void NoteCol::swap(NoteCol& other)
{
	std::swap(other.myNotes, myNotes);
	std::swap(other.myNum, myNum);
	std::swap(other.myCap, myCap);
}

void NoteCol::append(const Note& note)
{
	auto index = myNum;
	reserve(++myNum);
	myNotes[index] = note;
}

void NoteCol::insert(const NoteCol& insert)
{
	if (insert.myNum == 0) return;

	auto newSize = myNum + insert.myNum;
	reserve(newSize);

	// Work backwards, that way insertion can be done on the fly.
	auto write = myNotes + newSize - 1;

	auto read = myNotes + myNum - 1;
	auto readEnd = myNotes - 1;

	auto ins = insert.myNotes + insert.myNum - 1;
	auto insEnd = insert.myNotes - 1;

	while (read != readEnd)
	{
		// Copy notes from the insert list.
		while (ins != insEnd && ins->row > read->row)
		{
			*write = *ins;
			--write, --ins;
		}
		if (ins == insEnd) break;

		// Move existing notes.
		while (read != readEnd && read->row >= ins->row)
		{
			*write = *read;
			--write, --read;
		}
	}

	// After read end is reached, there might still be some notes that need to be inserted.
	while (ins != insEnd)
	{
		*write = *ins;
		--write, --ins;
	}

	myNum = newSize;
}

void NoteCol::remove(const NoteCol& remove)
{
	if (remove.myNum == 0) return;

	// Work forwards from the first note.
	auto it = myNotes;
	auto itEnd = myNotes + myNum;

	auto rem = remove.myNotes;
	auto remEnd = remove.myNotes + remove.myNum;

	// Mark all notes that have a matching note in the remove list.
	while (rem != remEnd)
	{
		while (it != itEnd && it->row < rem->row)
		{
			++it;
		}
		if (it != itEnd && it->row == rem->row)
		{
			it->row = -1;
		}
		++rem;
	}

	removeMarkedNotes();
}

void NoteCol::removeMarkedNotes()
{
	auto read = myNotes;
	auto end = myNotes + myNum;

	// Skip over valid notes until we arrive at the first marked note.
	while (read != end && read->row >= 0) ++read;
	auto write = read;

	// Then, start skipping blocks of marked notes and start moving blocks of unmarked notes.
	while (read != end)
	{
		// Skip a block of marked notes.
		while (read != end && read->row < 0) ++read;

		// Move a block of unmarked notes.
		auto moveBegin = read;
		while (read != end && read->row >= 0) ++read;
		auto offset = read - moveBegin;
		memmove(write, moveBegin, offset * sizeof(Note));
		write += offset;
	}

	// Update note count.
	auto oldNum = myNum;
	myNum = write - myNotes;
}

void NoteCol::sanitize(const GameMode* mode, const char* description)
{
	if (!description) description = "Note list";

	uint playerCount = mode->numPlayers;

	// Test the notes.
	int numInvalidPlayers = 0;
	int numInconsistent = 0;
	int numOverlapping = 0;
	int numUnsorted = 0;

	Row row = -1;
	int endrow = -1;

	// Make sure all notes are valid, sorted, and do not overlap.
	for (auto& note : *this)
	{
		if (note.player >= playerCount)
		{
			++numInvalidPlayers;
			note.row = -1;
		}
		else if (note.endRow < note.row)
		{
			++numInconsistent;
			note.row = -1;
		}
		else if (note.row <= endrow)
		{
			if (note.row >= row)
			{
				++numOverlapping;
			}
			else
			{
				++numUnsorted;
			}
			note.row = -1;
		}
		else
		{
			row = note.row;
			endrow = note.endRow;
		}
	}

	// Notify the user if the chart contained invalid notes.
	if (numInvalidPlayers + numInconsistent + numOverlapping + numUnsorted > 0)
	{
		if (numInvalidPlayers > 0)
			Log::warning(format("{} has {} note(s) with invalid player.", description, numInvalidPlayers));

		if (numInconsistent > 0)
			Log::warning(format("{} has {} inconsistent note(s).", description, numInconsistent));

		if (numOverlapping > 0)
			Log::warning(format("{} has {} overlapping note(s).", description, numOverlapping));

		if (numUnsorted > 0)
			Log::warning(format("{} has {} out of order note(s).", description, numUnsorted));

		removeMarkedNotes();
	}
}

// =====================================================================================================================
// NoteCol :: encoding / decoding.

void NoteCol::encodeTo(Serializer& out, bool removeOffset) const
{
	int offsetRows = 0;
	out.writeNum(myNum);
	if (removeOffset && myNum > 0)
	{
		offsetRows = -myNotes[0].row;
	}
	for (int i = 0; i < myNum; ++i)
	{
		EncodeNote(out, myNotes[i], offsetRows);
	}
}

void NoteCol::encodeTo(Serializer& out, const TimingData& timing, bool removeOffset)
{
	auto tracker = timing.timeTracker();
	double offsetTime = 0;
	out.writeNum(myNum);
	if (removeOffset && myNum > 0)
	{
		offsetTime = -timing.rowToTime(myNotes[0].row);
	}
	for (int i = 0; i < myNum; ++i)
	{
		EncodeNote(out, myNotes[i], tracker, offsetTime);
	}
}

void NoteCol::decodeFrom(Deserializer& in, int offsetRows)
{
	Note note;
	uint num = in.readNum();
	for (uint i = 0; i < num; ++i)
	{
		DecodeNote(in, note, offsetRows);
		append(note);
	}
}

void NoteCol::decodeFrom(Deserializer& in, const TimingData& timing, double offsetTime)
{
	auto tracker = timing.rowTracker();
	Note note;
	uint num = in.readNum();
	for (uint i = 0; i < num; ++i)
	{
		DecodeNote(in, note, tracker, offsetTime);
		append(note);
	}
}

int NoteCol::endRow() const
{
	return myNum ? myNotes[myNum - 1].endRow : 0;
}

} // namespace AV
