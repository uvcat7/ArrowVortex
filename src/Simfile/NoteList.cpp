#include <Simfile/NoteList.h>

#include <Core/Utils.h>
#include <Core/ByteStream.h>
#include <Core/StringUtils.h>

#include <System/Debug.h>

#include <Simfile/Chart.h>
#include <Simfile/TimingData.h>

#include <Managers/StyleMan.h>

#include <Editor/Common.h>

#include <stdlib.h>
#include <stdint.h>

namespace Vortex {
namespace {

inline int64_t NotePos(const Note* n) {
    return ((int64_t)n->row << 8) | n->col;
}

template <typename T, typename R>
inline bool ComesBefore(const T* a, const R* b) {
    return (a->row < b->row || (a->row == b->row && a->row < b->row));
}

};  // namespace

// ================================================================================================
// NoteNoteList :: destructor and constructors.

NoteList::~NoteList() {
    clear();
    free(myNotes);
}

NoteList::NoteList() : myNotes(nullptr), myNum(0), myCap(0) {}

NoteList::NoteList(List&& list)
    : myNotes(list.myNotes), myNum(list.myNum), myCap(list.myCap) {
    list.myNotes = nullptr;
}

NoteList::NoteList(const List& list) : myNotes(nullptr), myNum(0), myCap(0) {
    assign(list);
}

NoteList& NoteList::operator=(List&& list) {
    myNotes = list.myNotes;
    myNum = list.myNum;
    myCap = list.myCap;

    list.myNotes = nullptr;

    return *this;
}

NoteList& NoteList::operator=(const List& list) {
    assign(list);
    return *this;
}

// ================================================================================================
// NoteList :: manipulation.

void NoteList::clear() { myNum = 0; }

void NoteList::assign(const List& list) {
    myNum = list.myNum;
    myReserve(myNum);
    memcpy(myNotes, list.myNotes, myNum * sizeof(Note));
}

void NoteList::append(const Note& note) {
    int index = myNum;
    myReserve(++myNum);
    myNotes[index] = note;
}

void NoteList::insert(const List& insert) {
    if (insert.myNum == 0) return;

    int newSize = myNum + insert.myNum;
    myReserve(newSize);

    // Work backwards, that way insertion can be done on the fly.
    auto write = myNotes + newSize - 1;

    auto read = myNotes + myNum - 1;
    auto readEnd = myNotes - 1;

    auto ins = insert.myNotes + insert.myNum - 1;
    auto insEnd = insert.myNotes - 1;

    while (read != readEnd) {
        // Copy notes from the insert list.
        int64_t readPos = NotePos(read);
        while (ins != insEnd && NotePos(ins) > readPos) {
            *write = *ins;
            --write, --ins;
        }
        if (ins == insEnd) break;

        // Move existing notes.
        int64_t insPos = NotePos(ins);
        while (read != readEnd && NotePos(read) >= insPos) {
            *write = *read;
            --write, --read;
        }
    }

    // After read end is reached, there might still be some notes that need to
    // be inserted.
    while (ins != insEnd) {
        *write = *ins;
        --write, --ins;
    }

    myNum = newSize;
}

void NoteList::remove(const List& remove) {
    if (remove.myNum == 0) return;

    // Work forwards from the first note.
    auto it = myNotes;
    auto itEnd = myNotes + myNum;

    auto rem = remove.myNotes;
    auto remEnd = remove.myNotes + remove.myNum;

    // Invalidate the notes that have a matching note in the remove vector.
    while (rem != remEnd) {
        int64_t remPos = NotePos(rem);
        while (it != itEnd && NotePos(it) < remPos) {
            ++it;
        }
        if (it != itEnd && NotePos(it) == remPos) {
            it->row = -1;
        }
        ++rem;
    }
    cleanup();
}

void NoteList::cleanup() {
    auto read = myNotes;
    auto end = myNotes + myNum;

    // Skip over valid notes until we arrive at the first invalid note.
    while (read != end && read->row >= 0) ++read;
    auto write = read;

    // Then, start skipping blocks of invalid notes and start moving blocks of
    // valid notes.
    while (read != end) {
        // Skip a block of invalid notes.
        while (read != end && read->row < 0) ++read;

        // Move a block of valid notes.
        auto moveBegin = read;
        while (read != end && read->row >= 0) ++read;
        auto offset = read - moveBegin;
        memmove(write, moveBegin, offset * sizeof(Note));
        write += offset;
    }

    myNum = (int)(write - myNotes);
}

void NoteList::sanitize(const Chart* chart) {
    int numCols = chart->style->numCols;
    int numPlayers = chart->style->numPlayers;

    // Notify the user if the column or player count is invalid.
    if (numCols <= 0 || numPlayers <= 0) {
        std::string prefix(chart ? chart->description() : "Chart");
        if (numCols <= 0) {
            HudWarning("%s has an invalid column count (%i).", prefix.c_str(),
                       numCols);
        }
        if (numPlayers <= 0) {
            HudWarning("%s has an invalid player count (%i).", prefix.c_str(),
                       numPlayers);
        }
        clear();
    }

    // Test the notes.
    int numInvalidPlayers = 0;
    int numInvalidCols = 0;
    int numOverlapping = 0;
    int numUnsorted = 0;
    int numInvalidQuant = 0;

    Vector<int> endrowVec(numCols, -1);
    int* endrows = endrowVec.data();
    uint32_t col = -1;
    int row = -1;

    // Make sure all notes are valid, sorted, and do not overlap.
    for (auto& note : *this) {
        if (note.col >= (uint32_t)numCols) {
            ++numInvalidCols;
            note.row = -1;
        } else if (note.player >= (uint32_t)numPlayers) {
            ++numInvalidPlayers;
            note.row = -1;
        } else if (note.row <= endrows[note.col]) {
            ++numOverlapping;
            note.row = -1;
        } else if (note.row < row || (note.row == row && note.col <= col)) {
            ++numUnsorted;
            note.row = -1;
        } else if (note.quant <= 0 || note.quant > 192) {
            ++numInvalidQuant;
            note.row = -1;
        } else {
            col = note.col;
            row = note.row;
            endrows[note.col] = (int)note.endrow;
        }
    }

    // Notify the user if the chart contained invalid notes.
    if (numInvalidCols + numInvalidPlayers + numOverlapping + numUnsorted +
            numInvalidQuant >
        0) {
        cleanup();

        std::string suffix = " from ";
        Str::append(suffix, chart->description());

        if (numInvalidCols > 0) {
            HudNote("Removed %i note(s) with an invalid column%s.",
                    numInvalidCols, suffix.c_str());
        }
        if (numInvalidPlayers > 0) {
            HudNote("Removed %i note(s) with an invalid player%s.",
                    numInvalidPlayers, suffix.c_str());
        }
        if (numOverlapping > 0) {
            HudNote("Removed %i overlapping note(s)%s.", numOverlapping,
                    suffix.c_str());
        }
        if (numUnsorted > 0) {
            HudNote("Removed %i out of order note(s)%s.", numUnsorted,
                    suffix.c_str());
        }
        if (numInvalidQuant > 0) {
            HudNote("Removed %i note(s) with invalid quantization label(s)%s.",
                    numInvalidQuant, suffix.c_str());
        }
    }
}

// ================================================================================================
// NoteList :: preparation operations.

static void VerifyAdd(const Note& note, int64_t& prevPos, int* prevEndRows) {
    int64_t pos = NotePos(&note);
    if (pos <= prevPos) {
        if (pos == prevPos) {
            HudWarning(
                "Bug: added note is on the same position as the previous "
                "note.");
        } else {
            HudWarning(
                "Bug: added note is on a position before the previous note.");
        }
    } else if (note.endrow < note.row) {
        HudWarning("Bug: added note has its end row before its start row.");
    } else if (note.row < prevEndRows[note.col]) {
        HudWarning(
            "Bug: added note has its start row before the end of the previous "
            "note.");
    }
    prevPos = pos;
    prevEndRows[note.col] = note.endrow;
}

void NoteList::prepareEdit(const NoteEdit& in, NoteEditResult& out,
                           bool clearRegion) {
    out.add.clear();
    out.rem.clear();

    auto it = begin(), itEnd = end();
    auto add = in.add.begin(), addEnd = in.add.end();
    auto rem = in.rem.begin(), remEnd = in.rem.end();

    int regionBegin, regionEnd;
    if (add != addEnd && add->row < (addEnd - 1)->row) {
        regionBegin = add->row;
        regionEnd = (addEnd - 1)->row;
    } else {
        regionBegin = INT_MAX;
        regionEnd = 0;
    }

    int prevEndRows[SIM_MAX_COLUMNS];
    for (auto& v : prevEndRows) v = -1;

    int nextAddRows[SIM_MAX_COLUMNS];
    for (auto& v : nextAddRows) v = 0;

    const Note* nextAddNotes[SIM_MAX_COLUMNS];
    for (auto& v : nextAddNotes) v = add;

    int64_t prevPos = -1;
    int64_t nextAddPos = (add != addEnd) ? NotePos(add) : INT64_MAX;
    int64_t nextRemPos = (rem != remEnd) ? NotePos(rem) : INT64_MAX;

    for (; it != itEnd; ++it) {
        int64_t pos = NotePos(it);
        int row = it->row, col = it->col;
        bool removeNote = false;

        // Advance to the next to-be-added note, adding previously passed notes
        // in the process.
        while (nextAddPos < pos) {
            out.add.append(*add);
            VerifyAdd(*add, prevPos, prevEndRows);
            ++add;
            nextAddPos = (add != addEnd) ? NotePos(add) : INT64_MAX;
        }

        // Advance to the next to-be-removed note.
        while (nextRemPos < pos) {
            ++rem;
            nextRemPos = (rem != remEnd) ? NotePos(rem) : INT64_MAX;
        }

        // Keep track of the row of the next to-be-added note for this column.
        int nextAddRow = nextAddRows[col];
        while (nextAddRow <= row) {
            auto nextAdd = nextAddNotes[col];
            while (nextAdd != addEnd &&
                   (nextAdd->col != col || nextAdd->row <= row)) {
                ++nextAdd;
            }

            nextAddRow = nextAddRows[col] =
                (nextAdd != addEnd) ? nextAdd->row : INT_MAX;
            nextAddNotes[col] = nextAdd;
        }

        // Check if this note appears on the to-be-removed list.
        if (nextRemPos == pos) {
            removeNote = true;
        }

        // Check if the note is inside the modified region.
        if (clearRegion && row >= regionBegin && row < regionEnd) {
            removeNote = true;
        }

        // Check if the next note on the to-be-added list is in the same
        // position this note.
        if (nextAddPos == pos) {
            if (add->endrow == it->endrow && add->player == it->player &&
                add->type == it->type) {
                removeNote = false;  // Added note is identical to this note,
                                     // cancel removal of it.
            } else {
                removeNote = true;
                out.add.append(*add);
                VerifyAdd(*add, prevPos, prevEndRows);
            }
            ++add;
            nextAddPos = (add != addEnd) ? NotePos(add) : INT64_MAX;
        }

        // Check if the tail of the previously added note intersects this note.
        if (prevEndRows[col] >= row) {
            removeNote = true;
        }

        // Check if the tail of this note intersects the upcoming added note.
        if (nextAddRows[col] <= it->endrow && !removeNote) {
            Note trimmed = *it;
            trimmed.endrow = max(trimmed.row, nextAddRows[col] - 24);
            out.add.append(trimmed);
            VerifyAdd(trimmed, prevPos, prevEndRows);
            removeNote = true;
        }

        // After all this work, we finally know if the current note is going to
        // be removed or not.
        if (removeNote) {
            out.rem.append(*it);
        } else {
            prevPos = pos;
            prevEndRows[col] = it->endrow;
        }
    }

    // Remaining notes on the to-be-added list are appended at the end.
    for (; add != addEnd; ++add) {
        out.add.append(*add);
        VerifyAdd(*add, prevPos, prevEndRows);
    }
}

// ================================================================================================
// NoteList :: encoding / decoding.

static void EncodeNote(WriteStream& out, const Note& in, int offsetRows) {
    if (in.row == in.endrow && in.player == 0 && in.type == 0) {
        out.write<uint8_t>(in.col);
        out.writeNum(in.row + offsetRows);
        out.write<uint8_t>(in.quant);
    } else {
        out.write<uint8_t>(in.col | 0x80);
        out.writeNum(in.row + offsetRows);
        out.writeNum(in.endrow + offsetRows);
        out.write<uint8_t>((in.player << 4) | in.type);
        out.write<uint8_t>(in.quant);
    }
}

static void EncodeNote(WriteStream& out, const Note& in,
                       TempoTimeTracker& tracker, double offsetTime) {
    double time = tracker.advance(in.row);
    if (in.row == in.endrow && in.player == 0 && in.type == 0) {
        out.write<uint8_t>(in.col);
        out.write<double>(time + offsetTime);
        out.write<uint8_t>(in.quant);
    } else {
        out.write<uint8_t>(in.col | 0x80);
        out.write<double>(time + offsetTime);
        out.write<double>(tracker.lookAhead(in.endrow) + offsetTime);
        out.write<uint8_t>((in.player << 4) | in.type);
        out.write<uint8_t>(in.quant);
    }
}

// If we are outputting something marked as a non-standard quantization,
// adjust it to align with the expected custom snap.
static void ApplyQuantOffset(Note& out, int offsetRows) {
    int startingOffset = (out.row - offsetRows) % 192;
    int endingOffset = out.row % 192;
    int endingRowOffset = out.endrow % 192;
    if (out.quant == 0 || out.quant > 192) {
        out.quant = 192;
        HudError("Bug: Missing quantization label for note at %i", out.row);
    }
    // Use the starting row to shift basis to the correct one
    // get quant-based index of starting note/offset: round(endingOffset /
    // 192.0f * out.quant) recalculate the position in the measure of the quant:
    // (int) round(192.0f / out.quant * (above) then make this the new measure
    // offset
    if (192 % out.quant > 0 && startingOffset != endingOffset) {
        out.row = out.row - endingOffset +
                  (int)round(192.0f / out.quant *
                             round(endingOffset / 192.0f * out.quant));
        out.endrow = out.endrow - endingRowOffset +
                     (int)round(192.0f / out.quant *
                                round(endingRowOffset / 192.0f * out.quant));
    }
}

static void DecodeNote(ReadStream& in, Note& out, int offsetRows) {
    uint8_t col = in.read<uint8_t>();
    if ((col & 0x80) == 0) {
        int row = in.readNum() + offsetRows;
        uint8_t quant = in.read<uint8_t>();
        out = {row, row, col, 0, 0, quant};
    } else {
        out.col = col & 0x7F;
        out.row = in.readNum() + offsetRows;
        out.endrow = in.readNum() + offsetRows;
        uint32_t v = in.read<uint8_t>();
        out.player = v >> 4;
        out.type = v & 0xF;
        out.quant = in.read<uint8_t>();
    }
    ApplyQuantOffset(out, offsetRows);
}

static void DecodeNote(ReadStream& in, Note& out, TempoRowTracker& tracker,
                       double offsetTime) {
    uint8_t col = in.read<uint8_t>();
    int offsetRows = tracker.lookAhead(offsetTime);
    if ((col & 0x80) == 0) {
        double time = in.read<double>() + offsetTime;
        int row = tracker.advance(time);
        uint8_t quant = in.read<uint8_t>();
        out = {row, row, col, 0, 0, quant};
    } else {
        out.col = col & 0x7F;
        double time = in.read<double>() + offsetTime;
        out.row = tracker.advance(time);
        double endTime = in.read<double>() + offsetTime;
        out.endrow = tracker.lookAhead(time);
        uint32_t v = in.read<uint8_t>();
        out.player = v >> 4;
        out.type = v & 0xF;
        out.quant = in.read<uint8_t>();
    }
    ApplyQuantOffset(out, offsetRows);
}

void NoteList::encode(WriteStream& out, bool removeOffset) const {
    int offsetRows = 0;
    out.writeNum(myNum);
    if (removeOffset && myNum > 0) {
        offsetRows = -myNotes[0].row;
    }
    for (int i = 0; i < myNum; ++i) {
        EncodeNote(out, myNotes[i], offsetRows);
    }
}

void NoteList::encode(WriteStream& out, const TimingData& timing,
                      bool removeOffset) {
    double offsetTime = 0;
    TempoTimeTracker tracker(timing);
    out.writeNum(myNum);
    if (removeOffset && myNum > 0) {
        offsetTime = -timing.rowToTime(myNotes[0].row);
    }
    for (int i = 0; i < myNum; ++i) {
        EncodeNote(out, myNotes[i], tracker, offsetTime);
    }
}

void NoteList::decode(ReadStream& in, int offsetRows) {
    Note note;
    uint32_t num = in.readNum();
    for (uint32_t i = 0; i < num; ++i) {
        DecodeNote(in, note, offsetRows);
        append(note);
    }
}

void NoteList::decode(ReadStream& in, const TimingData& timing,
                      double offsetTime) {
    TempoRowTracker tracker(timing);
    Note note;
    uint32_t num = in.readNum();
    for (uint32_t i = 0; i < num; ++i) {
        DecodeNote(in, note, tracker, offsetTime);
        append(note);
    }
}

// ================================================================================================
// NoteList :: memory management.

void NoteList::myReserve(int num) {
    int numBytes = num * sizeof(Note);
    if (myCap < numBytes) {
        myCap = max(numBytes, myCap << 1);
        myNotes = (Note*)realloc(myNotes, myCap);
    }
}

};  // namespace Vortex