#include <Editor/History.h>

#include <Core/Draw.h>
#include <Core/Text.h>
#include <Core/Utils.h>

#include <System/System.h>

#include <Editor/Common.h>

#define HISTORY ((HistoryImpl*)gHistory)

#define NO_SAVED_ENTRIES -1

namespace Vortex {
namespace {

struct HistoryImpl : public History {

// ================================================================================================
// HistoryImpl :: helper structs.

struct Callback
{
	History::ApplyFunc apply;
	History::ReleaseFunc release;
};

struct Entry
{
	Entry* next;
};

struct EntryData
{
	uint id;
	Chart* chart;
	Tempo* tempo;
	uint size;
	const uchar* data;
};

struct EntryList
{
	EntryList() : head(nullptr)
	{
	}
	void add(Entry* entry)
	{
		if(head)
		{
			auto it = head;
			while(it->next) it = it->next;
			it->next = entry;
		}
		else
		{
			head = entry;
		}
	}
	void reverse()
	{
		Entry* it = head, *prev = nullptr, *next;
		while(it)
		{
			next = it->next;
			it->next = prev;
			prev = it;
			it = next;
		}
		head = prev;
	}
	Entry* head;
};

// ================================================================================================
// HistoryImpl :: helper functions.

static Entry* CreateEntry(EditId id, const void* data, uint size, Chart* c, Tempo* t)
{
	bool hasChart = (c != nullptr);
	bool hasTempo = (t != nullptr);

	WriteStream header;
	header.write<Entry>({nullptr});

	uint flags = (id << 2) | (hasTempo << 1) | (hasChart << 0);
	header.writeNum(flags);
	if(hasChart) header.write(c);
	if(hasTempo) header.write(t);
	header.writeNum(size);

#ifndef NDEBUG
	HudNote("Creating entry [header=%ib, data=%ib, chart:%c, tempo:%c",
		header.size(), size,
		hasChart ? 'y' : 'n',
		hasTempo ? 'y' : 'n');
#endif

	uchar* mem = (uchar*)malloc(header.size() + size);
	memcpy(mem, header.data(), header.size());
	memcpy(mem + header.size(), data, size);

	Entry* entry = (Entry*)mem;
	entry->next = nullptr;
	return (Entry*)entry;
}

static EntryData DecodeEntry(const Entry* in)
{
	ReadStream header(in + 1, INT_MAX);

	EntryData out = {0, nullptr, nullptr, 0, nullptr};

	uint flags;
	header.readNum(flags);
	if(flags & 1) header.read(out.chart);
	if(flags & 2) header.read(out.tempo);
	out.id = flags >> 2;

	header.readNum(out.size);
	out.data = header.pos();

	return out;
}

static Entry* Advance(Entry* it, Bindings& bound)
{
	ReadStream header(it + 1, INT_MAX);

	uint flags;
	header.readNum(flags);
	if(flags & 1) header.read(bound.chart);
	if(flags & 2) header.read(bound.tempo);

	return it->next;
}

static void ReleaseEntry(Entry* in, bool hasBeenApplied)
{
	EntryData entry = DecodeEntry(in);
	auto& callback = HISTORY->myCallbacks[entry.id];
	if(callback.release)
	{
		ReadStream stream(entry.data, entry.size);
		callback.release(stream, hasBeenApplied);
	}
	free(in);
}

static String ApplyEntry(const Entry* in, Bindings bound, bool undo, bool redo)
{
	EntryData entry = DecodeEntry(in);

	if(entry.chart) bound.chart = entry.chart;
	if(entry.tempo) bound.tempo = entry.tempo;

	auto& callback = HISTORY->myCallbacks[entry.id];
	ReadStream stream(entry.data, entry.size);
	return callback.apply(stream, bound, undo, redo);
}

// ================================================================================================
// HistoryImpl :: member data.

EntryList myEntries;

int mySavedEntries;
int myAppliedEntries;
int myTotalEntries;

Simfile* mySimfile;

EntryList myChain;
int myOpenChains;

Vector<Callback> myCallbacks;

// ================================================================================================
// HistoryImpl :: constructor and destructor.

~HistoryImpl()
{
	clearEverything();
}

HistoryImpl()
	: mySavedEntries(0)
	, myAppliedEntries(0)
	, myTotalEntries(0)
	, myOpenChains(0)
{
	myCallbacks.push_back({ApplyChain, ReleaseChain});
}

// ================================================================================================
// HistoryImpl :: adding callbacks.

EditId addCallback(ApplyFunc apply, ReleaseFunc release)
{
	EditId out = myCallbacks.size();
	myCallbacks.push_back({apply, release});
	return out;
}

// ================================================================================================
// HistoryImpl :: adding entries.

void pushEntry(Entry* entry)
{
	Bindings bound = {mySimfile, nullptr, nullptr};
	auto it = myEntries.head;
	while(it) it = Advance(it, bound);

	myEntries.add(entry);
	++myAppliedEntries;
	++myTotalEntries;

	String msg = ApplyEntry(entry, bound, false, false);

	if(msg.len()) HudNote("%s", msg.str());
}

void addEntry(EditId id, const void* data, uint size, Chart* targetChart, Tempo* targetTempo)
{
	if(id == 0 || id > (size_t)myCallbacks.size())
	{
		HudError("History edit has invalid ID!");
		return;
	}

	clearUnappliedEntries();

	// Find out what the current bindings are.
	Bindings bound = {mySimfile, nullptr, nullptr};
	auto it = myEntries.head;
	while(it) it = Advance(it, bound);

	// If the target chart/tempo are equal to the current chart/tempo, there is no need to record them.
	if(targetChart == bound.chart) targetChart = nullptr;
	if(targetTempo == bound.tempo) targetTempo = nullptr;

	Entry* entry = CreateEntry(id, data, size, targetChart, targetTempo);
	if(myOpenChains > 0)
	{
		myChain.add(entry);
	}
	else
	{
		pushEntry(entry);
	}
}

void addEntry(EditId id, const void* data, uint size)
{
	addEntry(id, data, size, nullptr, nullptr);
}

void addEntry(EditId id, const void* data, uint size, Tempo* targetTempo)
{
	addEntry(id, data, size, nullptr, targetTempo);
}

void addEntry(EditId id, const void* data, uint size, Chart* targetChart)
{
	addEntry(id, data, size, targetChart, nullptr);
}

// ================================================================================================
// HistoryImpl :: undo/redo entries.

void redoEntry()
{
	if(myTotalEntries > myAppliedEntries)
	{
		Bindings bound = {mySimfile, nullptr, nullptr};
		auto it = myEntries.head;
		for(int i = 0; i < myAppliedEntries; ++i) it = Advance(it, bound);

		String msg = ApplyEntry(it, bound, false, true);

		++myAppliedEntries;

		if(msg.empty()) msg = "---";
		HudNote("{tc:4a4}{g:redo}{tc:666}[%i/%i]:{tc} %s",
			myAppliedEntries, myTotalEntries, msg.str());
	}
}

void undoEntry()
{
	if(myAppliedEntries > 0)
	{
		Bindings bound = {mySimfile, nullptr, nullptr};
		auto it = myEntries.head;
		for(int i = 0; i < myAppliedEntries - 1; ++i) it = Advance(it, bound);

		String msg = ApplyEntry(it, bound, true, false);

		--myAppliedEntries;

		if(msg.empty()) msg = "---";
		HudNote("{tc:822}{g:undo}{tc:666}[%i/%i]:{tc} %s",
			myAppliedEntries, myTotalEntries, msg.str());
	}
}

// ================================================================================================
// HistoryImpl :: history interactions.

void onKeyPress(KeyPress& evt)
{
	if(evt.handled == false && (evt.keyflags & Keyflag::CTRL))
	{
		if(evt.key == Key::Z)
		{
			undoEntry();
		}
		else if(evt.key == Key::Y)
		{
			redoEntry();
		}
	}
}

void onFileOpen(Simfile* simfile)
{
	mySimfile = simfile;
}

void onFileSaved()
{
	mySavedEntries = myAppliedEntries;
}

void onFileClosed()
{
	clearEverything();
	mySavedEntries = 0;
	mySimfile = nullptr;
}

bool hasUnsavedChanges() const
{
	return (mySavedEntries != myAppliedEntries);
}

// ================================================================================================
// HistoryImpl :: chains.

static void ReleaseChain(ReadStream& in, bool hasBeenApplied)
{
	auto list = in.read<EntryList>();
	auto msg = in.readStr();
	if(in.success())
	{
		list.reverse(); // Release in reverse order, from most recent to oldest.
		for(auto it = list.head; it;)
		{
			auto next = it->next;
			ReleaseEntry(it, hasBeenApplied);
			it = next;
		}
	}
	else
	{
		HudError("History has invalid chain");
	}
}

static String ApplyChain(ReadStream& in, History::Bindings bound, bool undo, bool redo)
{
	auto list = in.read<EntryList>();
	auto msg = in.readStr();
	if(in.success())
	{
		if(undo)
		{
			list.reverse(); // Undo in reverse order, from most recent to oldest.
			for(auto it = list.head; it; it = it->next)
			{
				ApplyEntry(it, bound, true, redo);
			}
			list.reverse(); // Restore the original order.
		}
		else
		{
			for(auto it = list.head; it; it = it->next)
			{
				ApplyEntry(it, bound, false, redo);
			}
		}
	}
	else
	{
		HudError("History has invalid chain");
	}
	return msg;
}

void startChain()
{
	++myOpenChains;
}

void finishChain(String msg)
{
	myOpenChains = max(0, myOpenChains - 1);
	if(myChain.head && myOpenChains == 0)
	{
		clearUnappliedEntries();

		WriteStream stream;
		stream.write(myChain);
		stream.writeStr(msg);

		myChain.head = nullptr;

		auto entry = CreateEntry(0, stream.data(), stream.size(), nullptr, nullptr);
		pushEntry(entry);
	}
}

// ================================================================================================
// HistoryImpl :: clearing entries.

void clearUnappliedEntries()
{
	int unappliedEntries = myTotalEntries - myAppliedEntries;
	
	myEntries.reverse();
	auto it = myEntries.head;
	for(int i = 0; i < unappliedEntries; ++i)
	{
		auto next = it->next;
		ReleaseEntry(it, false);
		it = next;
	}
	myEntries.head = it;
	myEntries.reverse();

	myTotalEntries = myAppliedEntries;
	if(mySavedEntries > myAppliedEntries)
	{
		mySavedEntries = NO_SAVED_ENTRIES;
	}
}

void clearEverything()
{
	int unappliedEntries = myTotalEntries - myAppliedEntries;
	
	myEntries.reverse();
	auto it = myEntries.head;
	for(int i = 0; i < unappliedEntries; ++i)
	{
		auto next = it->next;
		ReleaseEntry(it, false);
		it = next;
	}
	for(int i = 0; i < myAppliedEntries; ++i)
	{
		auto next = it->next;
		ReleaseEntry(it, true);
		it = next;
	}
	myEntries.head = nullptr;

	myAppliedEntries = 0;
	mySavedEntries = 0;
	myTotalEntries = 0;
}

}; // HistoryImpl.
}; // anonymous namespace.

// ================================================================================================
// History API.

History* gHistory = nullptr;

void History::create()
{
	gHistory = new HistoryImpl;
}

void History::destroy()
{
	delete (HistoryImpl*)gHistory;
	gHistory = nullptr;
}

}; // namespace Vortex
