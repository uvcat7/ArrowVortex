#include <Managers/TempoMan.h>

#include <algorithm>
#include <climits>
#include <math.h>
#include <cfloat>

#include <Core/ByteStream.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>
#include <Core/VectorUtils.h>

#include <Editor/Clipboard.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/History.h>
#include <Editor/Selection.h>
#include <Editor/TempoBoxes.h>
#include <Editor/View.h>

#include <Managers/SimfileMan.h>

#include <Simfile/SegmentGroup.h>
#include <Simfile/SegmentList.h>
#include <Simfile/TimingData.h>

#include <System/System.h>

#include <optional>
#include <utility>
#include <sstream>
#include <string>
#include <tuple>

#define TEMPO_MAN ((TempoManImpl*)gTempo)

namespace Vortex {

// ================================================================================================
// TempoManImpl :: member data.

const char* TempoMan::clipboardTag = "tempo";

enum class VisualSyncMode { DESTRUCTIVE, NON_DESTRUCTIVE, OFFSET };

struct VisualSyncData {
    const int targetRow;
    const VisualSyncMode mode;
    const int leftLimitRow;
    const int rightLimitRow;
    const double initialBpm;
};

struct TempoManImpl : public TempoMan {
    Tempo* myTempo = nullptr;

    Chart* myChart;
    Simfile* mySimfile = nullptr;
    TimingData myTimingData;
    double myInitialBpm;

    int myTweakRow;
    Tempo* myTweakTempo = nullptr;
    TweakMode myTweakMode = TWEAK_NONE;
    double myTweakValue;

    std::optional<VisualSyncData> myVisualSync = std::nullopt;

    History::EditId myApplyOffsetId;
    History::EditId myApplySegmentsId;
    History::EditId myApplyInsertRowsId;
    History::EditId myApplyDisplayBpmId;

    // ================================================================================================
    // TempoManImpl :: constructor and destructor.

    ~TempoManImpl() = default;

    TempoManImpl()

    {
        myUpdateTimingData();

        myApplyOffsetId = gHistory->addCallback(ApplyOffset);
        myApplySegmentsId = gHistory->addCallback(ApplySegments);
        myApplyInsertRowsId = gHistory->addCallback(ApplyInsertRows);
        myApplyDisplayBpmId = gHistory->addCallback(ApplyDisplayBpm);
    }

    // ================================================================================================
    // TempoManImpl :: update functions.

    void myUpdateTimingData() {
        if (myTweakTempo) {
            myTimingData.update(myTweakTempo);
        } else if (myTempo) {
            myTimingData.update(myTempo);
        } else {
            myTimingData = TimingData();
        }

        if (gNotes) gNotes->updateTempo();

        gEditor->reportChanges(VCM_TEMPO_CHANGED);
    }

    void update(Simfile* sim, Chart* chart) override {
        myChart = chart;
        mySimfile = sim;

        Tempo* tempo = nullptr;
        if (chart && chart->hasTempo()) {
            tempo = chart->getTempo(sim);
        } else if (sim) {
            tempo = sim->tempo;
        }

        if (myTempo != tempo) {
            stopTweaking(false);
            myTempo = tempo;
            myUpdateTimingData();
        }
    }

    // ================================================================================================
    // TempoManImpl :: edit helper functions.

    void myStartEdit(Tempo* tempo) {
        if (myTempo == tempo) {
            stopTweaking(false);
        }
    }

    void myFinishEdit(Tempo* tempo) {
        tempo->sanitize();
        if (myTempo == tempo) {
            myUpdateTimingData();
            gEditor->reportChanges(VCM_TEMPO_CHANGED);
        }
    }

    // ================================================================================================
    // TempoManImpl :: apply segments.

    void myQueueSegments(const SegmentEdit& edit, bool clearRegion) {
        stopTweaking(false);
        SegmentEditResult result;
        myTempo->segments->prepareEdit(edit, result, clearRegion);
        if (result.add.numSegments() + result.rem.numSegments() > 0) {
            WriteStream stream;
            result.add.encode(stream);
            result.rem.encode(stream);
            gHistory->addEntry(myApplySegmentsId, stream.data(), stream.size(),
                               myTempo);
        }
    }

