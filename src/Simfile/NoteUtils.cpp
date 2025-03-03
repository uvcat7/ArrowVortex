#include <Precomp.h>

#include <Core/Common/Serialize.h>
#include <Core/Utils/Util.h>
#include <Core/System/Debug.h>

#include <Simfile/NoteUtils.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/GameMode.h>
#include <Simfile/Tempo.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Note encoding.

void EncodeNote(Serializer& out, const Note& in, int offsetRows)
{
	if (in.row == in.endRow && in.player == 0 && in.type == 0)
	{
		out.writeNum((in.row + offsetRows) << 1);
	}
	else
	{
		out.writeNum(((in.row + offsetRows) << 1) | 1);
		out.writeNum(in.endRow + offsetRows);
		out.write<uchar>((in.player << 4) | in.type);
	}
}

void EncodeNote(Serializer& out,const Note& in, TimingData::TimeTracker& tracker, double offsetTime)
{
	double time = tracker.advance(in.row);
	if (in.row == in.endRow && in.player == 0 && in.type == 0)
	{
		out.write<double>(max(0.000001, time + offsetTime));
	}
	else
	{
		out.write<double>(-max(0.000001, time + offsetTime));
		out.write<double>(tracker.lookAhead(in.endRow) + offsetTime);
		out.write<uchar>((in.player << 4) | in.type);
	}
}

void DecodeNote(Deserializer& in, Note& outNote, int offsetRows)
{
	Row rowBits = in.readNum();
	Row row = rowBits >> 1;
	if ((rowBits & 1) == 0)
	{
		outNote = Note(row, row, NoteType::Step, 0, 0);
	}
	else
	{
		outNote.row = row + offsetRows;
		outNote.endRow = in.readNum() + offsetRows;
		uint v = in.read<uchar>();
		outNote.player = v >> 4;
		outNote.type = v & 0xF;
	}
}

void DecodeNote(Deserializer& in, Note& outNote, TimingData::RowTracker& tracker, double offsetTime)
{
	double time = in.read<double>();
	if (time >= 0.0)
	{
		Row row = lround(tracker.advance(time + offsetTime));
		outNote = Note(row, row, NoteType::Step, 0, 0);
	}
	else
	{
		outNote.row = lround(tracker.advance(-time + offsetTime));
		double endTime = in.read<double>() + offsetTime;
		outNote.endRow = lround(tracker.lookAhead(endTime));
		uint v = in.read<uchar>();
		outNote.player = v >> 4;
		outNote.type = v & 0xF;
	}
}

// =====================================================================================================================
// Misc utils.

bool IsNoteIdentical(const Note& first, const Note& second)
{
	return memcmp(&first, &second, sizeof(Note)) == 0;
}

const char* GetNoteName(int noteType, bool plural)
{
	switch((NoteType)noteType)
	{
	case NoteType::Step:
		return plural ? "steps" : "step";
	case NoteType::Hold:
		return plural ? "holds" : "hold";
	case NoteType::Mine:
		return plural ? "mines" : "mine";
	case NoteType::Roll:
		return plural ? "rolls" : "roll";
	case NoteType::Lift:
		return plural ? "lifts" : "lift";
	case NoteType::Fake:
		return plural ? "fakes" : "fake";
	};
	return plural ? "notes" : "note";
}

const Note* GetNoteAt(Chart* chart, Row row, int col)
{
	VortexAssert(col >= 0 && col < chart->gameMode->numCols);

	auto begin = chart->notes[col].begin();
	auto end = chart->notes[col].end();
	auto it = lower_bound(begin, end, row, [](const Note& a, Row row)
	{
		return a.row < row;
	});

	return (it != end && it->row == row) ? it : nullptr;
}

const Note* GetNoteBefore(Chart* chart, Row row, int col)
{
	VortexAssert(col >= 0 && col < chart->gameMode->numCols);

	const auto& notes = chart->notes[col];
	if (notes.isEmpty())
		return nullptr;

	auto it = lower_bound(notes.begin(), notes.end(), row, [](const Note& a, Row row)
	{
		return a.row < row;
	});

	if (it->row == row)
		return it;

	if (it == notes.begin())
		return nullptr;

	return --it;
}

const Note* GetNoteIntersecting(Chart* chart, Row row, int col)
{
	VortexAssert(col >= 0 && col < chart->gameMode->numCols);

	auto it = chart->notes[col].begin();
	auto nend = chart->notes[col].end();

	for (; it != nend && it->endRow < row; ++it);
	for (; it != nend && it->row <= row; ++it)
	{
		if (it->endRow >= row) return it;
	}
	return nullptr;
}

vector<const Note*> GetNotesBeforeTime(Chart* chart, double time)
{
	vector<const Note*> out;
	out.reserve(chart->gameMode->numCols);

	for (auto& col : chart->notes)
	{
		auto row = (int)lround(chart->getTempo().timing.timeToRow(time));
		auto it = lower_bound(col.begin(), col.end(), row, [](const Note& a, Row row)
		{
			return a.row < row;
		});
		if (it != col.end() && it->row <= row)
		{
			out.push_back(it);
		}
		else
		{
			out.push_back(nullptr);
		}
	}

	return out;
}

} // namespace AV
