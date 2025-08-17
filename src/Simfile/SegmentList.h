#pragma once

#include <Core/NonCopyable.h>

#include <Simfile/Segments.h>

namespace Vortex {

// ================================================================================================
// Segment iterators.

struct SegmentIter {
    inline void operator--() { ptr = (Segment*)(((uint8_t*)ptr) - stride); }
    inline void operator++() { ptr = (Segment*)(((uint8_t*)ptr) + stride); }
    inline bool operator!=(const SegmentIter& other) {
        return (ptr != other.ptr);
    }
    inline bool operator==(const SegmentIter& other) {
        return (ptr == other.ptr);
    }
    inline Segment* operator->() { return ptr; }
    Segment* ptr;
    uint32_t stride;
};

struct SegmentConstIter {
    inline void operator--() {
        ptr = (const Segment*)(((const uint8_t*)ptr) - stride);
    }
    inline void operator++() {
        ptr = (const Segment*)(((const uint8_t*)ptr) + stride);
    }
    inline bool operator!=(const SegmentConstIter& other) {
        return (ptr != other.ptr);
    }
    inline bool operator==(const SegmentConstIter& other) {
        return (ptr == other.ptr);
    }
    inline const Segment* operator->() { return ptr; }
    const Segment* ptr;
    uint32_t stride;
};

// ================================================================================================
// SegmentList.

class SegmentList {
   public:
    typedef SegmentList List;

    ~SegmentList();
    SegmentList();
    SegmentList(List&&);
    SegmentList(const List&);

    List& operator=(List&&);
    List& operator=(const List&);

    // Sets the type of the stored segments.
    void setType(Segment::Type type);

    // Destroys all stored segments.
    void clear();

    // Removes segments with negative rows.
    void cleanup();

    // Removes all segments that are invalid in some way (bad values,
    // overlapping, out-of-order, etc).
    void sanitize(const Chart* owner);

    // Returns last segment that occured before or on the given row.
    const Segment* find(int row) const;

    // Returns the number of stored segments.
    inline int size() const { return myNum; }

    // Returns the offset in bytes between each segment.
    inline int stride() const { return myStride; }

    // Returns true if the list is empty, false otherwise.
    inline bool empty() const { return (myNum == 0); }

    // Returns the segment type of the stored segments.
    inline Segment::Type type() const { return myType; }

    // Returns an iterator to the begin of the segment list.
    SegmentIter begin();

    // Returns a const iterator to the begin of the segment list.
    SegmentConstIter begin() const;

    // Returns an iterator to the end of the segment list.
    SegmentIter end();

    // Returns a const iterator to the end of the segment list.
    SegmentConstIter end() const;

    // Returns an iterator to the reverse begin of the segment list.
    SegmentIter rbegin();

    // Returns a const iterator to the reverse begin of the segment list.
    SegmentConstIter rbegin() const;

    // Returns an iterator to the reverse end of the segment list.
    SegmentIter rend();

    // Returns a const iterator to the reverse end of the segment list.
    SegmentConstIter rend() const;

   private:
    friend class SegmentGroup;

    // Returns a pointer to the begin of the segment data.
    const uint8_t* rawBegin() const { return mySegs; }

    // Returns a pointer to the end of the segment data.
    const uint8_t* rawEnd() const { return mySegs + myNum * myStride; }

    // Replaces the contents with a copy of the given list.
    void assign(const List& other);

    // Appends a segment to the back of this list.
    void append(int row);

    // Appends a segment to the back of this list.
    void append(const Segment* seg);

    // Inserts a segment into this list.
    void insert(const Segment* seg);

    // Inserts segments from the insert list into this list.
    void insert(const List& insert);

    // Removes all segments that match the segments in the remove list.
    void remove(const List& remove);

    // Prepares a modification, see SegmentGroup.
    void prepareEdit(const List& inAdd, const List& inRem, List& outAdd,
                     List& outRem, int regionBegin, int regionEnd);

   private:
    void myReserve(int num);

    uint8_t* mySegs;
    int myNum, myStride, myCap;
    Segment::Type myType;
};

};  // namespace Vortex