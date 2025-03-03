#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Enum.h>
#include <Core/Common/Serialize.h>

#include <Core/System/Debug.h>

#include <Simfile/Common.h>
#include <Simfile/Simfile.h>
#include <Simfile/History.h>
#include <Simfile/Tempo.h>
#include <Simfile/Chart.h>

namespace AV {

using namespace std;
using namespace Util;

BpmRange::BpmRange(double min, double max)
	: min(min)
	, max(max)
{
}

DisplayBpm::DisplayBpm(double low, double high, DisplayBpmType type)
	: low(low)
	, high(high)
	, type(type)
{
}

const DisplayBpm DisplayBpm::actual(0, 0, DisplayBpmType::Actual);
const DisplayBpm DisplayBpm::random(0, 0, DisplayBpmType::Random);


template <>
const char* Enum::toString<DisplayBpmType>(DisplayBpmType value)
{
	switch (value)
	{
	case DisplayBpmType::Actual:
		return "actual";
	case DisplayBpmType::Random:
		return "random";
	case DisplayBpmType::Custom:
		return "custom";
	}
	return "unknown";
}

template <>
optional<DisplayBpmType> Enum::fromString<DisplayBpmType>(stringref str)
{
	if (str == "actual")
		return DisplayBpmType::Actual;
	if (str == "random")
		return DisplayBpmType::Random;
	if (str == "custom")
		return DisplayBpmType::Custom;
	return std::nullopt;
}

static void HandleOffsetChanged()
{
	EventSystem::publish<Tempo::OffsetChanged>();
	EventSystem::publish<Tempo::MetadataChanged>();
	EventSystem::publish<Tempo::TimingChanged>();
}

static void HandleDisplayBpmChanged()
{
	EventSystem::publish<Tempo::DisplayBpmChanged>();
}

Tempo::~Tempo()
{
}

Tempo::Tempo(Chart* chart)
	: offset(0)
	, displayBpm(EventSystem::getId<DisplayBpmChanged>(), DisplayBpm::actual)
	, chart(chart)
	, simfile(chart->simfile)
{
	updateTiming();
}

Tempo::Tempo(Simfile* simfile)
	: offset(0)
	, displayBpm(EventSystem::getId<DisplayBpmChanged>(), DisplayBpm::actual)
	, chart(nullptr)
	, simfile(simfile)
{
	updateTiming();
}

bool Tempo::hasSegments() const
{
	// TODO: fix
	// return (segments.numSegments() > 0);
	return true;
}

double Tempo::getBpm(Row row) const
{
	return bpmChanges.get(row).bpm;
}

void Tempo::sanitize()
{
	auto description = chart ? chart->getFullDifficulty() : "Simfile tempo";

	// TODO: fix
	// segments.sanitize(description.data());

	// Make sure there is a BPM change at row zero.
	/*auto bpmc = segments.begin<BpmChange>();
	auto end = segments.end<BpmChange>();
	if (bpmc == end || bpmc->row != 0)
	{
		double bpm = (bpmc != end) ? bpmc->bpm : (double)SimfileConstants::DefaultBpm;
		segments.insert(BpmChange(0, bpm));
		string suffix;
		if (chart)
		{
			suffix.append(" to ");
			suffix.append(chart->getFullDifficulty());
		}
		HudNote("Added an initial BPM of %.3f%s.", bpm, suffix.data());
	}
	*/
}

bool Tempo::isEquivalent(const Tempo& other)
{
	return false;
}

void Tempo::updateTiming()
{
	timing.update(offset.get(), *this);
}

// =====================================================================================================================
// Tempo :: history > set display BPM.

static string GetDescription(const DisplayBpm& bpm)
{
	if (bpm.type == DisplayBpmType::Actual)
	{
		return "actual";
	}
	else if (bpm.type == DisplayBpmType::Random)
	{
		return "random";
	}
	string out = String::fromDouble(bpm.low);
	if (bpm.low != bpm.high)
	{
		out += "-";
		String::appendDouble(out, bpm.high);
	}
	return out;
}

/*

// =====================================================================================================================
// Tempo :: history > modify segments.

static History::EditId ModifySegmentsId = 0;

static string ApplyModifySegments(History* history, Deserializer& data, bool undo)
{
	string msg;

	SegmentSet add, rem;
	add.decodeFrom(data);
	rem.decodeFrom(data);
	VortexAssert(data.success());

	auto tempo = history->getTempo();

	int numAdd = add.numSegments();
	int numRem = rem.numSegments();

	bool addMatchesRem = false;
	if (numAdd == numRem)
	{
		addMatchesRem = true;
		auto addList = add.begin(), addListEnd = add.end();
		auto remList = rem.begin(), remListEnd = rem.end();
		while (addList != addListEnd && remList != remListEnd)
		{
			if (addList->size() != remList->size())
			{
				addMatchesRem = false;
				break;
			}
			auto a = addList->begin(), aEnd = addList->end();
			auto r = remList->begin(), rEnd = remList->end();
			while (a != aEnd && r != rEnd)
			{
				if (a->row != r->row)
				{
					addMatchesRem = false;
					break;
				}
				++a, ++r;
			}
			++addList, ++remList;
		}
	}

	if (addMatchesRem)
	{
		msg += "Changed " + add.description();
		if (numAdd == 1 && numRem == 1)
		{
			msg += ": ";
			msg += rem.descriptionDetails();
			msg += " to ";
			msg += add.descriptionDetails();
		}
	}
	else
	{
		if (numAdd > 0)
		{
			msg += "Added " + add.description();
		}
		if (numRem > 0)
		{
			if (msg.length()) msg += ", ";
			msg += "Removed " + rem.description();
		}
		if (numAdd == 1 && numRem == 0)
		{
			msg += ": ";
			msg += add.descriptionDetails();
		}
		else if (numAdd == 0 && numRem == 1)
		{
			msg += ": ";
			msg += rem.descriptionDetails();
		}
	}

	if (undo)
	{
		tempo->remove(add);
		tempo->insert(rem);
	}
	else
	{
		tempo->remove(rem);
		tempo->insert(add);
	}

	return msg;
}

void QueueModifySegments(Tempo* tempo, TempoModification& mod)
{
	TempoModificationHelper::PrepareSet(tempo->segments, mod);
	if (mod.segmentsToAdd.numSegments() + mod.segmentsToRemove.numSegments() > 0)
	{
		Serializer data;
		mod.segmentsToAdd.encodeTo(data);
		mod.segmentsToRemove.encodeTo(data);
		GetHistory(tempo)->add(ModifySegmentsId, data);
	}
}

// =====================================================================================================================
// Tempo :: history > insert rows.

static History::EditId InsertRowsId = 0;

static void ApplyInsertRowsOffset(Tempo* tempo, int startRow, int numRows)
{
	for (auto& segments : tempo->segments)
	{
		for (auto& segment : segments)
		{
			if (segment.row >= startRow)
			{
				segment.row += numRows;
			}
		}
	}
}

static string ApplyInsertRows(History* history, Deserializer& data, bool undo)
{
	auto startRow = data.read<int>();
	auto numRows = data.read<int>();
	VortexAssert(data.success());

	if (undo) numRows = -numRows;

	auto tempo = history->getTempo();

	SegmentSet rem;
	rem.decodeFrom(data);

	// Apply positive offsets first, to make room for note insertion.
	if (numRows > 0)
	{
		ApplyInsertRowsOffset(tempo, startRow, numRows);
	}

	// Then, insert or remove segments.
	if (undo)
	{
		tempo->insert(rem);
	}
	else
	{
		tempo->remove(rem);
	}

	// Apply negative offsets after the notes are removed.
	if (numRows < 0)
	{
		ApplyInsertRowsOffset(tempo, startRow, numRows);
	}

	return string();
}

static void QueueInsertRows(Tempo* tempo, int startRow, int numRows)
{
	if (numRows != 0)
	{
		Serializer data;
		data.write<int>(startRow);
		data.write<int>(numRows);

		SegmentSet rem;
		if (numRows < 0)
		{
			Row endRow = startRow - numRows;
			for (auto& segments : tempo->segments)
			{
				auto type = segments.type();
				for (auto& seg : segments)
				{
					if (seg.row >= startRow && seg.row < endRow)
					{
						rem.append(type, &seg);
					}
				}
			}
		}
		rem.encodeTo(data);

		GetHistory(tempo)->add(InsertRowsId, data);
	}
}

*/

// =====================================================================================================================
// Tempo :: history functions.

static void SetDisplayBpm(Tempo* tempo, DisplayBpm newDisplayBpm)
{
	if (tempo->displayBpm.get() == newDisplayBpm)
		return;

	tempo->displayBpm.set(newDisplayBpm);
	EventSystem::publish<Tempo::MetadataChanged>();
}

} // namespace AV
