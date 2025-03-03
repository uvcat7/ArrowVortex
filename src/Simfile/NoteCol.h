#pragma once

#include <Simfile/Common.h>

namespace AV {

// Contains notes for a single column, sorted by row.
class NoteCol
{
public:
	~NoteCol();

	NoteCol();
	NoteCol(NoteCol&&);
	NoteCol(const NoteCol&);

	NoteCol& operator = (NoteCol&&);
	NoteCol& operator = (const NoteCol&);

	// Reserves space for the given number of notes.
	void reserve(size_t capacity);

	// Removes all notes.
	void clear();

	// Swaps contents with another note column.
	void swap(NoteCol& other);

	// Replaces the current note column with a copy of the given note column.
	void assign(const NoteCol& other);

	// Appends a note to the back of the current note column.
	void append(const Note& note);

	// Inserts all notes from the insert column.
	void insert(const NoteCol& insert);

	// Removes all notes that match the notes in the remove column.
	void remove(const NoteCol& remove);

	// Removes notes that are marked with negative rows.
	void removeMarkedNotes();

	// Removes all invalid notes (e.g. unsorted, overlapping, invalid row/player number).
	// If one or more notes were removed, a warning message including the description is shown.
	void sanitize(const GameMode* mode, const char* description = nullptr);

	// Encodes the note data and writes it to a bytestream.
	void encodeTo(Serializer& out, bool removeOffset) const;

	// Alternative version of encode that writes startTime stamps instead of rows.
	void encodeTo(Serializer& out, const TimingData& timing, bool removeOffset);

	// Reads encoded note data from a bytestream and inserts the decoded notes.
	// The given offset is applied to the rows and end rows of all the inserted notes.
	void decodeFrom(Deserializer& in, int offsetRows);

	// Alternative version of decode that reads startTime stamps instead of rows.
	void decodeFrom(Deserializer& in, const TimingData& timing, double offsetTime);

	// Returns the end row of the last note.
	Row endRow() const;

	// Returns the number of stored notes.
	inline size_t size() const { return myNum; }

	// Returns true if the list is empty, false otherwise.
	inline bool isEmpty() const { return (myNum == 0); }

	// Returns an iterator to the begin of the note column.
	Note* begin() { return myNotes; }

	// Returns a const iterator to the begin of the note column.
	const Note* begin() const { return myNotes; }

	// Returns an iterator to the end of the note column.
	Note* end() { return myNotes + myNum; }

	// Returns a const iterator to the end of the note column.
	const Note* end() const { return myNotes + myNum; }

private:
	Note* myNotes;
	size_t myNum;
	size_t myCap;
};

} // namespace AV