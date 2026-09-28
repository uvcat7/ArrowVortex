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

    ~ChartManImpl() = default;

    ChartManImpl() {
        myChart = nullptr;

        myApplyStepArtistId = gHistory->addCallback(ApplyStepArtist);
        myApplyMeterId = gHistory->addCallback(ApplyMeter);
        myApplyDifficultyId = gHistory->addCallback(ApplyDifficulty);
    }

    // ================================================================================================
    // ChartManImpl :: update functions.

    void update(Chart* chart) override { myChart = chart; }

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
            Difficulty newDiff = static_cast<Difficulty>(undo ? before : after);

            msg = bound.chart->description();
            msg = msg + " :: ";
            msg = msg + (undo ? "reverted" : "changed");
            msg = msg + " difficulty to ";
            msg = msg + GetDifficultyName(newDiff);

            gSimfile->openChart(bound.chart);
            bound.chart->difficulty = newDiff;
            gSimfile->sortCharts();
            gEditor->reportChanges(VCM_CHART_PROPERTIES_CHANGED);
        }
        return msg;
    }

    // ================================================================================================
    // ChartManImpl :: set functions.

    void setDifficulty(Difficulty difficulty) override {
        if (myChart && myChart->difficulty != difficulty) {
            myQueueDifficulty(difficulty);
        }
    }

    void setStepArtist(std::string artist) override {
        if (myChart && myChart->artist != artist) {
            myQueueStepArtist(artist);
        }
    }

    void setMeter(int meter) override {
        if (myChart && myChart->meter != meter) {
            myQueueMeter(meter);
        }
    }

    // ================================================================================================
    // ChartManImpl :: get functions.

    bool isOpen() const override { return (myChart != nullptr); }

    bool isClosed() const override { return (myChart == nullptr); }

    std::string getStepArtist() const override {
        return myChart ? myChart->artist : std::string();
    }

    Difficulty getDifficulty() const override {
        return myChart ? myChart->difficulty : DIFF_BEGINNER;
    }

    int getMeter() const override { return myChart ? myChart->meter : 1; }

    std::string getDescription() const override {
        return myChart ? myChart->description() : std::string();
    }

    const Chart* get() const override { return myChart; }

    // ================================================================================================
    // ChartManImpl :: stream breakdown.

    enum StreamType { STREAM_BREAK, STREAM_16TH, STREAM_24TH, STREAM_32ND };

    struct StreamItem {
        int row, endrow;
        StreamType type;
    };

    static std::vector<BreakdownItem> ToBreakdown(
        const std::vector<StreamItem>& items) {
        std::vector<BreakdownItem> out;
        BreakdownItem item = {-1, -1};
        for (auto it = items.begin(); it < items.end(); std::advance(it, 1)) {
            // Skip breaks at the beginning and end
             if ((it == items.begin() ||
                std::next(it) == items.end()) && it->type == STREAM_BREAK)
                continue;

            int measures = (it->endrow - it->row) / ROWS_PER_MEASURE;

            // Skip one-measure breaks, including 24th/16th breaks in 32nd/24th
            // streams
            if (measures <= 1 && it != items.begin() && std::next(it) != items.end() &&
                it->type < std::prev(it)->type &&
                it->type < std::next(it)->type)
                continue;

            switch (it->type) {
                case STREAM_BREAK:
                    item.text = "(" + std::to_string(measures) + ")";
                    break;
                case STREAM_16TH:
                    item.text = std::to_string(measures);
                    break;
                case STREAM_24TH:
                    item.text = "/" + std::to_string(measures) + "/";
                    break;
                case STREAM_32ND:
                    item.text = "*" + std::to_string(measures) + "*";
                    break;
            }
            item.row = it->row;
            item.endrow = it->endrow;
            out.emplace_back(item);
        }
        return out;
    }

    static void MergeItems(std::vector<StreamItem>& items) {
        // Merge successive streams and breaks into a single item.
        for (int i = items.size() - 1; i > 0; --i) {
            if (items[i].type == items[i - 1].type) {
                items[i - 1].endrow = items[i].endrow;
                items.erase(items.begin() + i);
            }
        }

        // Remove breaks at the front and back of the list.
        if (items.size() && items.back().type == STREAM_BREAK) items.pop_back();
        if (items.size() && items[0].type == STREAM_BREAK)
            items.erase(items.begin());
    }

    static void RemoveItems(std::vector<StreamItem>& items, int minRows,
                            bool breaks) {
        for (int i = items.size() - 1; i >= 0; --i) {
            if ((items[i].endrow - items[i].row) < minRows &&
                (items[i].type == STREAM_BREAK) == breaks) {
                items.erase(items.begin() + i);
            }
        }
        MergeItems(items);
    }

    std::vector<BreakdownItem> getStreamBreakdown(
        int* totalMeasures, int* total16thMeasures) const override {
        int dummy;
        if (!totalMeasures) 
            totalMeasures = &dummy;
        if (!total16thMeasures) total16thMeasures = &dummy;

        *totalMeasures = 0;
        double total_16ths = 0;
        std::vector<BreakdownItem> out;
        if (gNotes->empty()) return out;

        // Skip mines at the start and end.
        auto first = gNotes->begin();
        auto last = std::prev(gNotes->end());
        while (first != last && first->isMine) ++first;
        while (last != first && last->isMine) --last;
        if (last == first) return out;

        auto last_measure = [&](int row) {
            return std::max(0, (row / ROWS_PER_MEASURE - 1) * ROWS_PER_MEASURE);
        };

        auto start_of_measure = [&](int row) {
            return (row / ROWS_PER_MEASURE) * ROWS_PER_MEASURE;
        };

        auto next_measure = [&](int row) {
            return (row / ROWS_PER_MEASURE + 1) * ROWS_PER_MEASURE;
        };

        // Build a list of streams and breaks.
        std::vector<StreamItem> items;
        const ExpandedNote* sequence_end = nullptr;
        StreamType last_measure_type = STREAM_BREAK,
                   sequence_type = STREAM_BREAK;
        int sequence_begin = 0;
        int note_measure = 0;
        int notes_in_measure = 0;
        for (const ExpandedNote *n = first, *next; n <= gNotes->end(); n = next) {
            next = std::next(n);
            // Mines, fakes, and jumps/brackets/etc. aren't stream, skip them
            while ((next->isMine || next->isFake || next->row == n->row) && next <= last)
                ++next;

            int row = 0;
            // Make a dummy note after the last note to end whatever we currently have going on
            if (n > last)
                row = next_measure(last->endrow + ROWS_PER_MEASURE);
            else
                row = n->row;


            if (note_measure != start_of_measure(row)) {
                if (notes_in_measure >= 32) {
                    last_measure_type = STREAM_32ND;
                } else if (notes_in_measure >= 24) {
                    last_measure_type = STREAM_24TH;
                } else if (notes_in_measure >= 16) {
                    last_measure_type = STREAM_16TH;
                } else {
                    last_measure_type = STREAM_BREAK;
                }
                notes_in_measure = 1;
            } else {
                notes_in_measure++;
            }

            // If measure gaps between two streams, add a break
            if (next_measure(note_measure) < start_of_measure(row) &&
                sequence_type != STREAM_BREAK) {
                int new_measure = last_measure_type == STREAM_BREAK
                                      ? start_of_measure(note_measure)
                                      : next_measure(note_measure);
                items.emplace_back(sequence_begin, new_measure, sequence_type);
                sequence_type = STREAM_BREAK;
                sequence_begin = new_measure;
            }

            // Forcibly add whatever we have if it is the last note
            if (last_measure_type != sequence_type || (n > last && sequence_begin != last_measure(row))) {
                items.emplace_back(sequence_begin, last_measure(row),
                                   sequence_type);
                sequence_type = last_measure_type;
                sequence_begin = last_measure(row);
            }

            note_measure = start_of_measure(row);
        }

        for (auto& i : items) {
            if (i.type != STREAM_BREAK) {
                int measures = (i.endrow - i.row + ROWS_PER_BEAT) /
                    ROWS_PER_MEASURE;
                *totalMeasures += measures;
                switch (i.type) { 
                    case STREAM_16TH:
                        total_16ths += measures;
                        break;
                    case STREAM_24TH:
                        total_16ths += measures * 1.5;
                        break;
                    case STREAM_32ND:
                        total_16ths += measures * 2;
                        break;
                }
            }
        }

        *total16thMeasures = static_cast<int>(total_16ths);

        // Merge streams with breaks shorter than half a beat.
        const int rpb = ROWS_PER_BEAT;
        // RemoveItems(items, rpb / 2, true);

        //// Merge/remove more streams if the breakdown is too long.
        // if (items.size() > 28) RemoveItems(items, rpb * 8, false);
        // if (items.size() > 28) RemoveItems(items, rpb * 2, true);
        // if (items.size() > 28) RemoveItems(items, rpb * 16, false);
        // if (items.size() > 28) RemoveItems(items, rpb * 4, true);
        // if (items.size() > 28) RemoveItems(items, rpb * 24, false);
        // if (items.size() > 28) RemoveItems(items, rpb * 6, true);
        // if (items.size() > 28) RemoveItems(items, rpb * 32, false);

        // Finally, construct the breakdown.
        return ToBreakdown(items);
    }

    // ================================================================================================
    // ChartManImpl :: meter estimation.

    double getEstimatedMeter() const override {
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
