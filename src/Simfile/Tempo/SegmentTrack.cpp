#include <Precomp.h>

#include <Simfile/Tempo/SegmentTrack.h>
#include <Simfile/Tempo/SegmentTypes.h>
#include <Simfile/Tempo.h>

namespace AV {

using namespace std;

// Utility functions.

template <typename T>
inline bool Cmp(const SegmentRow<T>& segment, Row row) { return segment.row < row; }

template <typename T>
inline bool Cmp2(Row row, const SegmentRow<T>& segment) { return row < segment.row; }

// =====================================================================================================================
// PersistentSegmentTrack.

template <typename T>
PersistentSegmentTrack<T>::PersistentSegmentTrack()
	: mySegments{ { 0, T::fallback } }
{
}

template <typename T>
SegmentChanges<T> PersistentSegmentTrack<T>::modify(const SegmentModification<T>& modification)
{
	SegmentChanges<T> result;

	size_t i = 0;

	T currentValue = mySegments[0].value;

	for (auto& [targetRow, targetValue] : modification)
	{
		while (i < mySegments.size() && mySegments[i].row < targetRow)
		{
			currentValue = mySegments[i].value;
			if (i > 0 && mySegments[i - 1].value == currentValue)
			{
				result[mySegments[i].row] = { currentValue, currentValue };
				mySegments.erase(mySegments.begin() + i);
			}
			else
			{
				++i;
			}
		}

		if (i < mySegments.size() && mySegments[i].row == targetRow)
		{
			if (mySegments[i].value != targetValue)
			{
				result[targetRow] = { mySegments[i].value, targetValue };
				mySegments[i].value = targetValue;
			}
		}
		else
		{
			if (currentValue != targetValue)
			{
				result[targetRow] = { currentValue, targetValue };
				mySegments.insert(mySegments.begin() + i, { targetRow, targetValue });
			}
		}
	}

	while (i < mySegments.size())
	{
		currentValue = mySegments[i].value;
		if (i > 0 && mySegments[i - 1].value == currentValue)
		{
			result[mySegments[i].row] = { currentValue, currentValue };
			mySegments.erase(mySegments.begin() + i);
		}
		else
		{
			++i;
		}
	}

	if (!result.empty())
		EventSystem::publish<Tempo::SegmentsChanged>();

	return result;
}

template <typename T>
void PersistentSegmentTrack<T>::reapply(const SegmentChanges<T>& result)
{
	SegmentModification<T> mod;

	for (auto& [row, change] : result)
		mod[row] = change.after;

	modify(mod);
}

template <typename T>
void PersistentSegmentTrack<T>::undo(const SegmentChanges<T>& result)
{
	SegmentModification<T> mod;

	for (auto& [row, change] : result)
		mod[row] = change.before;

	modify(mod);
}

template <typename T>
const T& PersistentSegmentTrack<T>::get(Row row) const
{
	auto begin = mySegments.begin();
	auto end = mySegments.end();
	auto it = upper_bound(begin, end, row, Cmp2<T>);
	return it == begin ? T::fallback : (it - 1)->value;
}

// =====================================================================================================================
// Isolated segment track.

template <typename T>
SegmentChanges<T> IsolatedSegmentTrack<T>::modify(const SegmentModification<T>& modification)
{
	SegmentChanges<T> result;

	size_t i = 0;

	for (auto& [targetRow, targetValue] : modification)
	{
		size_t size = mySegments.size();

		while (i < mySegments.size() && mySegments[i].row < targetRow) ++i;

		if (targetValue == T::none)
		{
			if (i < mySegments.size() && mySegments[i].row == targetRow)
			{
				result[targetRow] = { mySegments[i].value, T::none };
				mySegments.erase(mySegments.begin() + i);
			}
		}
		else
		{
			if (i < mySegments.size() && mySegments[i].row == targetRow)
			{
				if (mySegments[i].value != targetValue)
				{
					result[targetRow] = { mySegments[i].value, targetValue };
					mySegments[i].value = targetValue;
				}
			}
			else
			{
				result[targetRow] = { T::none, targetValue };
				mySegments.insert(mySegments.begin() + i, { targetRow, targetValue });
			}
		}
	}

	return result;
}

template <typename T>
void IsolatedSegmentTrack<T>::reapply(const SegmentChanges<T>& result)
{
	SegmentModification<T> mod;

	for (auto& [row, change] : result)
		mod[row] = change.after;

	modify(mod);
}

template <typename T>
void IsolatedSegmentTrack<T>::undo(const SegmentChanges<T>& result)
{
	SegmentModification<T> mod;

	for (auto& [row, change] : result)
		mod[row] = change.before;

	modify(mod);
}

template <typename T>
const T* IsolatedSegmentTrack<T>::get(Row row) const
{
	auto begin = mySegments.begin();
	auto end = mySegments.end();
	auto it = lower_bound(begin, end, row, Cmp<T>);
	return it != end && it->row == row ? &it->value : nullptr;
}

// =====================================================================================================================
// Template instantiations.

#define DEF_PERSISTENT(T) \
	template T::Track::PersistentSegmentTrack(); \
	template SegmentChanges<T> T::Track::modify(const SegmentModification<T>&); \
	template void T::Track::reapply(const SegmentChanges<T>&); \
	template void T::Track::undo(const SegmentChanges<T>&); \
	template const T& T::Track::get(int) const;

#define DEF_ISOLATED(T) \
	template SegmentChanges<T> T::Track::modify(const SegmentModification<T>&); \
	template void T::Track::reapply(const SegmentChanges<T>&); \
	template void T::Track::undo(const SegmentChanges<T>&); \
	template const T* T::Track::get(int) const;

DEF_PERSISTENT(BpmChangeSegment);
DEF_PERSISTENT(TimeSignatureSegment);
DEF_PERSISTENT(TickCountSegment);
DEF_PERSISTENT(ComboSegment);
DEF_PERSISTENT(SpeedSegment);
DEF_PERSISTENT(ScrollSegment);

DEF_ISOLATED(StopSegment);
DEF_ISOLATED(DelaySegment);
DEF_ISOLATED(WarpSegment);
DEF_ISOLATED(FakeSegment);
DEF_ISOLATED(LabelSegment);

} // namespace AV
