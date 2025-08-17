#include <Simfile/SegmentGroup.h>

#include <Core/ByteStream.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>

namespace Vortex {

#define ForEachType(type)                                                  \
    for (Segment::Type type = (Segment::Type)0; type < Segment::NUM_TYPES; \
         type = (Segment::Type)(type + 1))

SegmentGroup::SegmentGroup() {
    ForEachType(type) { myLists[type].setType(type); }
}

SegmentGroup::~SegmentGroup() = default;

void SegmentGroup::clear() {
    ForEachType(type) { myLists[type].clear(); }
}

void SegmentGroup::cleanup() {
    ForEachType(type) { myLists[type].cleanup(); }
}

void SegmentGroup::sanitize(const Chart* owner) {
    ForEachType(type) { myLists[type].sanitize(owner); }
}

void SegmentGroup::prepareEdit(const SegmentEdit& in, SegmentEditResult& out,
                               bool clearRegion) {
    int regionBegin = INT_MAX;
    int regionEnd = 0;
    if (clearRegion) {
        ForEachType(type) {
            auto& list = in.add.myLists[type];
            if (list.size() >= 2) {
                auto first = list.begin();
                auto last = list.rbegin();
                if (last->row > first->row) {
                    regionBegin = min(regionBegin, first->row);
                    regionEnd = max(regionEnd, last->row);
                }
            }
        }
    }
    ForEachType(type) {
        myLists[type].prepareEdit(in.add.myLists[type], in.rem.myLists[type],
                                  out.add.myLists[type], out.rem.myLists[type],
                                  regionBegin, regionEnd);
    }
}

void SegmentGroup::append(Segment::Type type, int row) {
    myLists[type].append(row);
}

void SegmentGroup::append(Segment::Type type, const Segment* seg) {
    myLists[type].append(seg);
}

void SegmentGroup::insert(const SegmentGroup& add) {
    ForEachType(type) { myLists[type].insert(add.myLists[type]); }
}

void SegmentGroup::remove(const SegmentGroup& rem) {
    ForEachType(type) { myLists[type].remove(rem.myLists[type]); }
}

void SegmentGroup::encode(WriteStream& out) const {
    ForEachType(type) {
        auto& list = myLists[type];
        if (list.size()) {
            auto meta = Segment::meta[type];
            out.writeNum(list.size());
            out.write<uint8_t>(type);
            for (auto it = list.begin(), end = list.end(); it != end; ++it) {
                meta->encode(out, it.ptr);
            }
        }
    }
    out.writeNum(0);
}

void SegmentGroup::decode(ReadStream& in) {
    uint32_t num = in.readNum();
    while (num > 0) {
        uint8_t type = in.read<uint8_t>();
        if (type < Segment::NUM_TYPES) {
            auto meta = Segment::meta[type];
            for (; num > 0; --num) meta->decode(in, this);
        } else {
            in.invalidate();
        }
        num = in.readNum();
    }
}

static std::string GetSegmentDescription(uint32_t num, const char* singular,
                                         const char* plural) {
    switch (num) {
        case 0:
            return "nothing";
        case 1:
            return singular;
    }
    return Str::fmt("%1 %2").arg(num).arg(plural);
}

std::string SegmentGroup::description() const {
    int numTypes = 0, numSegments = 0, lastType = 0;
    ForEachType(type) {
        auto& list = myLists[type];
        numSegments += list.size();
        numTypes += (list.size() > 0);
        if (list.size() > 0) lastType = type;
    }

    if (numTypes <= 1) {
        auto meta = Segment::meta[lastType];
        return GetSegmentDescription(numSegments, meta->singular, meta->plural);
    }

    return GetSegmentDescription(numSegments, "segment", "segments");
}

std::string SegmentGroup::descriptionValues() const {
    int numTypes = 0, numSegments = 0, lastType = 0;
    ForEachType(type) {
        auto& list = myLists[type];
        numSegments += list.size();
        numTypes += (list.size() > 0);
        if (list.size() > 0) lastType = type;
    }

    if (numTypes <= 1) {
        Vector<std::string> info;

        auto& list = myLists[lastType];
        auto meta = Segment::meta[lastType];
        auto seg = list.begin(), segEnd = list.end();
        while (seg != segEnd) {
            info.push_back(meta->getDescription(seg.ptr));
            ++seg;
        }
        return Str::join(info, ", ");
    }

    return GetSegmentDescription(numSegments, "segment", "segments");
}

int SegmentGroup::numSegments() const {
    int numSegments = 0;
    for (auto it = myLists, end = myLists + Segment::NUM_TYPES; it != end;
         ++it) {
        numSegments += it->size();
    }
    return numSegments;
}

int SegmentGroup::numTypes() const {
    int numTypes = 0;
    for (auto it = myLists, end = myLists + Segment::NUM_TYPES; it != end;
         ++it) {
        numTypes += (it->size() != 0);
    }
    return numTypes;
}

SegmentList* SegmentGroup::begin() { return myLists; }

const SegmentList* SegmentGroup::begin() const { return myLists; }

SegmentList* SegmentGroup::end() { return myLists + Segment::NUM_TYPES; }

const SegmentList* SegmentGroup::end() const {
    return myLists + Segment::NUM_TYPES;
}

};  // namespace Vortex