#include <Managers/TempoMan.h>

#include <math.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/VectorUtils.h>
#include <Core/ByteStream.h>

#include <Simfile/SegmentList.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/TimingData.h>
#include <Simfile/Encoding.h>

#include <Managers/SimfileMan.h>

#include <Editor/History.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/Selection.h>
#include <Editor/View.h>
#include <Editor/TempoBoxes.h>

#include <algorithm>

#define TEMPO_MAN ((TempoManImpl*)gTempo)

namespace Vortex {

// ================================================================================================
// TempoManImpl :: member data.

const char* TempoMan::clipboardTag = "tempo";

struct TempoManImpl : public TempoMan {

Tempo* myTempo;

Chart* myChart;
Simfile* mySimfile;
TimingData myTimingData;
double myInitialBpm;

int myTweakRow;
Tempo* myTweakTempo;
TweakMode myTweakMode;
double myTweakValue;

History::EditId myApplyOffsetId;
History::EditId myApplySegmentsId;
History::EditId myApplyInsertRowsId;
History::EditId myApplyDisplayBpmId;

// ================================================================================================
// TempoManImpl :: constructor and destructor.

~TempoManImpl()
{
}

TempoManImpl()
	: myTempo(nullptr)
	, mySimfile(nullptr)
	, myTweakTempo(nullptr)
	, myTweakMode(TWEAK_NONE)
{
	myUpdateTimingData();

	myApplyOffsetId     = gHistory->addCallback(ApplyOffset);
	myApplySegmentsId   = gHistory->addCallback(ApplySegments);
	myApplyInsertRowsId = gHistory->addCallback(ApplyInsertRows);
	myApplyDisplayBpmId = gHistory->addCallback(ApplyDisplayBpm);
}

// ================================================================================================
// TempoManImpl :: update functions.

void myUpdateTimingData()
{
	if(myTweakTempo)
	{
		myTimingData.update(myTweakTempo);
	}
	else if(myTempo)
	{
		myTimingData.update(myTempo);
	}
	else
	{
		myTimingData = TimingData();
	}

	if(gNotes) gNotes->updateTempo();

	gEditor->reportChanges(VCM_TEMPO_CHANGED);

}

void update(Simfile* sim, Chart* chart)
{
	myChart = chart;
	mySimfile = sim;

	Tempo* tempo = nullptr;
	if(chart && chart->hasTempo())
	{
		tempo = chart->getTempo(sim);
	}
	else if(sim)
	{
		tempo = sim->tempo;
	}

	if(myTempo != tempo)
	{
		stopTweaking(false);
		myTempo = tempo;
		myUpdateTimingData();
	}
}

// ================================================================================================
// TempoManImpl :: edit helper functions.

void myStartEdit(Tempo* tempo)
{
	if(myTempo == tempo)
	{
		stopTweaking(false);
	}
}

void myFinishEdit(Tempo* tempo)
{
	tempo->sanitize();
	if(myTempo == tempo)
	{
		myUpdateTimingData();
		gEditor->reportChanges(VCM_TEMPO_CHANGED);
	}
}

// ================================================================================================
// TempoManImpl :: apply segments.

void myQueueSegments(const SegmentEdit& edit)
{
	stopTweaking(false);
	SegmentEditResult result;
	myTempo->segments->prepareEdit(edit, result);
	if(result.add.numSegments() + result.rem.numSegments() > 0)
	{
		WriteStream stream;
		result.add.encode(stream);
		result.rem.encode(stream);
		gHistory->addEntry(myApplySegmentsId, stream.data(), stream.size(), myTempo);
	}
}

String myApplySegments(Tempo* out, ReadStream& in, bool undo, bool redo)
{
	String msg;
	SegmentGroup add, rem;
	add.decode(in);
	rem.decode(in);
	if(in.success())
	{
		int numAdd = add.numSegments();
		int numRem = rem.numSegments();

		bool addMatchesRem = false;
		if(numAdd == numRem)
		{
			addMatchesRem = true;
			auto addList = add.begin(), addListEnd = add.end();
			auto remList = rem.begin(), remListEnd = rem.end();
			while(addList != addListEnd && remList != remListEnd)
			{
				if(addList->size() != remList->size())
				{
					addMatchesRem = false;
					break;
				}
				auto a = addList->begin(), aEnd = addList->end();
				auto r = remList->begin(), rEnd = remList->end();
				while(a != aEnd && r != rEnd)
				{
					if(a->row != r->row)
					{
						addMatchesRem = false;
						break;
					}
					++a, ++r;
				}
				++addList, ++remList;
			}
		}

		if(addMatchesRem)
		{
			msg += "Changed " + add.description();
		}
		else
		{
			if(numAdd > 0)
			{
				msg += "Added " + add.description();
			}
			if(numRem > 0)
			{
				if(msg.len()) msg += ", ";
				msg += "Removed " + rem.description();
			}
		}

		myStartEdit(out);
		if(undo)
		{
			out->segments->remove(add);
			out->segments->insert(rem);
		}
		else
		{
			out->segments->remove(rem);
			out->segments->insert(add);
			
		}
		myFinishEdit(out);
	}
	return msg;
}

static String ApplySegments(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	return TEMPO_MAN->myApplySegments(bound.tempo, in, undo, redo);
}

// ================================================================================================
// TempoManImpl :: apply insert rows.

void myWriteInsertRows(WriteStream& stream, Tempo* tempo, int startRow, int numRows)
{
	SegmentEdit edit;
	SegmentEditResult result;
	if(numRows < 0)
	{
		int endRow = startRow - numRows;
		SegmentGroup* segs = tempo->segments;
		for(auto& list : *segs)
		{
			for(auto seg = list.begin(), end = list.end(); seg != end; ++seg)
			{
				if(seg->row >= startRow && seg->row < endRow)
				{
					edit.rem.append(list.type(), seg.ptr);
				}
			}
		}
		segs->prepareEdit(edit, result);
	}
	stream.write(tempo);
	result.rem.encode(stream);
}

void myQueueInsertRows(int startRow, int numRows, bool curChartOnly)
{
	if(mySimfile && numRows != 0)
	{
		WriteStream stream;
		stream.write<int>(startRow);
		stream.write<int>(numRows);

		if(curChartOnly)
		{
			if(myChart && myChart->hasTempo())
			{
				myWriteInsertRows(stream, myChart->tempo, startRow, numRows);
			}
		}
		else
		{
			for(auto chart : mySimfile->charts)
			{
				if(chart->hasTempo())
				{
					myWriteInsertRows(stream, chart->tempo, startRow, numRows);
				}
			}
			myWriteInsertRows(stream, mySimfile->tempo, startRow, numRows);
		}
		stream.write((Tempo*)nullptr);

		gHistory->addEntry(myApplyInsertRowsId, stream.data(), stream.size());
	}
}

void myApplyInsertRowsOffset(Tempo* tempo, int startRow, int numRows)
{
	auto segs = tempo->segments;
	for(auto list = segs->begin(), listEnd = segs->end(); list != listEnd; ++list)
	{
		for(auto seg = list->begin(), end = list->end(); seg != end; ++seg)
		{
			if(seg->row >= startRow)
			{
				seg->row += numRows;
			}
		}
	}
}

String myApplyInsertRows(ReadStream& in, bool undo, bool redo)
{
	auto startRow = in.read<int>();
	auto numRows = in.read<int>();
	auto target = in.read<Tempo*>();
	while(in.success() && target)
	{
		SegmentGroup rem;
		rem.decode(in);
		if(!in.success()) break;

		if(undo) numRows = -numRows;

		// Apply positive offsets first, to make room for note insertion.
		myStartEdit(target);
		if(numRows > 0)
		{
			myApplyInsertRowsOffset(target, startRow, numRows);
		}

		// Then, insert or remove segments.
		if(undo)
		{
			target->segments->insert(rem);
		}
		else
		{
			target->segments->remove(rem);
		}

		// Apply negative offsets after the notes are removed.
		if(numRows < 0)
		{
			myApplyInsertRowsOffset(target, startRow, numRows);
		}
		myFinishEdit(target);
		
		target = in.read<Tempo*>();
	}
	return String();
}

static String ApplyInsertRows(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	return TEMPO_MAN->myApplyInsertRows(in, undo, redo);
}

// ================================================================================================
// TempoManImpl :: offset edit functions.

void myQueueOffset(double offset)
{
	WriteStream stream;
	stream.write(myTempo->offset);
	stream.write(offset);
	gHistory->addEntry(myApplyOffsetId, stream.data(), stream.size(), myTempo);
}

String myApplyOffset(Tempo* out, ReadStream& in, bool undo, bool redo)
{
	String msg;
	auto before = in.read<double>();
	auto after = in.read<double>();
	if(in.success())
	{
		double newOffset = undo ? before : after;

		msg = (undo ? "reverted" : "changed");
		msg += " offset to ";
		Str::appendVal(msg, newOffset);

		myStartEdit(out);
		out->offset = newOffset;
		myFinishEdit(out);
	}
	return msg;
}

static String ApplyOffset(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	return TEMPO_MAN->myApplyOffset(bound.tempo, in, undo, redo);
}

// ================================================================================================
// TempoManImpl :: display BPM edit functions.

struct DisplayBpmEdit { DisplayBpm type; BpmRange range; };

void myQueueDisplayBpm(const DisplayBpmEdit& change)
{
	WriteStream stream;
	stream.write(change);
	stream.write(DisplayBpmEdit{myTempo->displayBpmType, myTempo->displayBpmRange});
	gHistory->addEntry(myApplyDisplayBpmId, stream.data(), stream.size(), myTempo);
}

String myApplyDisplayBpm(Tempo* tempo, ReadStream& in, bool undo, bool redo)
{
	String msg;
	auto before = in.read<DisplayBpmEdit>();
	auto after = in.read<DisplayBpmEdit>();
	if(in.success())
	{
		DisplayBpmEdit value = (undo ? before : after);

		msg = (undo ? "Reverted" : "Changed");
		msg += "display BPM to ";
		if(value.type == BPM_ACTUAL)
		{
			msg += "default";
		}
		else if(value.type == BPM_RANDOM)
		{
			msg += "random";
		}
		else
		{
			Str::appendVal(msg, after.range.min, 3);
			if(value.range.min != value.range.max)
			{
				msg += ":";
				Str::appendVal(msg, after.range.max, 3);
			}
		}

		tempo->displayBpmType = value.type;
		tempo->displayBpmRange = value.range;
		gEditor->reportChanges(VCM_SONG_PROPERTIES_CHANGED);
	}
	return msg;
}

static String ApplyDisplayBpm(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	return TEMPO_MAN->myApplyDisplayBpm(bound.tempo, in, undo, redo);
}

// ================================================================================================
// TempoManImpl :: general editing functions.

static bool Differs(const BpmRange& a, const BpmRange& b)
{
	return a.min != b.min || a.max != b.max;
}

static double ClampAndRound(double val, double min, double max)
{
	return round(clamp(val, min, max) * 1000.0) / 1000.0;
}

void modify(const SegmentEdit& edit)
{
	stopTweaking(false);
	myQueueSegments(edit);
}

void removeSelectedSegments()
{
	SegmentEdit edit;
	for(auto& box : gTempoBoxes->getBoxes())
	{
		if(box.isSelected)
		{
			if(box.type != Segment::BPM || box.row != 0)
			{
				edit.rem.append(box.type, box.row);
			}
		}
	}
	modify(edit);
}

void insertRows(int startRow, int numRows, bool curChartOnly)
{
	myQueueInsertRows(startRow, numRows, curChartOnly);
}

void setOffset(double val)
{
	val = ClampAndRound(val, VC_MIN_OFFSET, VC_MAX_OFFSET);
	if(myTempo && myTempo->offset != val)
	{
		stopTweaking(false);
		myQueueOffset(val);
	}
}

void setDefaultBpm()
{
	if(myTempo && myTempo->displayBpmType != BPM_ACTUAL)
	{
		myQueueDisplayBpm({BPM_ACTUAL, myTempo->displayBpmRange});
	}
}

void setRandomBpm()
{
	if(myTempo && myTempo->displayBpmType != BPM_RANDOM)
	{
		myQueueDisplayBpm({BPM_RANDOM, myTempo->displayBpmRange});
	}
}

void setCustomBpm(BpmRange range)
{
	if(myTempo && (myTempo->displayBpmType != BPM_CUSTOM || Differs(myTempo->displayBpmRange, range)))
	{
		myQueueDisplayBpm({BPM_CUSTOM, range});
	}
}

// ================================================================================================
// TempoManImpl :: clipboard functions.

void copyToClipboard()
{
	SegmentGroup clipboard;

	// Copy all the selected segments.
	auto boxes = gTempoBoxes->getBoxes();
	for(auto list = myTempo->segments->begin(), listEnd = myTempo->segments->end(); list != listEnd; ++list)
	{
		auto type = list->type();
		auto seg = list->begin(), end = list->end();
		auto box = boxes.begin(), boxEnd = boxes.end();
		while(seg != end && box != boxEnd)
		{
			if(box->isSelected == 0 || box->type != type || seg->row > box->row)
			{
				++box;
			}
			else if(seg->row < box->row)
			{
				++seg;
			}
			else
			{
				clipboard.append(type, seg.ptr);
				++box, ++seg;
			}
		}
	}

	// Find out what the first row is.
	int row = INT_MAX;
	for(auto list = clipboard.begin(), listEnd = clipboard.end(); list != listEnd; ++list)
	{
		if(list->size())
		{
			row = min(row, list->begin()->row);
		}
	}

	// Offset all segments to row zero.
	for(auto list = clipboard.begin(), listEnd = clipboard.end(); list != listEnd; ++list)
	{
		for(auto seg = list->begin(), end = list->end(); seg != end; ++seg)
		{
			seg->row -= row;
		}
	}

	// Encode the segment data.
	if(clipboard.numSegments() > 0)
	{
		WriteStream stream;
		clipboard.encode(stream);
		SetClipboardData(clipboardTag, stream.data(), stream.size());
		HudNote("Copied %s", clipboard.description().str());
	}
}

void pasteFromClipboard()
{
	SegmentEdit clipboard;

	// Decode the clipboard data.	
	Vector<uchar> data = GetClipboardData(clipboardTag);
	ReadStream stream(data.begin(), data.size());
	clipboard.add.decode(stream);
	if(stream.success() == false || stream.bytesleft() > 0)
	{
		HudError("Clipboard contains invalid tempo data");
		return;
	}

	// Offset all segments to the cursor row.
	int row = gView->getCursorRow();
	for(auto list = clipboard.add.begin(), listEnd = clipboard.add.end(); list != listEnd; ++list)
	{
		for(auto seg = list->begin(), end = list->end(); seg != end; ++seg)
		{
			seg->row += row;
		}
	}

	// Add the pasted segments to the current tempo.
	modify(clipboard);
}

// ================================================================================================
// TempoManImpl :: tweak functions.

void startTweakingOffset()
{
	if(myTweakMode == TWEAK_OFFSET || !myTempo) return;
	
	stopTweaking(false);
	myTweakTempo = new Tempo;
	myTweakTempo->copy(myTempo);
	myTweakMode = TWEAK_OFFSET;
	myTweakRow = 0;
	myTweakValue = myTempo->offset;
}

void startTweakingBpm(int row)
{
	row = max(0, row);

	if((myTweakMode == TWEAK_BPM && myTweakRow == row) || !myTempo) return;
	
	stopTweaking(false);
	myTweakTempo = new Tempo;
	myTweakTempo->copy(myTempo);
	myTweakMode = TWEAK_BPM;
	myTweakRow = row;
	myTweakValue = getBpm(row);
}

void startTweakingStop(int row)
{
	if((myTweakMode == TWEAK_STOP && myTweakRow == row) || !myTempo) return;
	
	stopTweaking(false);
	myTweakTempo = new Tempo;
	myTweakTempo->copy(myTempo);
	myTweakMode = TWEAK_STOP;
	myTweakRow = row;
	myTweakValue = myTempo->segments->getRow<Stop>(myTweakRow).seconds;
}

void setTweakValue(double value)
{
	myTweakValue = value;
	if(myTweakMode == TWEAK_OFFSET)
	{
		myTweakTempo->offset = value;
	}
	else if(myTweakMode == TWEAK_BPM)
	{
		myTweakTempo->segments->insert(BpmChange(myTweakRow, value));
	}
	else if(myTweakMode == TWEAK_STOP)
	{
		myTweakTempo->segments->insert(Stop(myTweakRow, value));
	}

	myUpdateTimingData();
	gEditor->reportChanges(VCM_TEMPO_CHANGED);
}

void stopTweaking(bool apply)
{
	if(myTweakMode == TWEAK_NONE) return;

	TweakMode mode = myTweakMode;
	double value = myTweakValue;
	int row = myTweakRow;

	// Stop tweaking.
	myTweakRow = 0;
	myTweakValue = 0.0;
	myTweakMode = TWEAK_NONE;

	delete myTweakTempo;
	myTweakTempo = nullptr;

	// Apply the effect of tweaking.
	if(apply)
	{
		if(mode == TWEAK_OFFSET)
		{
			setOffset(value);
		}
		else if(mode == TWEAK_BPM)
		{
			addSegment(BpmChange(row, value));
		}
		else if(mode == TWEAK_STOP)
		{
			addSegment(Stop(row, value));
		}
	}

	myUpdateTimingData();
	gEditor->reportChanges(VCM_TEMPO_CHANGED);
}

// ================================================================================================
// TempoManImpl :: timing functions.

int timeToRow(double time) const
{
	return myTimingData.timeToRow(time);
}

double timeToBeat(double time) const
{
	return myTimingData.timeToBeat(time);
}

double rowToTime(int row) const
{
	return myTimingData.rowToTime(row);
}

double beatToTime(double beat) const
{
	return myTimingData.beatToTime(beat);
}

double beatToMeasure(double beat) const
{
	return myTimingData.beatToMeasure(beat);
}

double getBpm(int row) const
{
	if(myTweakTempo)
	{
		return myTweakTempo->segments->getRecent<BpmChange>(row).bpm;
	}
	else if(myTempo)
	{
		return myTempo->segments->getRecent<BpmChange>(row).bpm;
	}
	return SIM_DEFAULT_BPM;
}

// ================================================================================================
// TempoManImpl :: get functions.

TweakMode getTweakMode() const
{
	return myTweakMode;
}

double getTweakValue() const
{
	return myTweakValue;
}

int getTweakRow() const
{
	return myTweakRow;
}

double getOffset() const
{
	if(myTweakTempo)
	{
		return myTweakTempo->offset;
	}
	else if(myTempo)
	{
		return myTempo->offset;
	}
	return 0.0;
}

TimingMode getTimingMode() const
{
	bool splitTiming = false;
	for(auto chart : mySimfile->charts)
	{
		if(chart->hasTempo())
		{
			splitTiming = true;
			break;
		}
	}
	if(splitTiming)
	{
		return (myChart && myChart->hasTempo()) ? TIMING_STEPS : TIMING_SONG;
	}
	return TIMING_UNIFIED;
}

DisplayBpm getDisplayBpmType() const
{
	return myTempo ? myTempo->displayBpmType : BPM_ACTUAL;
}

BpmRange getDisplayBpmRange() const
{
	return myTempo ? myTempo->displayBpmRange : BpmRange{0.0, 0.0};
}

BpmRange getBpmRange() const
{
	double low = DBL_MAX, high = 0;
	if(myTempo)
	{
		auto it = myTempo->segments->begin<BpmChange>();
		auto end = myTempo->segments->end<BpmChange>();
		for(; it != end; ++it)
		{
			if(it->bpm >= 0)
			{
				low = min(low, it->bpm);
				high = max(high, it->bpm);
			}
		}
	}
	return (high >= low) ? BpmRange{low, high} : BpmRange{0, 0};
}

const TimingData& getTimingData() const
{
	return myTimingData;
}

const SegmentGroup* getSegments() const
{
	if(myTweakTempo)
	{
		return myTweakTempo->segments;
	}
	else if(myTempo)
	{
		return myTempo->segments;
	}
	return nullptr;
}

}; // TempoManImpl

// ================================================================================================
// Tempo API.

TempoMan* gTempo = nullptr;

void TempoMan::create()
{
	gTempo = new TempoManImpl;
}

void TempoMan::destroy()
{
	delete TEMPO_MAN;
	gTempo = nullptr;
}

}; // namespace Vortex