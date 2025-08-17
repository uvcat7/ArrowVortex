#pragma once

#include <Simfile/SegmentList.h>

namespace Vortex {

struct SegmentEdit;
struct SegmentEditResult;

class SegmentGroup {
   public:
    SegmentGroup();
    ~SegmentGroup();

    // Returns a pointer to the start of the segment list for the given type.
    template <typename T>
    const T* begin() const {
        return (const T*)(myLists[T::TYPE].rawBegin());
    }

    // Returns a pointer to the end of the segment list for the given type.
    template <typename T>
    const T* end() const {
        return (const T*)(myLists[T::TYPE].rawEnd());
    }

    // Appends a segment to the back of the target list.
    template <typename T>
    void append(const T& segment) {
        myLists[T::TYPE].append(&segment);
    }

    // Inserts a segment into the target list.
    template <typename T>
    void insert(const T& segment) {
        myLists[T::TYPE].insert(&segment);
    }

    // Returns the most recent segment that occurred on or before the given row.
    // If no segment occured, a default constucted segment is returned.
    template <typename T>
    T getRecent(int row) const {
        const T* it = (const T*)myLists[T::TYPE].find(row);
        return it ? T(*it) : T();
    }

    // Returns the segment that occurs on the given row.
    // If no segment occurs, a default constucted segment is returned.
    template <typename T>
    T getRow(int row) const {
        const T* it = (const T*)myLists[T::TYPE].find(row);
        return (it && it->row == row) ? T(*it) : T();
    }

    // Returns the list of the given segment type.
    template <typename T>
    const SegmentList& getList() const {
        return myLists[T::TYPE];
    }

    // Removes all segments.
    void clear();

    // Removes segments with negative rows.
    void cleanup();

    // Removes invalid segments (e.g. unsorted, overlapping, redundant, invalid
    // row).
    void sanitize(const Chart* owner = nullptr);

    // Prepares a modification. The input is a list of segments which should be
    // added and removed. The output is a list of segments that actually end up
    // being added and removed.
    void prepareEdit(const SegmentEdit& in, SegmentEditResult& out,
                     bool clearRegion);

    // Appends a default constructed segment of the given type at the given row.
    void append(Segment::Type type, int row);

    // Appends a default constructed segment of the given type at the given row.
    void append(Segment::Type type, const Segment* seg);

    // Merges the segments in add into this group.
    void insert(const SegmentGroup& add);

    // Removes the segments in rem from this group.
    void remove(const SegmentGroup& rem);

    // Encodes the segment data and writes it to a bytestream.
    void encode(WriteStream& out) const;

    // Reads encoded segment data from a bytestream and inserts it.
    void decode(ReadStream& in);

    // Returns a description of the segments (e.g. "2 stops", "3 timing
    // segments").
    std::string description() const;

    // Returns a description of the segment values.
    std::string descriptionValues() const;

    /// Returns the total number of segments contained in the group.
    int numSegments() const;

    /// Returns the number of segment types for which the group contains
    /// segments.
    int numTypes() const;

    // Returns a pointer to the begin of the segment lists.
    SegmentList* begin();

    // Returns a pointer to the begin of the segment lists.
    const SegmentList* begin() const;

    // Returns a pointer to the end of the segment lists.
    SegmentList* end();

    // Returns a pointer to the end of the segment lists.
    const SegmentList* end() const;

   private:
    SegmentList myLists[Segment::NUM_TYPES];
};

struct SegmentEdit {
    SegmentGroup add, rem;
};

struct SegmentEditResult {
    SegmentGroup add, rem;
};

};  // namespace Vortex