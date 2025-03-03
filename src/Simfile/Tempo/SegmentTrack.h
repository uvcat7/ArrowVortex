#pragma once

#include <Precomp.h>

namespace AV {

// A row/value pair.
template <typename T>
struct SegmentRow { Row row; T value; };

// A change pair with before/after values.
template <typename T>
struct SegmentChange { T before, after; };

// An intended modification for a segment track.
template <typename T>
using SegmentModification = std::map<int, T>;

// The changes resulting from a modification to a segment track.
template <typename T>
using SegmentChanges = std::map<int, SegmentChange<T>>;

// A track of segments that have a persistent effect from the current beat onwards (e.g. BPM change).
template <typename T>
class PersistentSegmentTrack
{
public:
	typedef SegmentRow<T> Segment;

	PersistentSegmentTrack();

	SegmentChanges<T> modify(const SegmentModification<T>& modification);
	void reapply(const SegmentChanges<T>& changes);
	void undo(const SegmentChanges<T>& changes);

	const T& get(Row row) const;

	inline const Segment* begin() const { return mySegments.data(); }
	inline const Segment* end() const { return mySegments.data() + mySegments.size(); }

private:
	vector<Segment> mySegments;
};

// A track of segments that have an isolated effect at the current beat (e.g. stop).
template <typename T>
class IsolatedSegmentTrack
{
public:
	typedef SegmentRow<T> Segment;

	SegmentChanges<T> modify(const SegmentModification<T>& modification);
	void reapply(const SegmentChanges<T>& changes);
	void undo(const SegmentChanges<T>& changes);

	const T* get(Row row) const;

	inline const Segment* begin() const { return mySegments.data(); }
	inline const Segment* end() const { return mySegments.data() + mySegments.size(); }

private:
	vector<Segment> mySegments;
};

} // namespace AV
