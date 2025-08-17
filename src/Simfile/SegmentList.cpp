#include <Simfile/SegmentList.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/Debug.h>

#include <Simfile/Chart.h>

#include <stdlib.h>

namespace Vortex {
namespace {

inline Segment* ofs(void* pos, int offset) {
    return (Segment*)(((uint8_t*)pos) + offset);
}

inline const Segment* ofs(const void* pos, int offset) {
    return (const Segment*)(((const uint8_t*)pos) + offset);
}

inline int diff(const Segment* begin, const Segment* end) {
    return ((const uint8_t*)end) - ((const uint8_t*)begin);
}

};  // anonymous namespace.

// ================================================================================================
// SegmentList :: destructor and constructors.

SegmentList::~SegmentList() {
    clear();
    free(mySegs);
}

SegmentList::SegmentList()
    : mySegs(nullptr),
      myNum(0),
      myStride(Segment::meta[Segment::BPM]->stride),
      myCap(0),
      myType(Segment::BPM) {}

SegmentList::SegmentList(List&& l)
    : mySegs(l.mySegs),
      myNum(l.myNum),
      myStride(l.myStride),
      myCap(l.myCap),
      myType(l.myType) {
    l.myNum = 0;
    l.mySegs = nullptr;
}

SegmentList::SegmentList(const List& list)
    : mySegs(nullptr),
      myNum(0),
      myStride(list.myStride),
      myCap(0),
      myType(list.myType) {
    assign(list);
}

SegmentList& SegmentList::operator=(List&& l) {
    mySegs = l.mySegs;
    myNum = l.myNum;
    myStride = l.myStride;
    myCap = l.myCap;
    myType = l.myType;

    l.myNum = 0;
    l.mySegs = nullptr;

    return *this;
}

SegmentList& SegmentList::operator=(const List& list) {
    assign(list);
    return *this;
}

// ================================================================================================
// SegmentList :: destructor and constructors.

void SegmentList::setType(Segment::Type type) {
    clear();

    myType = type;
    myStride = Segment::meta[type]->stride;
}

// ================================================================================================
// SegmentList :: removal.

void SegmentList::clear() {
    if (myNum > 0) {
        auto meta = Segment::meta[myType];
        for (int i = 0; i < myNum; ++i) {
            uint8_t* seg = mySegs + i * myStride;
            meta->destruct((Segment*)seg);
        }
    }
    myNum = 0;
}

void SegmentList::cleanup() {
    auto read = ofs(mySegs, 0);
    auto end = ofs(mySegs, myNum * myStride);
    auto meta = Segment::meta[myType];

    // Skip over valid segments until we arrive at the first invalid segment.
    while (read != end && read->row >= 0) {
        read = ofs(read, myStride);
    }
    auto write = read;

    // Then, start removing blocks of invalid segments and start moving blocks
    // of valid segments.
    while (read != end) {
        // Remove a block of invalid segments.
        while (read != end && read->row < 0) {
            meta->destruct(read);
            read = ofs(read, myStride);
            --myNum;
        }

        // Move a block of valid segments.
        auto moveBegin = read;
        while (read != end && read->row >= 0) {
            read = ofs(read, myStride);
        }
        int numBytes = diff(moveBegin, read);
        memmove(write, moveBegin, numBytes);
        write = ofs(write, numBytes);
    }
}

void SegmentList::sanitize(const Chart* owner) {
    auto meta = Segment::meta[myType];
    const Segment* prev = nullptr;
    int row = -1, numOverlap = 0, numUnsorted = 0, numRedundant = 0;
    for (auto seg = begin(), segEnd = end(); seg != segEnd; ++seg) {
        if (seg->row <= row) {
            numOverlap += (seg->row == row);
            numUnsorted += (seg->row < row);
            seg->row = -1;
        } else if (meta->isRedundant(seg.ptr, prev)) {
            ++numRedundant;
            seg->row = -1;
        } else {
            prev = seg.ptr;
            row = seg->row;
        }
    }

    if (numOverlap + numUnsorted + numRedundant > 0) {
        cleanup();
        std::string suffix;
        if (owner) {
            Str::append(suffix, " from ");
            Str::append(suffix, owner->description());
        }
        if (numOverlap > 0) {
            const char* segName =
                (numOverlap > 1 ? meta->plural : meta->singular);
            HudNote("Removed %i overlapping %s%s.", numOverlap, segName,
                    suffix.c_str());
        }
        if (numRedundant > 0) {
            const char* segName =
                (numRedundant > 1 ? meta->plural : meta->singular);
            HudNote("Removed %i redundant %s%s.", numRedundant, segName,
                    suffix.c_str());
        }
        if (numUnsorted > 0) {
            const char* segName =
                (numUnsorted > 1 ? meta->plural : meta->singular);
            HudNote("Removed %i out of order %s%s.", numUnsorted, segName,
                    suffix.c_str());
        }
    }
}

// ================================================================================================
// SegmentList :: manipulation.

void SegmentList::assign(const List& list) {
    clear();

    myNum = list.myNum;
    myReserve(myNum);
    myStride = list.myStride;
    myType = list.myType;

    int stride = myStride;
    auto meta = Segment::meta[myType];
    auto it = ofs(mySegs, 0), read = ofs(list.mySegs, 0);
    auto end = ofs(mySegs, myNum * stride);
    while (it != end) {
        meta->construct(it);
        meta->copy(it, read);
        it = ofs(it, stride);
        read = ofs(read, stride);
    }
}

void SegmentList::append(int row) {
    auto meta = Segment::meta[myType];
    int pos = myNum;
    myReserve(++myNum);
    auto it = ofs(mySegs, pos * myStride);
    meta->construct(it);
    it->row = row;
}

void SegmentList::append(const Segment* seg) {
    auto meta = Segment::meta[myType];
    int pos = myNum;
    myReserve(++myNum);
    auto it = ofs(mySegs, pos * myStride);
    meta->construct(it);
    meta->copy(it, seg);
}

void SegmentList::insert(const Segment* seg) {
    auto meta = Segment::meta[myType];
    int pos = myNum, row = seg->row, stride = myStride;
    const Segment* prev = ofs(mySegs, (pos - 1) * stride);
    while (pos > 0 && prev->row >= row) {
        prev = ofs(prev, -stride), --pos;
    }

    if (pos == myNum) {
        myReserve(++myNum);
        auto it = ofs(mySegs, pos * stride);
        meta->construct(it);
        meta->copy(it, seg);
    } else {
        Segment* cur = ofs(mySegs, pos * stride);
        if (cur->row != row) {
            int tail = myNum - pos;
            myReserve(++myNum);
            cur = ofs(mySegs, pos * stride);
            memmove(ofs(cur, stride), cur, tail * stride);
            meta->construct(cur);
        }
        meta->copy(cur, seg);
    }
}

void SegmentList::insert(const List& insert) {
    if (insert.myNum == 0) return;

    auto meta = Segment::meta[myType];
    int stride = myStride;

    int newSize = myNum + insert.myNum;
    myReserve(newSize);

    // Work backwards, that way insertion can be done on the fly.
    auto write = ofs(mySegs, stride * (newSize - 1));

    auto read = ofs(mySegs, stride * (myNum - 1));
    auto readEnd = ofs(mySegs, -stride);

    auto ins = ofs(insert.mySegs, stride * (insert.myNum - 1));
    auto insEnd = ofs(insert.mySegs, -stride);

    while (read != readEnd) {
        // Copy segments from the insert list.
        while (ins != insEnd && ins->row > read->row) {
            meta->construct(write);
            meta->copy(write, ins);
            write = ofs(write, -stride);
            ins = ofs(ins, -stride);
        }
        if (ins == insEnd) break;

        // Move existing segments.
        while (read != readEnd && read->row >= ins->row) {
            memmove(write, read, stride);
            write = ofs(write, -stride);
            read = ofs(read, -stride);
        }
    }

    // After read end is reached, there might still be some segments that need
    // to be inserted.
    while (ins != insEnd) {
        meta->construct(write);
        meta->copy(write, ins);
        write = ofs(write, -stride);
        ins = ofs(ins, -stride);
    }

    myNum = newSize;
}

void SegmentList::remove(const List& remove) {
    if (remove.myNum == 0) return;

    // Work forwards from the first segment.
    auto it = ofs(mySegs, 0);
    auto itEnd = ofs(mySegs, myNum * myStride);

    auto rem = ofs(remove.mySegs, 0);
    auto remEnd = ofs(remove.mySegs, remove.myNum * myStride);

    // Invalidate the segments that have a matching segment in the remove
    // vector.
    while (rem != remEnd) {
        while (it != itEnd && it->row < rem->row) {
            it = ofs(it, myStride);
        }
        if (it != itEnd && it->row == rem->row) {
            it->row = -1;
        }
        rem = ofs(rem, myStride);
    }
    cleanup();
}

// ================================================================================================
// SegmentList :: preparation operations.

static void VerifyAdd(Segment::Type type, const Segment* add,
                      const Segment*& prev, int& prevRow) {
    auto meta = Segment::meta[type];
    if (add->row <= prevRow) {
        if (add->row == prevRow) {
            HudWarning(
                "Bug: added segment is on the same row as the previous "
                "segment.");
        } else {
            HudWarning(
                "Bug: added segment is on a row before the previous segment.");
        }
    }
    if (meta->isRedundant(add, prev)) {
        HudWarning("Bug: added segment is redundant.");
    }
    prev = add;
    prevRow = add->row;
}

void SegmentList::prepareEdit(const List& inAdd, const List& inRem,
                              List& outAdd, List& outRem, int regionBegin,
                              int regionEnd) {
    VortexAssert(inAdd.type() == myType);
    VortexAssert(inRem.type() == myType);
    VortexAssert(outAdd.type() == myType);
    VortexAssert(outRem.type() == myType);

    outAdd.clear();
    outRem.clear();

    auto meta = Segment::meta[myType];

    auto it = begin(), itEnd = end();
    auto add = inAdd.begin(), addEnd = inAdd.end();
    auto rem = inRem.begin(), remEnd = inRem.end();

    int nextAddRow = (add != addEnd) ? add->row : INT_MAX;
    int nextRemRow = (rem != remEnd) ? rem->row : INT_MAX;

    const Segment* prev = nullptr;
    int prevRow = -1;

    for (; it != itEnd; ++it) {
        int row = it->row;
        bool remove = false;
        bool keep = false;

        // Add all segments on the to-be-added list that occur before this
        // segment.
        while (nextAddRow < row) {
            if (nextAddRow > prevRow && !meta->isRedundant(add.ptr, prev)) {
                outAdd.append(add.ptr);
                VerifyAdd(myType, add.ptr, prev, prevRow);
            }
            ++add;
            nextAddRow = (add != addEnd) ? add->row : INT_MAX;
        }

        // Check if the next segment on the to-be-added list is on the same row
        // as this segment.
        if (nextAddRow == row) {
            // If the added segment is equivalent to this segment, prevent
            // removal of the segment.
            if (meta->isEquivalent(it.ptr, add.ptr)) {
                keep = true;
            } else  // The added segment is different and overwrites this
                    // segment.
            {
                if (!meta->isRedundant(add.ptr, prev)) {
                    outAdd.append(add.ptr);
                    VerifyAdd(myType, add.ptr, prev, prevRow);
                }
                remove = true;
            }
        }

        // Check if this segment has become redundant due to previous
        // additions/removals.
        if (meta->isRedundant(it.ptr, prev)) {
            remove = true;
        } else if (!keep) {
            // Check if this segment intersects the modified region.
            if (it->row >= regionBegin && it->row <= regionEnd) {
                remove = true;
            }
            // Check if this segment appears on the to-be-removed list.
            else {
                while (nextRemRow < row) {
                    ++rem;
                    nextRemRow = (rem != remEnd) ? rem->row : INT_MAX;
                }
                if (nextRemRow == row) {
                    remove = true;
                }
            }
        }

        // At this point we finally know whether or not to remove the segment.
        if (remove) {
            outRem.append(it.ptr);
        } else {
            prev = it.ptr;
            prevRow = prev->row;
        }
    }

    // Remaining segments on the to-be-added list are appended at the end.
    for (; add != addEnd; ++add) {
        if (!meta->isRedundant(add.ptr, prev)) {
            outAdd.append(add.ptr);
            VerifyAdd(myType, add.ptr, prev, prevRow);
        }
    }
}

// ================================================================================================
// SegmentList :: iterators.

SegmentIter SegmentList::begin() {
    return {(Segment*)mySegs, (uint32_t)myStride};
}

SegmentConstIter SegmentList::begin() const {
    return {(const Segment*)mySegs, (uint32_t)myStride};
}

SegmentIter SegmentList::end() {
    return {(Segment*)(mySegs + myNum * myStride), (uint32_t)myStride};
}

SegmentConstIter SegmentList::end() const {
    return {(const Segment*)(mySegs + myNum * myStride), (uint32_t)myStride};
}

SegmentIter SegmentList::rbegin() {
    return {(Segment*)(mySegs + (myNum - 1) * myStride), (uint32_t)myStride};
}

SegmentConstIter SegmentList::rbegin() const {
    return {(const Segment*)(mySegs + (myNum - 1) * myStride),
            (uint32_t)myStride};
}

SegmentIter SegmentList::rend() {
    return {(Segment*)(mySegs - myStride), (uint32_t)myStride};
}

SegmentConstIter SegmentList::rend() const {
    return {(const Segment*)(mySegs - myStride), (uint32_t)myStride};
}

// ================================================================================================
// SegmentList :: binary search.

const Segment* SegmentList::find(int row) const {
    if (myNum == 0) return nullptr;

    int step, count = myNum;
    const uint8_t *it = mySegs, *mid;
    while (count > 1) {
        step = count >> 1;
        mid = it + step * myStride;
        if (((const Segment*)mid)->row <= row) {
            it = mid;
            count -= step;
        } else {
            count = step;
        }
    }
    auto out = (const Segment*)it;

    return (out->row <= row) ? out : nullptr;
}

// ================================================================================================
// SegmentList :: memory management.

void SegmentList::myReserve(int num) {
    int numBytes = num * myStride;
    if (myCap < numBytes) {
        myCap = max(numBytes, myCap << 1);
        mySegs = (uint8_t*)realloc(mySegs, myCap);
    }
}

};  // namespace Vortex