    std::string myApplySegments(Tempo* out, ReadStream& in, bool undo,
                                bool redo) {
        std::string msg;
        SegmentGroup add, rem;
        add.decode(in);
        rem.decode(in);
        if (in.success()) {
            int numAdd = add.numSegments();
            int numRem = rem.numSegments();

            bool addMatchesRem = false;
            if (numAdd == numRem) {
                addMatchesRem = true;
                auto addList = add.begin(), addListEnd = add.end();
                auto remList = rem.begin(), remListEnd = rem.end();
                while (addList != addListEnd && remList != remListEnd) {
                    if (addList->size() != remList->size()) {
                        addMatchesRem = false;
                        break;
                    }
                    auto a = addList->begin(), aEnd = addList->end();
                    auto r = remList->begin(), rEnd = remList->end();
                    while (a != aEnd && r != rEnd) {
                        if (a->row != r->row) {
                            addMatchesRem = false;
                            break;
                        }
                        ++a, ++r;
                    }
                    ++addList, ++remList;
                }
            }

            std::string remove = rem.descriptionValues();
            std::string after = add.descriptionValues();

            if (addMatchesRem) {
                msg = msg + "Changed " + add.description() + ": " + remove +
                      " {g:arrow right} " + after;
            } else {
                if (numAdd > 0) {
                    msg = msg + "Added " + add.description() + ": " + after;
                }
                if (numRem > 0) {
                    if (msg.length()) msg = msg + ", ";
                    msg = msg + "Removed " + rem.description() + ": " + remove;
                }
            }

            myStartEdit(out);
            if (undo) {
                out->segments->remove(add);
                out->segments->insert(rem);
            } else {
                out->segments->remove(rem);
                out->segments->insert(add);
            }
            myFinishEdit(out);
        }
        return msg;
    }

    static std::string ApplySegments(ReadStream& in, History::Bindings bound,
                                     bool undo, bool redo) {
        return TEMPO_MAN->myApplySegments(bound.tempo, in, undo, redo);
    }

    // ================================================================================================
    // TempoManImpl :: apply insert rows.

    void myWriteInsertRows(WriteStream& stream, Tempo* tempo, int startRow,
                           int numRows) {
        SegmentEdit edit;
        SegmentEditResult result;
        if (numRows < 0) {
            int endRow = startRow - numRows;
            SegmentGroup* segs = tempo->segments;
            for (auto& list : *segs) {
                for (auto seg = list.begin(), end = list.end(); seg != end;
                     ++seg) {
                    if (seg->row >= startRow && seg->row < endRow) {
                        edit.rem.append(list.type(), seg.ptr);
                    }
                }
            }
            segs->prepareEdit(edit, result, true);
        }
        stream.write(tempo);
        result.rem.encode(stream);
    }

    void myQueueInsertRows(int startRow, int numRows, bool curChartOnly) {
        if (mySimfile && numRows != 0) {
            WriteStream stream;
            stream.write<int>(startRow);
            stream.write<int>(numRows);

            if (curChartOnly) {
                if (myChart && myChart->hasTempo()) {
                    myWriteInsertRows(stream, myChart->tempo, startRow,
                                      numRows);
                } else {
                    myWriteInsertRows(stream, mySimfile->tempo, startRow,
                                      numRows);
                }
            } else {
                for (auto chart : mySimfile->charts) {
                    if (chart->hasTempo()) {
                        myWriteInsertRows(stream, chart->tempo, startRow,
                                          numRows);
                    }
                }
                myWriteInsertRows(stream, mySimfile->tempo, startRow, numRows);
            }
            stream.write(static_cast<Tempo*>(nullptr));

            gHistory->addEntry(myApplyInsertRowsId, stream.data(),
                               stream.size());
        }
    }

    void myApplyInsertRowsOffset(Tempo* tempo, int startRow, int numRows) {
        auto segs = tempo->segments;
        for (auto& list : *segs) {
            for (auto seg = list.begin(), end = list.end(); seg != end; ++seg) {
                if (seg->row >= startRow && seg->row > 0) {
                    seg->row += numRows;
                }
            }
        }
    }

