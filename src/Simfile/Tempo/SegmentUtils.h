#pragma once

#include <Simfile/Tempo/SegmentTrack.h>

namespace AV {

template <typename T>
inline SegmentChanges<T> SetSegment(PersistentSegmentTrack<T>& segments, Row row, const T& value)
{
	return segments.modify(SegmentModification<T>{ {row, value} });
}

template <typename T>
inline SegmentChanges<T> SetSegment(IsolatedSegmentTrack<T>& segments, Row row, const T& value)
{
	return segments.modify(SegmentModification<T>{ {row, value} });
}

} // namespace AV
