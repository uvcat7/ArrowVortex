#pragma once

#include <Simfile/Common.h>

#include <Core/Vector.h>

namespace Vortex {

// Supported note types.
enum NoteType
{
	NOTE_STEP_OR_HOLD,
	NOTE_MINE,
	NOTE_ROLL,
	NOTE_LIFT,
	NOTE_FAKE,

	NUM_NOTE_TYPES
};

/// Expanded representation of a note, including editing data.
struct ExpandedNote
{
	/// Row index of the note.
	int row;

	/// Column index of the note.
	int col;

	/// Equal to 'row' for steps, larger than 'row' for holds.
	int endrow;

	/// Time of the row.
	double time;

	/// Time of the end row.
	double endtime;

	/// 1 indicates a mine, 0 indicates a step or hold. 
	uint isMine : 1;

	/// 1 indicates a roll, 0 indicates a freeze.
	uint isRoll : 1;

	/// 1 indicates a warped note, 0 indicates a regular note.
	uint isWarped : 1;

	/// 1 indicates selected, 0 indicates not selected.
	uint isSelected : 1;

	/// One of the values in NoteType, indicates what kind of note it is.
	uint type : 4;

	/// Indicates which player the note belongs to in routine modes.
	uint player : 24;
};

// Converts and expanded note to a compact note.
inline Note CompressNote(const ExpandedNote& note)
{
	return {note.row, note.endrow, (uint)note.col, (uint)note.player, (uint)note.type};
}

// Encodes a single note and writes it to a bytestream.
void EncodeNote(WriteStream& out, const Note& in);

// Encodes a single note and writes it to a bytestream.
void EncodeNoteWithTime(WriteStream& out, const ExpandedNote& in);

// Encodes the notes and writes them to a bytestream.
void EncodeNotes(WriteStream& out, const Note* in, int num);

// Reads an encoded note from a bytestream and decodes it to out.
void DecodeNote(ReadStream& in, Note& out);

// Reads an encoded note from a bytestream and decodes it to out.
void DecodeNoteWithTime(ReadStream& in, ExpandedNote& out);

}; // namespace Vortex