    std::string myApplyInsertRows(ReadStream& in, bool undo, bool redo) {
        auto startRow = in.read<int>();
        auto numRows = in.read<int>();
        auto target = in.read<Tempo*>();
        while (in.success() && target) {
            SegmentGroup rem;
            rem.decode(in);
            if (!in.success()) break;

            if (undo) numRows = -numRows;

            // Apply positive offsets first, to make room for note insertion.
            myStartEdit(target);
            if (numRows > 0) {
                myApplyInsertRowsOffset(target, startRow, numRows);
            }

            // Then, insert or remove segments.
            if (undo) {
                target->segments->insert(rem);
            } else {
                target->segments->remove(rem);
            }

            // Apply negative offsets after the notes are removed.
            if (numRows < 0) {
                myApplyInsertRowsOffset(target, startRow, numRows);
            }
            myFinishEdit(target);

            target = in.read<Tempo*>();
        }
        return std::string();
    }

    static std::string ApplyInsertRows(ReadStream& in, History::Bindings bound,
                                       bool undo, bool redo) {
        return TEMPO_MAN->myApplyInsertRows(in, undo, redo);
    }

    // ================================================================================================
    // TempoManImpl :: offset edit functions.

    void myQueueOffset(double offset) {
        WriteStream stream;
        stream.write(myTempo->offset);
        stream.write(offset);
        gHistory->addEntry(myApplyOffsetId, stream.data(), stream.size(),
                           myTempo);
    }

    std::string myApplyOffset(Tempo* out, ReadStream& in, bool undo,
                              bool redo) {
        std::string msg;
        auto before = in.read<double>();
        auto after = in.read<double>();
        if (in.success()) {
            double newOffset = undo ? before : after;

            msg = "Changed offset: ";
            Str::appendVal(msg, before);
            msg = msg + " {g:arrow right} ";
            Str::appendVal(msg, after);

            myStartEdit(out);
            out->offset = newOffset;
            myFinishEdit(out);
        }
        return msg;
    }

    static std::string ApplyOffset(ReadStream& in, History::Bindings bound,
                                   bool undo, bool redo) {
        return TEMPO_MAN->myApplyOffset(bound.tempo, in, undo, redo);
    }

    // ================================================================================================
    // TempoManImpl :: display BPM edit functions.

    struct DisplayBpmEdit {
        DisplayBpm type;
        BpmRange range;
    };

    void myQueueDisplayBpm(const DisplayBpmEdit& change) {
        WriteStream stream;
        stream.write(
            DisplayBpmEdit{myTempo->displayBpmType, myTempo->displayBpmRange});
        stream.write(change);
        gHistory->addEntry(myApplyDisplayBpmId, stream.data(), stream.size(),
                           myTempo);
    }

    std::string myApplyDisplayBpm(Tempo* tempo, ReadStream& in, bool undo,
                                  bool redo) {
        std::string msg;
        auto before = in.read<DisplayBpmEdit>();
        auto after = in.read<DisplayBpmEdit>();
        if (in.success()) {
            DisplayBpmEdit value = (undo ? before : after);

            msg = "Changed display BPM: ";
            myApplyDisplayBpmMessage(msg, before);
            msg = msg + " {g:arrow right} ";
            myApplyDisplayBpmMessage(msg, after);

            tempo->displayBpmType = value.type;
            tempo->displayBpmRange = value.range;
            gEditor->reportChanges(VCM_SONG_PROPERTIES_CHANGED);
        }
        return msg;
    }

    void myApplyDisplayBpmMessage(std::string& msg, DisplayBpmEdit& edit) {
        if (edit.type == BPM_ACTUAL) {
            msg = msg + "default";
        } else if (edit.type == BPM_RANDOM) {
            msg = msg + "random";
        } else {
            Str::appendVal(msg, edit.range.min);
            if (edit.range.min != edit.range.max) {
                msg = msg + "-";
                Str::appendVal(msg, edit.range.max);
            }
        }
    }

