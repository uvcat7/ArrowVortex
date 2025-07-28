#include <Managers/ChartMan.h>

#include <Core/StringUtils.h>

#include <Simfile/Tempo.h>

#include <Editor/History.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/RatingEstimator.h>

#include <Managers/SimfileMan.h>
#include <Managers/NoteMan.h>

namespace Vortex {

#define CHART_MAN ((ChartManImpl*)gChart)

struct ChartManImpl : public ChartMan {
    // ================================================================================================
    // ChartManImpl :: member data.

    Chart* myChart;

    History::EditId myApplyStepArtistId;
    History::EditId myApplyMeterId;
    History::EditId myApplyDifficultyId;

    // ================================================================================================
    // ChartManImpl :: constructor and destructor.

    ~ChartManImpl() {}

    ChartManImpl() {
        myChart = nullptr;

        myApplyStepArtistId = gHistory->addCallback(ApplyStepArtist);
        myApplyMeterId = gHistory->addCallback(ApplyMeter);
        myApplyDifficultyId = gHistory->addCallback(ApplyDifficulty);
    }

    // ================================================================================================
    // ChartManImpl :: update functions.

    void update(Chart* chart) { myChart = chart; }

    // ================================================================================================
    // ChartManImpl :: step artist editing.

    void myQueueStepArtist(std::string artist) {
        WriteStream stream;
        stream.writeStr(myChart->artist);
        stream.writeStr(artist);
        gHistory->addEntry(myApplyStepArtistId, stream.data(), stream.size(),
                           myChart);
    }

    static std::string ApplyStepArtist(ReadStream& in, History::Bindings bound,
                                       bool undo, bool redo) {
        std::string msg;
        std::string before = in.readStr();
        std::string after = in.readStr();
        if (in.success()) {
            const std::string& newVal = undo ? before : after;

            msg = bound.chart->description();
            msg = msg + " :: ";
            msg = msg + (undo ? "reverted" : "changed");
            msg = msg + " step artist to ";
            msg = msg + newVal;

            gSimfile->openChart(bound.chart);
            bound.chart->artist = newVal;
            gEditor->reportChanges(VCM_CHART_PROPERTIES_CHANGED);
        }
        return msg;
    }

    // ================================================================================================
    // ChartManImpl :: meter editing.

    void myQueueMeter(int meter) {
        WriteStream stream;
        stream.write<int>(myChart->meter);
        stream.write<int>(meter);
        gHistory->addEntry(myApplyMeterId, stream.data(), stream.size(),
                           myChart);
    }

    static std::string ApplyMeter(ReadStream& in, History::Bindings bound,
                                  bool undo, bool redo) {
        std::string msg;
        int before = in.read<int>();
        int after = in.read<int>();
        if (in.success()) {
            int newVal = undo ? before : after;

            msg = bound.chart->description();
            msg = msg + " :: ";
            msg = msg + (undo ? "reverted" : "changed");
            msg = msg + " meter to ";
            Str::appendVal(msg, newVal);

            gSimfile->openChart(bound.chart);
            bound.chart->meter = newVal;
            gEditor->reportChanges(VCM_CHART_PROPERTIES_CHANGED);
        }
        return msg;
    }

    // ================================================================================================
    // ChartManImpl :: difficulty editing.

    void myQueueDifficulty(Difficulty difficulty) {
        WriteStream stream;
        stream.write<int>(myChart->difficulty);
        stream.write<int>(difficulty);
        gHistory->addEntry(myApplyDifficultyId, stream.data(), stream.size(),
                           myChart);
    }

    static std::string ApplyDifficulty(ReadStream& in, History::Bindings bound,
                                       bool undo, bool redo) {
        std::string msg;
        int before = in.read<int>();
        int after = in.read<int>();
        if (in.success()) {
            Difficulty newDiff = (Difficulty)(undo ? before : after);

            msg = bound.chart->description();
            msg = msg + " :: ";
            msg = msg + (undo ? "reverted" : "changed");
            msg = msg + " difficulty to ";
            msg = msg + GetDifficultyName(newDiff);

            gSimfile->openChart(bound.chart);
            bound.chart->difficulty = newDiff;
            gEditor->reportChanges(VCM_CHART_PROPERTIES_CHANGED);
        }
        return msg;
    }

    // ================================================================================================
    // ChartManImpl :: set functions.

    void setDifficulty(Difficulty difficulty) {
        if (myChart && myChart->difficulty != difficulty) {
            myQueueDifficulty(difficulty);
        }
    }

    void setStepArtist(std::string artist) {
        if (myChart && myChart->artist != artist) {
            myQueueStepArtist(artist);
        }
    }

    void setMeter(int meter) {
        if (myChart && myChart->meter != meter) {
            myQueueMeter(meter);
        }
    }

    // ================================================================================================
    // ChartManImpl :: get functions.

    bool isOpen() const { return (myChart != nullptr); }

    bool isClosed() const { return (myChart == nullptr); }

    std::string getStepArtist() const {
        return myChart ? myChart->artist : std::string();
    }

