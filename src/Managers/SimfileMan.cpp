#include <Managers/SimfileMan.h>

#include <algorithm>
#include <math.h>
#include <set>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/System.h>
#include <System/File.h>
#include <System/Debug.h>

#include <Simfile/Simfile.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/Parsing.h>
#include <Simfile/TimingData.h>

#include <Editor/Music.h>
#include <Editor/History.h>
#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/Notefield.h>

#include <Managers/MetadataMan.h>
#include <Managers/ChartMan.h>
#include <Managers/StyleMan.h>
#include <Managers/TempoMan.h>
#include <Managers/NoteMan.h>
#include <Managers/NoteskinMan.h>

#define SIMFILE_MAN ((SimfileManImpl*)gSimfile)

namespace Vortex {

// ================================================================================================
// SimfileManImpl :: member data.

struct SimfileManImpl : public SimfileMan {
    Chart* myChart;
    Simfile* mySimfile;

    int myEndRow;
    int myChartIndex;
    bool myBackupOnSave;

    History::EditId myApplyAddChartId;
    History::EditId myApplyRemoveChartId;

    // ================================================================================================
    // SimfileManImpl :: constructor and destructor.

    SimfileManImpl()
        : myChart(nullptr),
          mySimfile(nullptr),
          myEndRow(0),
          myChartIndex(-1),
          myBackupOnSave(false) {
        myApplyAddChartId =
            gHistory->addCallback(ApplyAddChart, ReleaseAddChart);
        myApplyRemoveChartId =
            gHistory->addCallback(ApplyRemoveChart, ReleaseRemoveChart);
    }

    ~SimfileManImpl() { delete mySimfile; }

    // ================================================================================================
    // SimfileManImpl :: update.

    void myUpdateEndRow() {
        if (!mySimfile) return;

        int endRow = 0;

        // Row of the last segment.
        auto segments = mySimfile->tempo->segments;
        for (auto list = segments->begin(), listEnd = segments->end();
             list != listEnd; ++list) {
            for (auto seg = list->begin(), end = list->end(); seg != end;
                 ++seg) {
                endRow = max(endRow, seg->row);
            }
        }

        // Row of the last note.
        if (myChart) {
            for (auto& n : myChart->notes) {
                endRow = max(endRow, n.endrow);
            }
        }

        // Row of the end of the music.
        endRow = max(endRow, gTempo->timeToRow(gMusic->getSongLength()));

        // Round up to the next beat, with at least half a beat leeway.
        int beatRow = endRow + ROWS_PER_BEAT * 3 / 2 - 1;
        beatRow -= beatRow % ROWS_PER_BEAT;

        // Round up to the next measure, with at least half a measure leeway.
        int measureRow = endRow + (ROWS_PER_BEAT * 4) * 3 / 2 - 1;
        measureRow -= measureRow % (ROWS_PER_BEAT * 4);

        // Only use the measure row if the time is not too far off.
        double timeDiff =
            gTempo->rowToTime(measureRow) - gTempo->rowToTime(endRow);
        endRow = (timeDiff < 10.0) ? measureRow : beatRow;

        // Check if the end row has changed.
        if (myEndRow != endRow) {
            myEndRow = endRow;
            gEditor->reportChanges(VCM_END_ROW_CHANGED);
        }
    }

    void myUpdateChart() {
        if (mySimfile) {
            myChartIndex =
                clamp(myChartIndex, -1, mySimfile->charts.size() - 1);
            myChart =
                (myChartIndex >= 0) ? mySimfile->charts[myChartIndex] : nullptr;
        } else {
            myChartIndex = -1;
            myChart = nullptr;
        }

        gStyle->update(myChart);
        gChart->update(myChart);
        gNoteskin->update(myChart);
        gNotes->update(mySimfile, myChart);
        gTempo->update(mySimfile, myChart);

        gEditor->reportChanges(VCM_CHART_CHANGED |
                               VCM_CHART_PROPERTIES_CHANGED);
    }