    static std::string ApplyDisplayBpm(ReadStream& in, History::Bindings bound,
                                       bool undo, bool redo) {
        return TEMPO_MAN->myApplyDisplayBpm(bound.tempo, in, undo, redo);
    }

    // ================================================================================================
    // TempoManImpl :: general editing functions.

    static bool Differs(const BpmRange& a, const BpmRange& b) {
        return a.min != b.min || a.max != b.max;
    }

    static double ClampAndRound(double val, double min, double max) {
        return std::round(std::clamp(val, min, max) * 1000000.0) / 1000000.0;
    }

    void modify(const SegmentEdit& edit) override { modify(edit, true); }

    void modify(const SegmentEdit& edit, bool clearRegion) override {
        stopTweaking(false);
        myQueueSegments(edit, clearRegion);
    }

    void removeSelectedSegments() override {
        SegmentEdit edit;
        for (auto& box : gTempoBoxes->getBoxes()) {
            if (box.isSelected) {
                if (box.type != Segment::BPM || box.row != 0) {
                    edit.rem.append(box.type, box.row);
                }
            }
        }
        modify(edit);
    }

    void insertRows(int startRow, int numRows, bool curChartOnly) override {
        myQueueInsertRows(startRow, numRows, curChartOnly);
    }

    void setOffset(double val) override {
        val = ClampAndRound(val, VC_MIN_OFFSET, VC_MAX_OFFSET);
        if (myTempo && myTempo->offset != val) {
            stopTweaking(false);
            myQueueOffset(val);
        }
    }

    void setDefaultBpm() override {
        if (myTempo && myTempo->displayBpmType != BPM_ACTUAL) {
            myQueueDisplayBpm({BPM_ACTUAL, myTempo->displayBpmRange});
        }
    }

    void setRandomBpm() override {
        if (myTempo && myTempo->displayBpmType != BPM_RANDOM) {
            myQueueDisplayBpm({BPM_RANDOM, myTempo->displayBpmRange});
        }
    }

    void setCustomBpm(BpmRange range) override {
        if (myTempo && (myTempo->displayBpmType != BPM_CUSTOM ||
                        Differs(myTempo->displayBpmRange, range))) {
            myQueueDisplayBpm({BPM_CUSTOM, range});
        }
    }

    // ================================================================================================
    // TempoManImpl :: clipboard functions.

    int minSelectionRow() const override {
        auto& boxes = gTempoBoxes->getBoxes();
        for (auto& box : boxes) {
            if (box.isSelected) return box.row;
        }
        return INT_MAX;
    }

    void copyToClipboard(std::string& out, int minRow) override {
        SegmentGroup clipboard;

        // Copy all the selected segments.
        auto& boxes = gTempoBoxes->getBoxes();
        for (auto& segment : *myTempo->segments) {
            auto type = segment.type();
            auto seg = segment.begin(), end = segment.end();
            auto box = boxes.begin(), boxEnd = boxes.end();
            while (seg != end && box != boxEnd) {
                if (box->isSelected == 0 || box->type != type ||
                    seg->row > box->row) {
                    ++box;
                } else if (seg->row < box->row) {
                    ++seg;
                } else {
                    clipboard.append(type, seg.ptr);
                    ++box, ++seg;
                }
            }
        }

        // Find out what the first row is.
        int row = minRow;
        if (row == INT_MAX) {
            for (auto& list : clipboard) {
                if (list.size()) {
                    row = std::min(row, list.begin()->row);
                }
            }
        }

        // Offset all segments to row zero.
        for (auto& list : clipboard) {
            for (auto seg = list.begin(), end = list.end(); seg != end; ++seg) {
                seg->row -= row;
            }
        }

        // Encode the segment data.
        if (clipboard.numSegments() > 0) {
            WriteStream stream;
            clipboard.encode(stream);
            out.append(clipboardTag);
            Base64Encode(out, stream.data(), stream.size());
            HudNote("Copied %s", clipboard.description().c_str());
        }
    }

