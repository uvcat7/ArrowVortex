#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Common/Serialize.h>

#include <Core/System/Debug.h>

#include <Simfile/Tempo/SegmentSet.h>

namespace AV {

using namespace std;
using namespace Util;

/*
* 
namespace SegmentSetCpp {

inline Segment* ofs(void* pos, int offset)
{
	return (Segment*)(((uchar*)pos) + offset);
}

inline const Segment* ofs(const void* pos, int offset)
{
	return (const Segment*)(((const uchar*)pos) + offset);
}

inline int diff(const Segment* begin, const Segment* end)
{
	return ((const uchar*)end) - ((const uchar*)begin);
}

typedef SegmentSet::SegmentList SegmentList;

} // namespace SegmentSetCpp
using namespace SegmentSetCpp;

// =====================================================================================================================
// SegmentList.

SegmentList::~SegmentList()
{
	clear();
	free(myList);
}

SegmentList::SegmentList(SegmentType type)
	: myList(nullptr)
	, myNum(0)
	, myCap(0)
	, myType(type)
	, myStride(Segment::getTypeData(type)->stride)
{
}

SegmentList::SegmentList(SegmentList&& l)
	: myList(l.myList)
	, myNum(l.myNum)
	, myStride(l.myStride)
	, myCap(l.myCap)
	, myType(l.myType)
{
	l.myNum = 0;
	l.myList = nullptr;
}

SegmentList::SegmentList(const SegmentList& list)
	: myList(nullptr)
	, myNum(0)
	, myStride(list.myStride)
	, myCap(0)
	, myType(list.myType)
{
	assign(list);
}

SegmentSet::SegmentList& SegmentSet::SegmentList::operator = (SegmentList&& l)
{
	myList = l.myList;
	myNum = l.myNum;
	myStride = l.myStride;
	myCap = l.myCap;
	myType = l.myType;

	l.myNum = 0;
	l.myList = nullptr;

	return *this;
}

SegmentList& SegmentList::operator = (const SegmentList& list)
{
	assign(list);
	return *this;
}

// =====================================================================================================================
// SegmentList :: API.

void SegmentList::reserve(int capacity)
{
	int numBytes = capacity * myStride;
	if (myCap < numBytes)
	{
		myCap = max(numBytes, myCap << 1);
		myList = (uchar*)realloc(myList, myCap);
	}
}

void SegmentList::clear()
{
	if (myNum > 0)
	{
		auto meta = Segment::getTypeData(myType);
		for (int i = 0; i < myNum; ++i)
		{
			uchar* seg = myList + i * myStride;
			meta->destruct((Segment*)seg);
		}
		myNum = 0;
	}
}

void SegmentList::assign(const SegmentList& list)
{
	clear();

	myNum = list.myNum;
	reserve(myNum);

	myStride = list.myStride;
	myType = list.myType;

	int stride = myStride;
	auto meta = Segment::getTypeData(myType);
	auto it = ofs(myList, 0), read = ofs(list.myList, 0);
	auto end = ofs(myList, myNum * stride);
	while (it != end)
	{
		meta->construct(it);
		meta->copy(it, read);
		it = ofs(it, stride);
		read = ofs(read, stride);
	}
}

void SegmentList::swap(SegmentList& other)
{
	std::swap(other.myList, myList);
	std::swap(other.myNum, myNum);
	std::swap(other.myStride, myStride);
	std::swap(other.myCap, myCap);
	std::swap(other.myType, myType);
}

void SegmentList::append(Row row)
{
	auto meta = Segment::getTypeData(myType);
	int pos = myNum;
	reserve(++myNum);
	auto it = ofs(myList, pos * myStride);
	meta->construct(it);
	it->row = row;
}

void SegmentList::append(const Segment* seg)
{
	auto meta = Segment::getTypeData(myType);
	int pos = myNum;
	reserve(++myNum);
	auto it = ofs(myList, pos * myStride);
	meta->construct(it);
	meta->copy(it, seg);
}

void SegmentList::insert(const Segment* seg)
{
	auto meta = Segment::getTypeData(myType);
	int pos = myNum, row = seg->row, stride = myStride;
	const Segment* prev = ofs(myList, (pos - 1) * stride);
	while (pos > 0 && prev->row >= row)
	{
		prev = ofs(prev, -stride), --pos;
	}

	if (pos == myNum)
	{
		reserve(++myNum);
		auto it = ofs(myList, pos * stride);
		meta->construct(it);
		meta->copy(it, seg);
	}
	else
	{
		Segment* cur = ofs(myList, pos * stride);
		if (cur->row != row)
		{
			int tail = myNum - pos;
			reserve(++myNum);
			cur = ofs(myList, pos * stride);
			memmove(ofs(cur, stride), cur, tail * stride);
			meta->construct(cur);
		}
		meta->copy(cur, seg);
	}
}

void SegmentList::insert(const SegmentList& insert)
{
	if (insert.myNum == 0) return;

	auto meta = Segment::getTypeData(myType);
	int stride = myStride;

	int newSize = myNum + insert.myNum;
	reserve(newSize);

	// Work backwards, that way insertion can be done on the fly.
	auto write = ofs(myList, stride * (newSize - 1));

	auto read = ofs(myList, stride * (myNum - 1));
	auto readEnd = ofs(myList, -stride);

	auto ins = ofs(insert.myList, stride * (insert.myNum - 1));
	auto insEnd = ofs(insert.myList, -stride);

	while (read != readEnd)
	{
		// Copy segments from the insert list.
		while (ins != insEnd && ins->row > read->row)
		{
			meta->construct(write);
			meta->copy(write, ins);
			write = ofs(write, -stride);
			ins = ofs(ins, -stride);
		}
		if (ins == insEnd) break;

		// Move existing segments.
		while (read != readEnd && read->row >= ins->row)
		{
			memmove(write, read, stride);
			write = ofs(write, -stride);
			read = ofs(read, -stride);
		}
	}

	// After read end is reached, there might still be some segments that need to be inserted.
	while (ins != insEnd)
	{
		meta->construct(write);
		meta->copy(write, ins);
		write = ofs(write, -stride);
		ins = ofs(ins, -stride);
	}

	myNum = newSize;
}

void SegmentList::remove(const SegmentList& remove)
{
	if (remove.myNum == 0) return;

	// Work forwards from the first segment.
	auto it = ofs(myList, 0);
	auto itEnd = ofs(myList, myNum * myStride);

	auto rem = ofs(remove.myList, 0);
	auto remEnd = ofs(remove.myList, remove.myNum * myStride);

	// Invalidate the segments that have a matching segment in the remove vector.
	while (rem != remEnd)
	{
		while (it != itEnd && it->row < rem->row)
		{
			it = ofs(it, myStride);
		}
		if (it != itEnd && it->row == rem->row)
		{
			it->row = -1;
		}
		rem = ofs(rem, myStride);
	}

	removeMarkedSegments();
}

void SegmentList::removeMarkedSegments()
{
	auto read = ofs(myList, 0);
	auto end = ofs(myList, myNum * myStride);
	auto meta = Segment::getTypeData(myType);

	// Skip over valid segments until we arrive at the first invalid segment.
	while (read != end && read->row >= 0)
	{
		read = ofs(read, myStride);
	}
	auto write = read;

	// Then, start removing blocks of invalid segments and start moving blocks of valid segments.
	int oldNum = myNum;
	while (read != end)
	{
		// Remove a block of invalid segments.
		while (read != end && read->row < 0)
		{
			meta->destruct(read);
			read = ofs(read, myStride);
			--myNum;
		}

		// Move a block of valid segments.
		auto moveBegin = read;
		while (read != end && read->row >= 0)
		{
			read = ofs(read, myStride);
		}
		int numBytes = diff(moveBegin, read);
		memmove(write, moveBegin, numBytes);
		write = ofs(write, numBytes);
	}
}

void SegmentList::sanitize(const char* description)
{
	string source;

	if (!description) description = "Segment list";

	auto meta = Segment::getTypeData(myType);
	const Segment* prev = nullptr;
	Row row = -1, numOverlap = 0, numUnsorted = 0, numRedundant = 0;
	for (auto seg = begin(), segEnd = end(); seg != segEnd; ++seg)
	{
		if (seg->row <= row)
		{
			numOverlap += (seg->row == row);
			numUnsorted += (seg->row < row);
			seg->row = -1;
		}
		else if (meta->isRedundant(seg.ptr, prev))
		{
			++numRedundant;
			seg->row = -1;
		}
		else
		{
			prev = seg.ptr;
			row = seg->row;
		}
	}

	if (numOverlap + numUnsorted + numRedundant > 0)
	{
		if (numOverlap > 0)
		{
			const char* segName = (numOverlap > 1 ? meta->plural : meta->singular);
			HudNote("%s has %i overlapping %s.", description, numOverlap, segName);
		}
		if (numRedundant > 0)
		{
			const char* segName = (numRedundant > 1 ? meta->plural : meta->singular);
			HudNote("%s has %i redundant %s.", description, numRedundant, segName);
		}
		if (numUnsorted > 0)
		{
			const char* segName = (numUnsorted > 1 ? meta->plural : meta->singular);
			HudNote("%s has %i out of order %s.", description, numUnsorted, segName);
		}
		removeMarkedSegments();
	}
}

int SegmentList::endRow() const
{
	return myNum ? ((const Segment*)(myList + (myNum - 1) * myStride))->row : 0;
}

const Segment* SegmentList::find(Row row) const
{
	if (myNum == 0) return nullptr;

	int step, count = myNum;
	const uchar* it = myList, *mid;
	while (count > 1)
	{
		step = count >> 1;
		mid = it + step * myStride;
		if (((const Segment*)mid)->row <= row)
		{
			it = mid;
			count -= step;
		}
		else
		{
			count = step;
		}
	}
	auto out = (const Segment*)it;

	return (out->row <= row) ? out : nullptr;
}

// =====================================================================================================================
// SegmentSet.

SegmentSet::~SegmentSet()
{
}

SegmentSet::SegmentSet()
	: myLists
	{
		{SegmentType::STOP},
		{SegmentType::DELAY},
		{SegmentType::WARP},
		{SegmentType::TIME_SIGNATURE},
		{SegmentType::TICK_COUNT},
		{SegmentType::COMBO},
		{SegmentType::SPEED},
		{SegmentType::SCROLL},
		{SegmentType::FAKE},
		{SegmentType::LABEL}
	}
{
}

void SegmentSet::swap(SegmentSet& other)
{
	for (auto& segments : myLists)
	{
		segments.swap(other.myLists[(int)segments.myType]);
	}
}

void SegmentSet::clear()
{
	for (auto& segments : myLists)
	{
		segments.clear();
	}
}

void SegmentSet::append(SegmentType type, Row row)
{
	myLists[(int)type].append(row);
}

void SegmentSet::append(SegmentType type, const Segment* seg)
{
	myLists[(int)type].append(seg);
}

void SegmentSet::insert(const SegmentSet& add)
{
	for (auto& segments : myLists)
	{
		segments.insert(add.myLists[(int)segments.myType]);
	}
}

void SegmentSet::remove(const SegmentSet& rem)
{
	for (auto& segments : myLists)
	{
		segments.remove(rem.myLists[(int)segments.myType]);
	}
}

void SegmentSet::removeMarkedSegments()
{
	for (auto& segments : myLists)
	{
		segments.removeMarkedSegments();
	}
}

void SegmentSet::sanitize(const char* description)
{
	for (auto& segments : myLists)
	{
		segments.sanitize(description);
	}
}

void SegmentSet::encodeTo(Serializer& out) const
{
	for (auto& segments : myLists)
	{
		if (segments.size())
		{
			auto type = segments.myType;
			auto meta = Segment::getTypeData(type);
			out.writeNum(segments.size());
			out.writeNum((size_t)type);
			for (auto& segment : segments)
			{
				meta->encode(out, &segment);
			}
		}
	}
	out.writeNum(0);
}

void SegmentSet::decodeFrom(Deserializer& in)
{
	uint num = in.readNum();
	while (num > 0)
	{
		size_t type = in.readNum();
		if (type < (size_t)SegmentType::COUNT)
		{
			auto meta = Segment::getTypeData((SegmentType)type);
			for (; num > 0; --num) meta->decode(in, this);
		}
		else
		{
			in.invalidate();
		}
		num = in.readNum();
	}
}

static string GetSegmentDescription(uint num, const char* singular, const char* plural)
{
	switch(num)
	{
		case 0:  return "nothing";
		case 1:  return singular;
		default: return String::format("%1 %2").arg(num).arg(plural);
	}
}

string SegmentSet::description() const
{
	int numTypes = 0, numSegments = 0;
	SegmentType lastType = SegmentType::STOP;
	for (auto& segments : myLists)
	{
		numSegments += segments.size();
		numTypes += (segments.size() > 0);
		if (segments.size() > 0)
			lastType = (SegmentType)segments.myType;
	}

	if (numTypes <= 1)
	{
		auto meta = Segment::getTypeData(lastType);
		return GetSegmentDescription(numSegments, meta->singular, meta->plural);
	}

	return GetSegmentDescription(numSegments, "segment", "segments");
}

string SegmentSet::descriptionDetails() const
{
	for (auto& segments : myLists)
	{
		if (segments.size() > 0)
		{
			auto type = segments.myType;
			auto firstSegment = (const Segment*)segments.myList;
			return Segment::getTypeData(type)->getDescription(firstSegment);
		}
	}
	return {};
}

int SegmentSet::numSegments() const
{
	int numSegments = 0;
	for (auto& segments : myLists)
	{
		numSegments += segments.size();
	}
	return numSegments;
}

int SegmentSet::numTypes() const
{
	int result = 0;

	for (auto& segments : myLists)
		result += (segments.size() != 0);

	return result;
}

int SegmentSet::endRow() const
{
	int result = 0;

	for (auto& segments : myLists)
		result = max(result, segments.endRow());

	return result;
}

SegmentList* SegmentSet::begin()
{
	return myLists;
}

const SegmentList* SegmentSet::begin() const
{
	return myLists;
}

SegmentList* SegmentSet::end()
{
	return myLists + (size_t)SegmentType::COUNT;
}

const SegmentList* SegmentSet::end() const
{
	return myLists + (size_t)SegmentType::COUNT;
}

*/

} // namespace AV