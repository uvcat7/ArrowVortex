#pragma once

#include <Simfile/Notes.h>

namespace Vortex {

struct NoteEdit;
struct NoteEditResult;

class NoteList {
   public:
    typedef NoteList List;

    ~NoteList();
    NoteList();
    NoteList(List&&);
    NoteList(const List&);

    List& operator=(List&&);
    List& operator=(const List&);

    // Removes all notes.
    void clear();

    // Removes notes with negative rows.
    void cleanup();

    // Removes invalid notes (e.g. unsorted, overlapping, invalid
    // row/column/player number).
    void sanitize(const Chart* owner);

    // Prepares a modification. The input is a list of notes which should be
    // added and removed. The output is a list of notes that actually end up
    // being added and removed. If clearRegion is true, all notes in the edit
    // region are also removed.
    void prepareEdit(const NoteEdit& in, NoteEditResult& out, bool clearRegion);

    // Replaces the contents with a copy of the given list.
    void assign(const List& other);

    // Appends a note to the back of this list.
    void append(const Note& note);

    // Inserts notes from the insert list into this list.
    void insert(const List& insert);

    // Removes all notes that match the notes in the remove list.
    void remove(const List& remove);

    // Encodes the note data and writes it to a bytestream.
    void encode(WriteStream& out, bool removeOffset) const;

    // Alternative version of encode that writes time stamps instead of rows.
    void encode(WriteStream& out, const TimingData& timing, bool removeOffset);

    // Reads encoded note data from a bytestream and inserts it.
    void decode(ReadStream& in, int offsetRows);

    // Alternative version of decode that reads time stamps instead of rows.
    void decode(ReadStream& in, const TimingData& timing, double offsetTime);

    // Returns the number of stored notes.
    inline int size() const { return myNum; }

    // Returns true if the list is empty, false otherwise.
    inline bool empty() const { return (myNum == 0); }

    // Returns an iterator to the begin of the note list.
    Note* begin() { return myNotes; }

    // Returns a const iterator to the begin of the note list.
    const Note* begin() const { return myNotes; }

    // Returns an iterator to the end of the note list.
    Note* end() { return myNotes + myNum; }

    // Returns a const iterator to the end of the note list.
    const Note* end() const { return myNotes + myNum; }

   private:
    void myReserve(int num);
    Note* myNotes;
    int myNum, myCap;
};

struct NoteEdit {
    NoteList add, rem;
};

struct NoteEditResult {
    NoteList add, rem;
};

};  // namespace Vortex