    void pasteFromClipboard(ClipboardData clipboard, bool insert) override {
        SegmentEdit edit;

        // Decode the clipboard data.
        std::vector<uint8_t> buffer = clipboard.tempos;
        if (buffer.size() == 0) return;

        ReadStream stream(&(*buffer.begin()), buffer.size());
        edit.add.decode(stream);
        if (stream.success() == false || stream.bytesleft() > 0) {
            HudError("Clipboard contains invalid tempo data.");
            return;
        }

        // Offset all segments to the cursor row.
        int row = gView->getCursorRow();
        for (auto& list : edit.add) {
            for (auto seg = list.begin(), end = list.end(); seg != end; ++seg) {
                seg->row += row;
            }
        }

        // Add the pasted segments to the current tempo.
        modify(edit, !insert);
    }

    // ================================================================================================
    // TempoManImpl :: tweak functions.

    void startTweakingOffset() override {
        if (myTweakMode == TWEAK_OFFSET || !myTempo) return;

        stopTweaking(true);
        myTweakTempo = new Tempo;
        myTweakTempo->copy(myTempo);
        myTweakMode = TWEAK_OFFSET;
        myTweakRow = 0;
        myTweakValue = myTempo->offset;
    }

    void startTweakingBpm(int row) override {
        row = std::max(0, row);

        if ((myTweakMode == TWEAK_BPM && myTweakRow == row) || !myTempo) return;

        stopTweaking(true);
        myTweakTempo = new Tempo;
        myTweakTempo->copy(myTempo);
        myTweakMode = TWEAK_BPM;
        myTweakRow = row;
        myTweakValue = getBpm(row);
    }

    void startTweakingStop(int row) override {
        if ((myTweakMode == TWEAK_STOP && myTweakRow == row) || !myTempo)
            return;

        stopTweaking(true);
        myTweakTempo = new Tempo;
        myTweakTempo->copy(myTempo);
        myTweakMode = TWEAK_STOP;
        myTweakRow = row;
        myTweakValue = myTempo->segments->getRow<Stop>(myTweakRow).seconds;
    }

    void setTweakValue(double value) override {
        myTweakValue = value;
        if (myTweakMode == TWEAK_OFFSET) {
            myTweakTempo->offset = value;
        } else if (myTweakMode == TWEAK_BPM) {
            myTweakTempo->segments->insert(BpmChange(myTweakRow, value));
        } else if (myTweakMode == TWEAK_STOP) {
            myTweakTempo->segments->insert(Stop(myTweakRow, value));
        }

        myUpdateTimingData();
        gEditor->reportChanges(VCM_TEMPO_CHANGED);
    }

    void stopTweaking(bool apply) override {
        if (myTweakMode == TWEAK_NONE) return;

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
        if (apply) {
            if (mode == TWEAK_OFFSET) {
                setOffset(value);
            } else if (mode == TWEAK_BPM) {
                addSegment(BpmChange(row, value));
            } else if (mode == TWEAK_STOP) {
                addSegment(Stop(row, value));
            }
        }

        myUpdateTimingData();
        gEditor->reportChanges(VCM_TEMPO_CHANGED);
    }

    // ================================================================================================
    // TempoManImpl :: timing functions.

    int timeToRow(double time) const override {
        return myTimingData.timeToRow(time);
    }

    double timeToBeat(double time) const override {
        return myTimingData.timeToBeat(time);
    }

    double rowToTime(int row) const override {
        return myTimingData.rowToTime(row);
    }

    double beatToTime(double beat) const override {
        return myTimingData.beatToTime(beat);
    }

    double beatToMeasure(double beat) const override {
        return myTimingData.beatToMeasure(beat);
    }

    double getBpm(int row) const override {
        if (myTweakTempo) {
            return myTweakTempo->segments->getRecent<BpmChange>(row).bpm;
        } else if (myTempo) {
            return myTempo->segments->getRecent<BpmChange>(row).bpm;
        }
        return SIM_DEFAULT_BPM;
    }

    double rowToScroll(int row) const override {
        return myTimingData.rowToScroll(row);
    }

    double beatToScroll(double beat) const override {
        return myTimingData.beatToScroll(beat);
    }

