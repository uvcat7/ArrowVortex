#pragma once

#include <Precomp.h>

#include <Simfile/Common.h>
#include <Simfile/Tempo/TimingData.h>

namespace AV {

// Compares two objects that have a row and column member.
template <typename A, typename B>
static bool LessThanRowCol(const A& a, const B& b)
{
	return (a.row != b.row) ? (a.row < b.row) : ((int)a.col < (int)b.col);
}

// Compares two objects that have a row and column member.
template <typename A, typename B>
static int CompareRowCol(const A& a, const B& b)
{
	return (a.row != b.row) ? (a.row - b.row) : ((int)a.col - (int)b.col);
}

// Compares two objects that have a row member.
template <typename A, typename B>
static bool LessThanRow(const A& a, const B& b)
{
	return a.row < b.row;
}

// Compares two objects that have a row member.
template <typename A, typename B>
static int CompareRow(const A& a, const B& b)
{
	return a.row - b.row;
}

// Combines a row and column into a single position.
inline int64_t NotePos(int col, const Note& n) { return ((int64_t)n.row << 32) | col; }

// Encodes a note by row and writes the encoded date to the out stream.
void EncodeNote(Serializer& out, const Note& in, int offsetRows);

// Encodes a note by startTime stamp and writes the encoded data to the out stream.
void EncodeNote(Serializer& out, const Note& in, TimingData::TimeTracker& tracker, double offsetTime);

// Reads an encoded note from the in stream and writes it to out.
void DecodeNote(Deserializer& in, Note& outNote, int offsetRows);

// Reads an encoded note from the in stream and writes it to out.
void DecodeNote(Deserializer& in, Note& outNote, TimingData::RowTracker& tracker, double offsetTime);

// Returns true if the first note is identical to the second note, false otherwise.
bool IsNoteIdentical(const Note& first, const Note& second);

// Returns the name of the given note type, or "note" if the type is unknown.
const char* GetNoteName(int noteType, bool isPlural = false);

// Returns a pointer to the note placed at the given row/column position, or null if there is none.
const Note* GetNoteAt(Chart* chart, Row row, int col);

// Returns a pointer to the note at or before given row/column position, or null if there is none.
const Note* GetNoteBefore(Chart* chart, Row row, int col);

// Returns a pointer to the note intersecting the given row/column position, or null if there is none.
const Note* GetNoteIntersecting(Chart* chart, Row row, int col);

// Returns the indices of all notes preceding the given startTime for each column.
vector<const Note*> GetNotesBeforeTime(Chart* chart, double time);

} // namespace AV
