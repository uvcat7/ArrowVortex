#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/Log.h>

#include <Core/Common/Serialize.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/GameMode.h>
#include <Simfile/NoteUtils.h>
#include <Simfile/Tempo/TimingData.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// NoteSet.

NoteSet::~NoteSet()
{
}

NoteSet::NoteSet(int numColumns)
	: myLists(new NoteCol[numColumns])
	, myNumColumns(numColumns)
{
}

void NoteSet::swap(NoteSet& other)
{
	int numCols = min(myNumColumns, other.myNumColumns);
	for (int col = 0; col < numCols; ++col)
	{
		myLists[col].swap(other.myLists[col]);
	}
}

void NoteSet::clear()
{
	for (int col = 0; col < myNumColumns; ++col)
	{
		myLists[col].clear();
	}
}

void NoteSet::insert(const NoteSet& add)
{
	for (int col = 0, end = max(myNumColumns, add.myNumColumns); col < end; ++col)
	{
		myLists[col].insert(add.myLists[col]);
	}
}

void NoteSet::remove(const NoteSet& rem)
{
	for (int col = 0, end = max(myNumColumns, rem.myNumColumns); col < end; ++col)
	{
		myLists[col].remove(rem.myLists[col]);
	}
}

void NoteSet::removeMarkedNotes()
{
	for (int col = 0; col < myNumColumns; ++col)
	{
		myLists[col].removeMarkedNotes();
	}
}

void NoteSet::sanitize(const GameMode* mode, const char* description)
{
	if (mode->numCols != myNumColumns)
	{
		if (!description) description = "Note set";
		Log::warning(format("{} has {} columns, but {} has {} columns.",
			description, myNumColumns, mode->id.data(), mode->numCols));
	}
	else
	{
		for (int col = 0; col < myNumColumns; ++col)
		{
			myLists[col].sanitize(mode, description);
		}
	}
}

void NoteSet::encodeTo(Serializer& out, bool removeOffset) const
{
	for (int col = 0; col < myNumColumns; ++col)
	{
		if (myLists[col].size())
		{
			out.writeNum(col + 1);
			myLists[col].encodeTo(out, removeOffset);
		}
	}
	out.writeNum(0);
}

void NoteSet::encodeTo(Serializer& out, const TimingData& timing, bool removeOffset) const
{
	for (int col = 0; col < myNumColumns; ++col)
	{
		if (myLists[col].size())
		{
			out.writeNum(col + 1);
			myLists[col].encodeTo(out, timing, removeOffset);
		}
	}
	out.writeNum(0);
}

void NoteSet::decodeFrom(Deserializer& in, int offsetRows)
{
	uint num = in.readNum();
	while (num > 0)
	{
		int column = num - 1;
		if (column < myNumColumns)
		{
			myLists[column].decodeFrom(in, offsetRows);
		}
		num = in.readNum();
	}
}

void NoteSet::decodeFrom(Deserializer& in, const TimingData& timing, int offsetRows)
{
	uint num = in.readNum();
	while (num > 0)
	{
		int column = num - 1;
		if (column < myNumColumns)
		{
			myLists[column].decodeFrom(in, timing, offsetRows);
		}
		num = in.readNum();
	}
}

int NoteSet::numNotes() const
{
	int numNotes = 0;

	for (int col = 0; col < myNumColumns; ++col)
		numNotes += (int)myLists[col].size();

	return numNotes;
}

int NoteSet::numColumns() const
{
	return myNumColumns;
}

const Note* NoteSet::firstNote() const
{
	int outRow = INT_MAX;
	const Note* outNote = nullptr;
	for (int col = 0; col < myNumColumns; ++col)
	{
		auto& list = myLists[col];
		auto first = list.begin();
		if (list.size() && first->row < outRow)
		{
			outNote = first;
			outRow = first->row;
		}
	}
	return outNote;
}

const Note* NoteSet::lastNote() const
{
	int outRow = 0;
	const Note* outNote = nullptr;
	for (int col = 0; col < myNumColumns; ++col)
	{
		auto& list = myLists[col];
		auto last = list.end() - 1;
		if (list.size() && last->row >= outRow)
		{
			outNote = last;
			outRow = last->row;
		}
	}
	return outNote;
}

int NoteSet::endRow() const
{
	int result = 0;

	for (int col = 0; col < myNumColumns; ++col)
		result = max(result, myLists[col].endRow());

	return result;
}

NoteCol* NoteSet::begin()
{
	return myLists;
}

const NoteCol* NoteSet::begin() const
{
	return myLists;
}

NoteCol* NoteSet::end()
{
	return myLists + myNumColumns;
}

const NoteCol* NoteSet::end() const
{
	return myLists + myNumColumns;
}

} // namespace AV
