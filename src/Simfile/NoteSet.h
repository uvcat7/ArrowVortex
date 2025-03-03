#pragma once

#include <Simfile/NoteCol.h>

namespace AV {

// Contains lists of notes for one or more columns.
class NoteSet
{
public:
	friend class NoteModificationHelper;

	~NoteSet();
	NoteSet(int numNoteCols);

	// Swaps contents with another set.
	void swap(NoteSet& other);

	// Removes all notes.
	void clear();

	// Merges the given note set into this set.
	void insert(const NoteSet& add);

	// Removes the notes in the given set from this set.
	void remove(const NoteSet& rem);

	// Removes notes with negative rows.
	void removeMarkedNotes();

	// Removes invalid notes (e.g. unsorted, overlapping, redundant, invalid row).
	// If one or more notes were removed, a warning message including the description is shown.
	void sanitize(const GameMode* mode, const char* description);

	// Encodes the note data and writes it to a bytestream.
	void encodeTo(Serializer& out, bool removeOffset) const;

	// Encodes the note data and writes it to a bytestream.
	void encodeTo(Serializer& out, const TimingData& timing, bool removeOffset) const;

	// Reads encoded note data from a bytestream and inserts it.
	void decodeFrom(Deserializer& in, int offsetRows);

	// Reads encoded note data from a bytestream and inserts it.
	void decodeFrom(Deserializer& in, const TimingData& timing, int offsetRows);

	// Returns the combined number of notes in all columns.
	int numNotes() const;

	// Returns the number of columns.
	int numColumns() const;

	// Returns a pointer to the note with the lowest row.
	const Note* firstNote() const;

	// Returns a pointer to the note with the highest row.
	const Note* lastNote() const;

	// Returns the largest end row of all columns.
	Row endRow() const;

	// Returns a pointer to the begin of the note lists.
	NoteCol* begin();

	// Returns a pointer to the begin of the note lists.
	const NoteCol* begin() const;

	// Returns a pointer to the end of the note lists.
	NoteCol* end();

	// Returns a pointer to the end of the note lists.
	const NoteCol* end() const;

	// Returns the note list of the given column.
	inline NoteCol& column(int column) { return myLists[column]; }

	// Returns the note list of the given column.
	inline const NoteCol& column(int column) const { return myLists[column]; }

	// Returns the note list of the given column.
	inline NoteCol& operator [] (int column) { return myLists[column]; }

	// Returns the note list of the given column.
	inline const NoteCol& operator [] (int column) const { return myLists[column]; }

private:
	NoteCol* myLists;
	int myNumColumns;
};

} // namespace AV