    void onChanges(int changes) {
        if (changes &
            (VCM_NOTES_CHANGED | VCM_TEMPO_CHANGED | VCM_MUSIC_IS_LOADED)) {
            myUpdateEndRow();
        }
    }

    // ================================================================================================
    // SimfileManImpl :: open and close functions.

    bool load(const std::string& path) {
        close();

        bool loadedFromAudio = false;

        // Check if the path is empty.
        if (path.empty()) return false;

        // Extract the filename and extension.
        Path file(path);
        std::string filename = file.filename();
        std::string ext = file.ext();
        Str::toLower(ext);

        // Create a new simfile.
        mySimfile = new Simfile;
        mySimfile->dir = file.dir();
        mySimfile->file = file.name();
        HudInfo("Opening: %s", filename.c_str());

        // Check if we are loading a stepmania simfile.
        if (ext == "sm" || ext == "ssc" || ext == "dwi" || ext == "osu" ||
            ext == "osz") {
            if (!LoadSimfile(*mySimfile, path)) {
                close();
                return false;
            }
            sortCharts();
            myBackupOnSave = true;
        } else  // If not, assume it's an audio file.
        {
            mySimfile->music = filename;
            mySimfile->tempo->segments->append(BpmChange(0, SIM_DEFAULT_BPM));
            loadedFromAudio = true;
        }

        // Select the last non-edit chart.
        myChartIndex = mySimfile->charts.size() - 1;
        while (myChartIndex > 0 &&
               mySimfile->charts[myChartIndex]->difficulty == DIFF_EDIT) {
            --myChartIndex;
        }
        myUpdateChart();

        // Load music before filling in the metadata.
        gMusic->load();

        // Try to fill in some metadata.
        gMetadata->update(mySimfile);
        if (loadedFromAudio) {
            mySimfile->background = gMetadata->findBackgroundFile();
            mySimfile->banner = gMetadata->findBannerFile();
            mySimfile->title = gMusic->getTitle();
            mySimfile->artist = gMusic->getArtist();
        }

        mySimfile->sanitize();

        gHistory->onFileOpen(mySimfile);

        gEditor->reportChanges(VCM_ALL_CHANGES);

        return true;
    }

    bool save(const std::string& dir, const std::string& name,
              SimFormat format) {
        if (!mySimfile) return false;

        // Update the song directory and filename.
        mySimfile->dir = dir;
        mySimfile->file = name;

        // Save the simfile.
        bool result = SaveSimfile(*mySimfile, format, myBackupOnSave);
        myBackupOnSave = false;

        gHistory->onFileSaved();

        return result;
    }

    void close() {
        if (!mySimfile) return;

        delete mySimfile;
        mySimfile = nullptr;

        myUpdateChart();
        myEndRow = 0;
        myBackupOnSave = false;

        gMusic->unload();
        gHistory->onFileClosed();

        gEditor->reportChanges(VCM_ALL_CHANGES);
    }

    // ================================================================================================
    // SimfileManImpl :: add chart.

    static void ReleaseAddChart(ReadStream& in, bool hasBeenApplied) {
        auto chart = in.read<Chart*>();
        if (in.success() && !hasBeenApplied) {
            delete chart;
        }
    }

    static std::string ApplyAddChart(ReadStream& in, History::Bindings bound,
                                     bool undo, bool redo) {
        std::string msg;
        auto chart = in.read<Chart*>();
        if (in.success()) {
            if (undo) {
                msg = SIMFILE_MAN->applyRemove(chart);
            } else {
                msg = SIMFILE_MAN->applyAdd(chart);
            }
        }
        return msg;
    }

    void addChart(const Style* style, std::string artist, Difficulty difficulty,
                  int meter) {
        Chart* chart = new Chart;

        chart->style = style;
        chart->artist = artist;
        chart->difficulty = difficulty;
        chart->meter = meter;

        gHistory->addEntry(myApplyAddChartId, &chart, sizeof(chart));
    }