    double positionToSpeed(double beat, double time) const override {
        return myTimingData.positionToSpeed(beat, time);
    }

    // ================================================================================================
    // TempoManImpl :: get functions.

    TweakMode getTweakMode() const override { return myTweakMode; }

    double getTweakValue() const override { return myTweakValue; }

    int getTweakRow() const override { return myTweakRow; }

    double getOffset() const override {
        if (myTweakTempo) {
            return myTweakTempo->offset;
        } else if (myTempo) {
            return myTempo->offset;
        }
        return 0.0;
    }

    TimingMode getTimingMode() const override {
        bool splitTiming = false;
        for (auto chart : mySimfile->charts) {
            if (chart->hasTempo()) {
                splitTiming = true;
                break;
            }
        }
        if (splitTiming) {
            return (myChart && myChart->hasTempo()) ? TIMING_STEPS
                                                    : TIMING_SONG;
        }
        return TIMING_UNIFIED;
    }

    DisplayBpm getDisplayBpmType() const override {
        return myTempo ? myTempo->displayBpmType : BPM_ACTUAL;
    }

    BpmRange getDisplayBpmRange() const override {
        return myTempo ? myTempo->displayBpmRange : BpmRange{0.0, 0.0};
    }

    BpmRange getBpmRange() const override {
        double low = DBL_MAX, high = 0;
        if (myTempo) {
            auto it = myTempo->segments->begin<BpmChange>();
            auto end = myTempo->segments->end<BpmChange>();
            for (; it != end; ++it) {
                if (it->bpm >= 0) {
                    low = std::min(low, it->bpm);
                    high = std::max(high, it->bpm);
                }
            }
        }
        return (high >= low) ? BpmRange{low, high} : BpmRange{0, 0};
    }

    const TimingData& getTimingData() const override { return myTimingData; }

    const SegmentGroup* getSegments() const override {
        if (myTweakTempo) {
            return myTweakTempo->segments;
        } else if (myTempo) {
            return myTempo->segments;
        }
        return nullptr;
    }

    // ================================================================================================
    // TempoManImpl :: visual sync
    bool isInVisualSync() override { return myVisualSync != std::nullopt; }

    void injectBoundingBpmChange(const int targetRow) override {
        if (targetRow <= 0) {
            return;
        }

        const BpmChange cur_bpm =
            this->myTempo->segments->getRecent<BpmChange>(targetRow);

        if (cur_bpm.row == targetRow) {
            return;
        }

        SegmentEdit edit;
        edit.add.append(BpmChange(targetRow, cur_bpm.bpm));

        gHistory->startChain();
        modify(edit, false);
        gHistory->finishChain("Injected bounding BPM change");
    }

    const double calculateBpmChangeForShift(const int rowDifference,
                                            const double timeDifference) {
        return 60.0 * (static_cast<double>(rowDifference) / 48.) /
               timeDifference;
    }

    std::pair<int, int> getBeatBounds(const int targetRow) {
        return std::pair(48 * ((targetRow - 1) / 48),
                         48 * (targetRow / 48) + 48);
    }

    std::pair<int, int> findClosestBoundingRows(const Chart* chart,
                                                const int targetRow) const {
        int closestBoundingLeftRow = INT32_MIN;
        int closestBoundingRightRow = INT32_MAX;

        auto segmentIt = chart->getTempo(mySimfile)->segments->begin();
        decltype(segmentIt) segmentItEnd =
            chart->getTempo(mySimfile)->segments->end();

        for (; segmentIt != segmentItEnd; ++segmentIt) {
            auto objIt = segmentIt->begin();
            decltype(objIt) objItEnd = segmentIt->end();

            for (; objIt != objItEnd; ++objIt) {
                const int row = objIt->row;

                if (closestBoundingLeftRow < row && row < targetRow) {
                    closestBoundingLeftRow = row;
                }
                if (targetRow < row && row < closestBoundingRightRow) {
                    closestBoundingRightRow = row;
                }
            }
        }

        for (auto& note : chart->notes) {
            if (closestBoundingLeftRow < note.row && note.row < targetRow) {
                closestBoundingLeftRow = note.row;
            }
            if (targetRow < note.row && note.row < closestBoundingRightRow) {
                closestBoundingRightRow = note.row;
            }
            if (closestBoundingLeftRow < note.endrow &&
                note.endrow < targetRow) {
                closestBoundingLeftRow = note.endrow;
            }
            if (targetRow < note.endrow &&
                note.endrow < closestBoundingRightRow) {
                closestBoundingRightRow = note.endrow;
            }
        }

        return std::pair(closestBoundingLeftRow, closestBoundingRightRow);
    }