    Difficulty getDifficulty() const {
        return myChart ? myChart->difficulty : DIFF_BEGINNER;
    }

    int getMeter() const { return myChart ? myChart->meter : 1; }

    std::string getDescription() const {
        return myChart ? myChart->description() : std::string();
    }

    const Chart* get() const { return myChart; }

    // ================================================================================================
    // ChartManImpl :: stream breakdown.

    struct StreamItem {
        int row, endrow;
        bool isBreak;
    };

    static Vector<BreakdownItem> ToBreakdown(const Vector<StreamItem>& items) {
        Vector<BreakdownItem> out;
        BreakdownItem item = {-1, -1};
        for (auto& it : items) {
            if (it.isBreak) {
                if (it.endrow - it.row > (ROWS_PER_BEAT * 4)) {
                    out.push_back(item);
                    item.row = item.endrow = -1;
                    item.text.clear();
                } else {
                    item.text = item.text + "-";
                }
            } else {
                Str::appendVal(item.text,
                               (it.endrow - it.row + 48) / (ROWS_PER_BEAT * 4));
                if (item.row == -1) item.row = it.row;
                item.endrow = it.endrow;
            }
        }
        if (item.row != -1) out.push_back(item);
        return out;
    }

    static void MergeItems(Vector<StreamItem>& items) {
        // Merge successive streams and breaks into a single item.
        for (int i = items.size() - 1; i > 0; --i) {
            if (items[i].isBreak == items[i - 1].isBreak) {
                items[i - 1].endrow = items[i].endrow;
                items.erase(i);
            }
        }

        // Remove breaks at the front and back of the list.
        if (items.size() && items.back().isBreak) items.pop_back();
        if (items.size() && items[0].isBreak) items.erase(0);
    }

    static void RemoveItems(Vector<StreamItem>& items, int minRows,
                            bool breaks) {
        for (int i = items.size() - 1; i >= 0; --i) {
            if ((items[i].endrow - items[i].row) < minRows &&
                items[i].isBreak == breaks) {
                items.erase(i);
            }
        }
        MergeItems(items);
    }

    Vector<BreakdownItem> getStreamBreakdown(int* totalMeasures) const {
        int dummy;
        if (!totalMeasures) totalMeasures = &dummy;

        *totalMeasures = 0;
        Vector<BreakdownItem> out;
        if (gNotes->empty()) return out;

        // Skip mines at the start and end.
        auto first = gNotes->begin();
        auto last = gNotes->end() - 1;
        while (first != last && first->isMine) ++first;
        while (last != first && last->isMine) --last;
        if (last == first) return out;

        // Build a list of streams and breaks.
        Vector<StreamItem> items;
        const ExpandedNote *prevStreamEnd = nullptr, *streamBegin = nullptr;
        for (const ExpandedNote *n = first, *next; n != last; n = next) {
            next = n + 1;
            while (next->isMine) ++next;
            bool isStreamNote = (next->row - n->row <= 12);

            if (isStreamNote) {
                if (streamBegin == nullptr) {
                    streamBegin = n;
                }
            }

            if (next == last || !isStreamNote) {
                auto streamEnd = n;
                if (streamBegin &&
                    streamEnd->row >= streamBegin->row + (ROWS_PER_BEAT * 4)) {
                    if (prevStreamEnd &&
                        streamBegin->row > prevStreamEnd->row) {
                        items.push_back(
                            {prevStreamEnd->row, streamBegin->row, true});
                    }
                    items.push_back({streamBegin->row, streamEnd->row, false});
                    prevStreamEnd = streamEnd;
                }
                streamBegin = nullptr;
            }
        }

        for (auto& i : items) {
            if (!i.isBreak) {
                *totalMeasures += (i.endrow - i.row + 48) / (ROWS_PER_BEAT * 4);
            }
        }

        // Merge streams with breaks shorter than half a beat.
        const int rpb = ROWS_PER_BEAT;
        RemoveItems(items, rpb / 2, true);

        // Merge/remove more streams if the breakdown is too long.
        if (items.size() > 28) RemoveItems(items, rpb * 8, false);
        if (items.size() > 28) RemoveItems(items, rpb * 2, true);
        if (items.size() > 28) RemoveItems(items, rpb * 16, false);
        if (items.size() > 28) RemoveItems(items, rpb * 4, true);
        if (items.size() > 28) RemoveItems(items, rpb * 24, false);
        if (items.size() > 28) RemoveItems(items, rpb * 6, true);
        if (items.size() > 28) RemoveItems(items, rpb * 32, false);

        // Finally, construct the breakdown.
        return ToBreakdown(items);
    }

    // ================================================================================================
    // ChartManImpl :: meter estimation.

    double getEstimatedMeter() const {
        RatingEstimator hm("assets/rating estimate data.txt");
        return hm.estimateRating();
    }

};  // ChartManImpl

// ================================================================================================
// Chart :: create and destroy.

ChartMan* gChart = nullptr;

void ChartMan::create() { gChart = new ChartManImpl; }

void ChartMan::destroy() {
    delete CHART_MAN;
    gChart = nullptr;
}

};  // namespace Vortex