    // ================================================================================================
    // SimfileManImpl :: apply remove chart.

    static void ReleaseRemoveChart(ReadStream& in, bool hasBeenApplied) {
        auto chart = in.read<Chart*>();
        if (in.success() && hasBeenApplied) {
            delete chart;
        }
    }

    static std::string ApplyRemoveChart(ReadStream& in, History::Bindings bound,
                                        bool undo, bool redo) {
        std::string msg;
        auto chart = in.read<Chart*>();
        if (in.success()) {
            if (undo) {
                msg = SIMFILE_MAN->applyAdd(chart);
            } else {
                msg = SIMFILE_MAN->applyRemove(chart);
            }
        }
        return msg;
    }

    void removeChart(const Chart* chart) {
        if (!chart) return;

        if (mySimfile) {
            int pos = mySimfile->charts.find((Chart*)chart);
            if (pos == mySimfile->charts.size()) {
                HudError(
                    "Trying to remove a chart that is not in the chart list.");
            } else {
                gHistory->addEntry(myApplyRemoveChartId, &chart, sizeof(chart));
            }
        }
    }

    // ================================================================================================
    // SimfileManImpl :: misc functions.

    /*void removeRolls()
    {
            for(auto chart : mySimfile->charts)
            {
                    for(auto& note : chart->notes)
                    {
                            if(note.type == NOTE_ROLL && note.endrow == note.row
    + 5)
                            {
                                    note.type = NOTE_STEP_OR_HOLD;
                                    note.endrow = note.row;
                            }
                    }
            }
    }*/

    /*void doMagic()
    {
            if(mySimfile->charts.empty()) return;
            auto it = mySimfile->charts.back()->notes.begin();
            auto end = mySimfile->charts.back()->notes.end();
            if(it == end) return;

            TimingData td;
            td.update(mySimfile->tempo);

            mySimfile->tempo->offset = -td.rowToTime(it->row);

            Vector<Vector<double>> timestamps;
            for(auto& chart : mySimfile->charts)
            {
                    auto& out = timestamps.push_back();
                    for(auto& note : chart->notes)
                    {
                            out.push_back(td.rowToTime(note.row));
                            if(note.row != note.endrow)
                            {
                                    out.push_back(td.rowToTime(note.endrow));
                            }
                    }
            }

            int row = 0;
            Vector<BpmChange> bpms;
            double curBpm = mySimfile->tempo->bpmChanges[0].bpm;
            int dists[] = {12, 24, 48};
            for(; it + 1 != end; ++it)
            {
                    double diff = td.rowToTime((it + 1)->row) -
    td.rowToTime(it->row); if(diff > 0)
                    {
                            //int i = rand() % 3;
                            int i = 1;
                            double bpm = (60.0 * dists[i] / ROWS_PER_BEAT) /
    diff; if(i == 2 && bpm > 300.0) { --i; bpm *= 2; bpm /= 3; } if(i == 1 &&
    bpm > 300.0) { --i; bpm /= 2; } if(i == 0 && bpm < 80.0) { ++i; bpm *= 2; }
                            if(i == 1 && bpm < 40.0) { ++i; bpm *= 2; }
                            if(curBpm != bpm)
                            {
                                    bpms.push_back({row, bpm});
                                    curBpm = bpm;
                            }
                            row += dists[i];
                    }
            }

            double spm = SecPerRow(bpms[0].bpm) * ROWS_PER_MEASURE;
            double timeOffset = 0.0;
            int rowOffset = 0;
            while(mySimfile->tempo->offset < -spm)
            {
                    mySimfile->tempo->offset += spm;
                    rowOffset += ROWS_PER_MEASURE;
            }
            for(auto& bpm : bpms)
            {
                    bpm.row += rowOffset;
            }
            bpms[0].row = 0;

            mySimfile->tempo->stops.release();
            mySimfile->tempo->bpmChanges.swap(bpms);
            SanitizeBpms(mySimfile->tempo->bpmChanges, "Song", true);

            td.update(mySimfile->tempo);

            int i = 0;
            for(auto& chart : mySimfile->charts)
            {
                    double* time = timestamps[i++].begin();
                    for(auto& note : chart->notes)
                    {
                            if(note.row != note.endrow)
                            {
                                    note.row = td.timeToRow(timeOffset +
    *time++); note.endrow = td.timeToRow(timeOffset + *time++);
                            }
                            else
                            {
                                    note.row = note.endrow =
    td.timeToRow(timeOffset + *time++);
                            }
                            note.row = ((note.row + 6) / 12) * 12;
                            note.endrow = ((note.endrow + 6) / 12) * 12;
                    }
            }

            SanitizeCharts(mySimfile->charts);
    }*/