    std::pair<int, int> calculateVisualSyncBoundaries(const int targetRow) {
        if (myChart == nullptr) {
            HudError(
                "Somehow reached visual sync boundary check without an active "
                "chart. Report this.");
            return std::pair(0, 0);
        }

        int closestBoundingLeftRow, closestBoundingRightRow;
        int candidateLeftBound, candidateRightBound;

        std::tie(closestBoundingLeftRow, closestBoundingRightRow) =
            this->getBeatBounds(targetRow);

        // We must go through ourselves first so that sameChartFlag message
        //   only appears if a different chart genuinely triggered the message
        std::tie(candidateLeftBound, candidateRightBound) =
            findClosestBoundingRows(myChart, targetRow);
        closestBoundingLeftRow =
            std::max(closestBoundingLeftRow, candidateLeftBound);
        closestBoundingRightRow =
            std::min(closestBoundingRightRow, candidateRightBound);

        bool sameChartFlag = true;
        const bool areWeSplitTimed = myChart->hasTempo();
        for (const Chart* chart : mySimfile->charts) {
            if (chart == nullptr) {
                HudError(
                    "There was a nullptr chart in simfile charts list. Report "
                    "this.");
                continue;
            }
            if (chart == myChart) {
                continue;
            }
            const bool isSplitTimed = chart->hasTempo();
            const bool isValidTarget = !areWeSplitTimed && !isSplitTimed;

            if (!isValidTarget) {
                continue;
            }

            std::tie(candidateLeftBound, candidateRightBound) =
                findClosestBoundingRows(chart, targetRow);

            if (closestBoundingLeftRow < candidateLeftBound) {
                closestBoundingLeftRow = candidateLeftBound;
                sameChartFlag = false;
            }
            if (candidateRightBound < closestBoundingRightRow) {
                closestBoundingRightRow = candidateRightBound;
                sameChartFlag = false;
            }
        }

        if (!sameChartFlag) {
            HudWarning(
                "A different chart in this simfile causes the visual sync to "
                "be bound this way.");
        }

        return std::pair(closestBoundingLeftRow, closestBoundingRightRow);
    }

    void startDestructiveVisualSync(const int targetRow) override {
        if (this->isInVisualSync()) {
            return;
        }

        if (targetRow <= 0) {
            myVisualSync.emplace(
                VisualSyncData{.mode = VisualSyncMode::OFFSET});
            gHistory->startChain();
            return;
        }

        BpmChange adjustedBpmChange;
        if (myChart) {
            adjustedBpmChange =
                myChart->getTempo(mySimfile)->segments->getRecent<BpmChange>(
                    targetRow - 1);
        } else {
            adjustedBpmChange =
                mySimfile->tempo->segments->getRecent<BpmChange>(targetRow - 1);
        }

        VisualSyncData syncData{
            .targetRow = targetRow,
            .mode = VisualSyncMode::DESTRUCTIVE,
            .leftLimitRow = adjustedBpmChange.row,
            .initialBpm = adjustedBpmChange.bpm,
        };

        myVisualSync.emplace(syncData);

        gHistory->startChain();
    }

    void startNondestructiveVisualSync(const int targetRow) override {
        if (this->isInVisualSync()) {
            return;
        }

        if (targetRow <= 0) {
            myVisualSync.emplace(
                VisualSyncData{.mode = VisualSyncMode::OFFSET});
            gHistory->startChain();
            return;
        }

        int leftLimitRow, rightLimitRow;
        std::tie(leftLimitRow, rightLimitRow) =
            calculateVisualSyncBoundaries(targetRow);
        const double replicatedBpm = this->getBpm(rightLimitRow);

        VisualSyncData syncData{.targetRow = targetRow,
                                .mode = VisualSyncMode::NON_DESTRUCTIVE,
                                .leftLimitRow = leftLimitRow,
                                .rightLimitRow = rightLimitRow,
                                .initialBpm = replicatedBpm};

        myVisualSync.emplace(syncData);

        gHistory->startChain();
    }

