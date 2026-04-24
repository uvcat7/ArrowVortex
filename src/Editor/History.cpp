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

    struct Callback {
        History::ApplyFunc apply;
        History::ReleaseFunc release;
    };

    struct Entry {
        Entry* next;
    };

    struct EntryData {
        uint32_t id;
        Chart* chart;
        Tempo* tempo;
        uint32_t size;
        const uint8_t* data;
    };

    struct EntryList {
        EntryList() = default;
        void add(Entry* entry) {
            if (head) {
                auto it = head;
                while (it->next) it = it->next;
                it->next = entry;
            } else {
                head = entry;
            }
        }
        void reverse() {
            Entry *it = head, *prev = nullptr, *next;
            while (it) {
                next = it->next;
                it->next = prev;
                prev = it;
                it = next;
            }
            head = prev;
        }
        Entry* head = nullptr;
    };

    // ================================================================================================
    // HistoryImpl :: helper functions.

    static Entry* CreateEntry(EditId id, const void* data, uint32_t size,
                              Chart* c, Tempo* t) {
        bool hasChart = (c != nullptr);
        bool hasTempo = (t != nullptr);

        WriteStream header;
        header.write<Entry>({nullptr});

        uint32_t flags = (id << 2) | (hasTempo << 1) | (hasChart << 0);
        header.writeNum(flags);
        if (hasChart) header.write(c);
        if (hasTempo) header.write(t);
        header.writeNum(size);

#ifndef NDEBUG
        HudNote("Creating entry [header=%ib, data=%ib, chart:%c, tempo:%c",
                header.size(), size, hasChart ? 'y' : 'n',
                hasTempo ? 'y' : 'n');
#endif

        uint8_t* mem = static_cast<uint8_t*>(malloc(header.size() + size));
        memcpy(mem, header.data(), header.size());
        memcpy(mem + header.size(), data, size);

        Entry* entry = reinterpret_cast<Entry*>(mem);
        entry->next = nullptr;
        return entry;
    }

    static EntryData DecodeEntry(const Entry* in) {
        ReadStream header(in + 1, INT_MAX);

        EntryData out = {0, nullptr, nullptr, 0, nullptr};

        uint32_t flags;
        header.readNum(flags);
        if (flags & 1) header.read(out.chart);
        if (flags & 2) header.read(out.tempo);
        out.id = flags >> 2;

        header.readNum(out.size);
        out.data = header.pos();

        return out;
    }

    static Entry* Advance(Entry* it, Bindings& bound) {
        ReadStream header(it + 1, INT_MAX);

        uint32_t flags;
        header.readNum(flags);
        if (flags & 1) header.read(bound.chart);
        if (flags & 2) header.read(bound.tempo);

        return it->next;
    }

    static void ReleaseEntry(Entry* in, bool hasBeenApplied) {
        EntryData entry = DecodeEntry(in);
        auto& callback = HISTORY->myCallbacks[entry.id];
        if (callback.release) {
            ReadStream stream(entry.data, entry.size);
            callback.release(stream, hasBeenApplied);
        }
        free(in);
    }

    static std::string ApplyEntry(const Entry* in, Bindings bound, bool undo,
                                  bool redo) {
        EntryData entry = DecodeEntry(in);

        if (entry.chart) bound.chart = entry.chart;
        if (entry.tempo) bound.tempo = entry.tempo;

        auto& callback = HISTORY->myCallbacks[entry.id];
        ReadStream stream(entry.data, entry.size);
        return callback.apply(stream, bound, undo, redo);
    }

    // ================================================================================================
    // HistoryImpl :: member data.

    EntryList myEntries;

    int mySavedEntries = 0;
    int myAppliedEntries = 0;
    int myTotalEntries = 0;

    Simfile* mySimfile;

    EntryList myChain;
    int myOpenChains = 0;

    Vector<Callback> myCallbacks;

    // ================================================================================================
    // HistoryImpl :: constructor and destructor.

    ~HistoryImpl() { clearEverything(); }

    HistoryImpl()

    {
        myCallbacks.push_back({ApplyChain, ReleaseChain});
    }

    // ================================================================================================
    // HistoryImpl :: adding callbacks.

    EditId addCallback(ApplyFunc apply, ReleaseFunc release) override {
        EditId out = myCallbacks.size();
        myCallbacks.push_back({apply, release});
        return out;
    }

    // ================================================================================================
    // HistoryImpl :: adding entries.

    void pushEntry(Entry* entry) {
        Bindings bound = {mySimfile, nullptr, nullptr};
        auto it = myEntries.head;
        while (it) it = Advance(it, bound);

        myEntries.add(entry);
        ++myAppliedEntries;
        ++myTotalEntries;

        std::string msg = ApplyEntry(entry, bound, false, false);

        if (msg.length()) HudNote("%s", msg.c_str());
    }

    void addEntry(EditId id, const void* data, uint32_t size,
                  Chart* targetChart, Tempo* targetTempo) {
        if (id == 0 || id > static_cast<size_t>(myCallbacks.size())) {
            HudError("History edit has invalid ID!");
            return;
        }

        clearUnappliedEntries();

        // Find out what the current bindings are.
        Bindings bound = {mySimfile, nullptr, nullptr};
        auto it = myEntries.head;
        while (it) it = Advance(it, bound);

        // If the target chart/tempo are equal to the current chart/tempo, there
        // is no need to record them.
        if (targetChart == bound.chart) targetChart = nullptr;
        if (targetTempo == bound.tempo) targetTempo = nullptr;

        Entry* entry = CreateEntry(id, data, size, targetChart, targetTempo);
        if (myOpenChains > 0) {
            myChain.add(entry);
        } else {
            pushEntry(entry);
        }
    }

    void addEntry(EditId id, const void* data, uint32_t size) override {
        addEntry(id, data, size, nullptr, nullptr);
    }

    void addEntry(EditId id, const void* data, uint32_t size,
                  Tempo* targetTempo) override {
        addEntry(id, data, size, nullptr, targetTempo);
    }

    void addEntry(EditId id, const void* data, uint32_t size,
                  Chart* targetChart) override {
        addEntry(id, data, size, targetChart, nullptr);
    }

    // ================================================================================================
    // HistoryImpl :: undo/redo entries.

    void redoEntry() {
        if (myTotalEntries > myAppliedEntries) {
            Bindings bound = {mySimfile, nullptr, nullptr};
            auto it = myEntries.head;
            for (int i = 0; i < myAppliedEntries; ++i) it = Advance(it, bound);

            std::string msg = ApplyEntry(it, bound, false, true);

            ++myAppliedEntries;

            if (msg.empty()) msg = "---";
            HudNote("{tc:4a4}{g:redo}{tc:666}[%i/%i]:{tc} %s", myAppliedEntries,
                    myTotalEntries, msg.c_str());
        }
    }

    void undoEntry() {
        if (myAppliedEntries > 0) {
            Bindings bound = {mySimfile, nullptr, nullptr};
            auto it = myEntries.head;
            for (int i = 0; i < myAppliedEntries - 1; ++i)
                it = Advance(it, bound);

            std::string msg = ApplyEntry(it, bound, true, false);

            --myAppliedEntries;

            if (msg.empty()) msg = "---";
            HudNote("{tc:822}{g:undo}{tc:666}[%i/%i]:{tc} %s", myAppliedEntries,
                    myTotalEntries, msg.c_str());
        }
    }

    // ================================================================================================
    // HistoryImpl :: history interactions.

    void onKeyPress(KeyPress& evt) override {
        if (evt.handled == false && (evt.keyflags & Keyflag::CTRL)) {
            if (evt.key == Key::Z) {
                undoEntry();
            } else if (evt.key == Key::Y) {
                redoEntry();
            }
        }
    }

    void onFileOpen(Simfile* simfile) override { mySimfile = simfile; }

    void onFileSaved() override { mySavedEntries = myAppliedEntries; }

    void onFileClosed() override {
        clearEverything();
        mySavedEntries = 0;
        mySimfile = nullptr;
    }

    bool hasUnsavedChanges() const override {
        return (mySavedEntries != myAppliedEntries);
    }

    // ================================================================================================
    // HistoryImpl :: chains.

    static void ReleaseChain(ReadStream& in, bool hasBeenApplied) {
        auto list = in.read<EntryList>();
        auto msg = in.readStr();
        if (in.success()) {
            list.reverse();  // Release in reverse order, from most recent to
                             // oldest.
            for (auto it = list.head; it;) {
                auto next = it->next;
                ReleaseEntry(it, hasBeenApplied);
                it = next;
            }
        } else {
            HudError("History has invalid chain");
        }
    }

    static std::string ApplyChain(ReadStream& in, History::Bindings bound,
                                  bool undo, bool redo) {
        auto list = in.read<EntryList>();
        auto msg = in.readStr();
        if (in.success()) {
            if (undo) {
                list.reverse();  // Undo in reverse order, from most recent to
                                 // oldest.
                for (auto it = list.head; it; it = it->next) {
                    ApplyEntry(it, bound, true, redo);
                }
                list.reverse();  // Restore the original order.
            } else {
                for (auto it = list.head; it; it = it->next) {
                    ApplyEntry(it, bound, false, redo);
                }
            }
        } else {
            HudError("History has invalid chain");
        }
        return msg;
    }

    void startChain() override { ++myOpenChains; }

    void finishChain(std::string msg) override {
        myOpenChains = max(0, myOpenChains - 1);
        if (myChain.head && myOpenChains == 0) {
            clearUnappliedEntries();

            WriteStream stream;
            stream.write(myChain);
            stream.writeStr(msg);

            myChain.head = nullptr;

            auto entry =
                CreateEntry(0, stream.data(), stream.size(), nullptr, nullptr);
            pushEntry(entry);
        }
    }

    // ================================================================================================
    // HistoryImpl :: clearing entries.

    void clearUnappliedEntries() {
        int unappliedEntries = myTotalEntries - myAppliedEntries;

        myEntries.reverse();
        auto it = myEntries.head;
        for (int i = 0; i < unappliedEntries; ++i) {
            auto next = it->next;
            ReleaseEntry(it, false);
            it = next;
        }
        myEntries.head = it;
        myEntries.reverse();

        myTotalEntries = myAppliedEntries;
        if (mySavedEntries > myAppliedEntries) {
            mySavedEntries = NO_SAVED_ENTRIES;
        }
    }

    void clearEverything() {
        int unappliedEntries = myTotalEntries - myAppliedEntries;

        myEntries.reverse();
        auto it = myEntries.head;
        for (int i = 0; i < unappliedEntries; ++i) {
            auto next = it->next;
            ReleaseEntry(it, false);
            it = next;
        }
        for (int i = 0; i < myAppliedEntries; ++i) {
            auto next = it->next;
            ReleaseEntry(it, true);
            it = next;
        }
        myEntries.head = nullptr;

        myAppliedEntries = 0;
        mySavedEntries = 0;
        myTotalEntries = 0;
    }

};  // HistoryImpl.
};  // anonymous namespace.

// ================================================================================================
// History API.

History* gHistory = nullptr;

void History::create() { gHistory = new HistoryImpl; }

void History::destroy() {
    delete static_cast<HistoryImpl*>(gHistory);
    gHistory = nullptr;
}

};  // namespace Vortex