    // ================================================================================================
    // SimfileManImpl :: low level chart functions.

    std::string applyAdd(Chart* chart) {
        mySimfile->charts.push_back(chart);
        sortCharts();
        openChart(chart);
        gEditor->reportChanges(VCM_CHART_LIST_CHANGED);
        return "Added chart: " + chart->description();
    }

    std::string applyRemove(Chart* chart) {
        int pos = mySimfile->charts.find(chart);
        if (pos == mySimfile->charts.size()) {
            std::string err = "Failed to remove " + chart->description() +
                              ", could not find it.";
            HudError("%s", err.c_str());
        } else {
            mySimfile->charts.erase(pos);
            myUpdateChart();
        }
        gEditor->reportChanges(VCM_CHART_LIST_CHANGED);
        return "Removed chart: " + chart->description();
    }

    void sortCharts() {
        std::stable_sort(mySimfile->charts.begin(), mySimfile->charts.end(),
                         [](const Chart* a, const Chart* b) {
                             if (a->style != b->style) {
                                 return a->style->index < b->style->index;
                             }
                             if (a->difficulty != b->difficulty) {
                                 return a->difficulty < b->difficulty;
                             }
                             return a->meter < b->meter;
                         });
    }

    // ================================================================================================
    // SimfileManImpl :: open chart.

    void openChart(int index) {
        if (mySimfile && myChartIndex != index && index >= -1 &&
            index < mySimfile->charts.size()) {
            myChartIndex = index;
            myUpdateChart();
            if (myChart) {
                std::string desc = myChart->description();
                HudNote("Switched to %s :: %s", myChart->style->name.c_str(),
                        desc.c_str());
            } else {
                HudNote("Switched to sync mode");
            }
        }
    }

    void openChart(const Chart* chart) {
        if (mySimfile) {
            for (int i = 0; i < mySimfile->charts.size(); ++i) {
                if (mySimfile->charts[i] == chart) {
                    openChart(i);
                    return;
                }
            }
        }
        HudWarning("Trying to open a chart that is not in the chart list.");
    }

    void nextChart() { openChart(myChartIndex + 1); }

    void previousChart() { openChart(myChartIndex - 1); }

    // ================================================================================================
    // SimfileManImpl :: get functions.

    std::string getDir() const {
        return mySimfile ? mySimfile->dir : std::string();
    }

    std::string getFile() const {
        return mySimfile ? mySimfile->file : std::string();
    }

    int getNumCharts() const {
        return mySimfile ? mySimfile->charts.size() : 0;
    }

    int getActiveChart() const { return myChartIndex; }

    const Chart* getChart(int index) const {
        return mySimfile->charts.begin()[index];
    }

    int getEndRow() const { return myEndRow; }

    bool isOpen() const { return mySimfile != nullptr; }

    bool isClosed() const { return mySimfile == nullptr; }

    const Simfile* get() const { return mySimfile; }

};  // SimfileManImpl

// ================================================================================================
// Song :: create and destroy.

SimfileMan* gSimfile = nullptr;

void SimfileMan::create() { gSimfile = new SimfileManImpl; }

void SimfileMan::destroy() {
    delete SIMFILE_MAN;
    gSimfile = nullptr;
}

};  // namespace Vortex