    void tickVisualSync(const double targetTime) override {
        static bool shouldWarn = true;

        if (!this->isInVisualSync()) {
            return;
        }

        // NOTE: deliberately if-else chain because C++'s switch statement suck
        //   and add indentation.
        if (myVisualSync->mode == VisualSyncMode::DESTRUCTIVE) {
            const double bpmChangeTime = rowToTime(myVisualSync->leftLimitRow);
            if (targetTime < bpmChangeTime) {
                if (shouldWarn) {
                    HudWarning(
                        "Cannot move past previous BPM change in "
                        "destructive shift.");
                    shouldWarn = false;
                }
                return;
            }
            shouldWarn = true;
            const double newBpm = calculateBpmChangeForShift(
                myVisualSync->targetRow - myVisualSync->leftLimitRow,
                targetTime - bpmChangeTime);
            SegmentEdit edit;
            edit.add.append(BpmChange(myVisualSync->leftLimitRow, newBpm));
            modify(edit, false);
        } else if (myVisualSync->mode == VisualSyncMode::NON_DESTRUCTIVE) {
            const double leftTime = rowToTime(myVisualSync->leftLimitRow);
            const double rightTime = rowToTime(myVisualSync->rightLimitRow);

            if (targetTime < leftTime || targetTime > rightTime) {
                if (shouldWarn) {
                    HudWarning(
                        "Cannot move outside the bounding area in "
                        "non-destructive shifts.");
                    shouldWarn = false;
                }
                return;
            }
            shouldWarn = true;

            const double newLeftBpm = calculateBpmChangeForShift(
                myVisualSync->targetRow - myVisualSync->leftLimitRow,
                targetTime - leftTime);
            const double newCentralBpm = calculateBpmChangeForShift(
                myVisualSync->rightLimitRow - myVisualSync->targetRow,
                rightTime - targetTime);

            SegmentEdit edit;
            edit.add.append(BpmChange(myVisualSync->leftLimitRow, newLeftBpm));
            edit.add.append(BpmChange(myVisualSync->targetRow, newCentralBpm));
            edit.add.append(BpmChange(myVisualSync->rightLimitRow,
                                      myVisualSync->initialBpm));
            modify(edit, false);
        } else if (myVisualSync->mode == VisualSyncMode::OFFSET) {
            this->setOffset(-targetTime);
        }
        gHistory->updateChain();
        return;
    }

    void endVisualSync() override {
        if (!this->isInVisualSync()) {
            return;
        }

        // NOTE: deliberately if-else chain because C++'s switch statement suck
        //   and add indentation.
        std::string msg;
        if (myVisualSync->mode == VisualSyncMode::DESTRUCTIVE) {
            const double newBpm = this->getBpm(myVisualSync->leftLimitRow);
            std::ostringstream details;
            details << "Shifted anchor row destructively, modifying BPM at row "
                    << myVisualSync->leftLimitRow << " from "
                    << myVisualSync->initialBpm << " to " << newBpm;
            msg = details.str();
        } else if (myVisualSync->mode == VisualSyncMode::NON_DESTRUCTIVE) {
            msg = "Shifted anchor row non-destructively";
        } else if (myVisualSync->mode == VisualSyncMode::OFFSET) {
            msg = "Offset change completed";
        }
        gHistory->finishChain(msg);
        myVisualSync.reset();
    }
};  // TempoManImpl

// ================================================================================================
// Tempo API.

TempoMan* gTempo = nullptr;

void TempoMan::create() { gTempo = new TempoManImpl; }

void TempoMan::destroy() {
    delete TEMPO_MAN;
    gTempo = nullptr;
}

};  // namespace Vortex
