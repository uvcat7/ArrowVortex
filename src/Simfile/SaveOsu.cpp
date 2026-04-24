#include <Core/Core.h>

#include <set>
#include <map>
#include <algorithm>
#include <fstream>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/Debug.h>
#include <System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/TimingData.h>

#include <Managers/StyleMan.h>

namespace Vortex {
namespace Osu {
namespace {

// ===================================================================================
// Exporting utilities.

static int ToMilliseconds(double time) {
    return static_cast<int>(time * 1000.0 + 0.5);
}

static void WriteBlock(std::ofstream& out, const char* name) {
    out << "\n[" << name << "]\n";
}

static void Write(std::ofstream& out, const char* str) { out << str << '\n'; }

static void Write(std::ofstream& out, const char* name, const char* val) {
    out << name << ':' << val << '\n';
}

static void Write(std::ofstream& out, const char* name,
                  const std::string& val) {
    Write(out, name, val.c_str());
}

static void Write(std::ofstream& out, const char* name, int val) {
    Write(out, name, Str::val(val));
}

// ===================================================================================
// Timing exporting.

struct ExportTP {
    double time;
    double spr;
};

template <typename T>
static int FindNextRow(const Vector<T>& list, int row) {
    const T* it =
        std::lower_bound(list.begin(), list.end(), row,
                         [](const T& a, int row) { return a.row < row; });
    return (it != list.end()) ? it->row : INT_MAX;
}

static int FindNextNoteRow(const NoteList& list, int row) {
    const Note* it =
        std::lower_bound(list.begin(), list.end(), row,
                         [](const Note& a, int row) { return a.row < row; });
    while (it != list.end() && it->type != NOTE_STEP_OR_HOLD) ++it;
    return (it != list.end()) ? it->row : INT_MAX;
}

static void ConvertStop(Vector<ExportTP>& tps, const TimingData::Event* cur,
                        const TimingData::Event* next, const Chart* chart) {
    // Find the row on which the next BPM change, stop, or note occurs.
    int endRow = cur->row + 48;

    int nextNote = FindNextNoteRow(chart->notes, cur->row + 1);

    if (nextNote < endRow) {
        endRow = nextNote;
    }

    if (next && next->row < endRow) {
        endRow = next->row;
    }

    // Advance time to the end row.
    double time = cur->time + (endRow - cur->row) * cur->spr;

    // Insert the temporary BPM change.
    double timeDiff =
        (cur->endTime - cur->time) + (endRow - cur->row) * cur->spr;
    double rowDiff = endRow - cur->row;
    tps.push_back({cur->time, timeDiff / rowDiff});

    // Revert back to actual BPM after the change.
    tps.push_back({cur->endTime, cur->spr});
}

static void WriteTimingPoints(std::ofstream& out, const Chart* chart,
                              const TimingData& timing, const Tempo* tempo) {
    Vector<ExportTP> tps;
    tps.reserve(32);

    // Create a list of timing point from the tempo.
    auto it = timing.events.begin();
    auto end = timing.events.end();
    for (; it != end; ++it) {
        if (it->endTime > it->time) {
            auto next = it + 1;
            if (next == end) next = nullptr;
            ConvertStop(tps, it, next, chart);
        } else {
            tps.push_back({it->time, it->spr});
        }
    }

    // Output the timing points.
    for (auto& tp : tps) {
        double msPerBeat = tp.spr * 48.0 * 1000.0;
        double time = tp.time;
        if (time < 0) {
            double secPerMeasure = (msPerBeat / 1000.0) * 4.0;
            time = secPerMeasure - fmod(-time, secPerMeasure);
        }
        out << ToMilliseconds(time) << ',' << msPerBeat << ",4,1,0,100,1,0\n";
    }
}

// ===================================================================================
// Notes exporting.

static void WriteNotes(std::ofstream& out, const Chart* chart,
                       const TimingData& timing) {
    int numCols = chart->style->numCols;
    int colWidth = 512 / numCols;

    // Export the notes as hit objects.
    TempoTimeTracker tracker(timing);
    for (auto& note : chart->notes) {
        if (note.type != NOTE_MINE) {
            // Conver the note's [column, row] to [x, time].
            int x = note.col * colWidth + colWidth / 2;
            double time = tracker.advance(note.row);
            double endtime = time;
            if (note.endrow > note.row) {
                endtime = timing.rowToTime(note.endrow);
            }

            // Write the output hitobject.
            out << x << ",192," << ToMilliseconds(time);
            if (endtime > time)
                out << ",128,0," << ToMilliseconds(endtime) << ":0:0:0:0:\n";
            else
                out << ",1,0,0:0:0:0:\n";
        }
    }
}

// ===================================================================================
// Chart exporting.

static void SaveChart(fs::path path, const Simfile* sim, const Chart* chart) {
    // Open the output file.
    std::ofstream out(path.c_str());
    if (out.fail()) return;

    // Write the version number;
    Write(out, "osu file format v14");

    // Write general properties.
    WriteBlock(out, "General");
    Write(out, "AudioFilename", sim->music);
    Write(out, "AudioLeadIn", "0");
    Write(out, "PreviewTime", "-1");
    Write(out, "Countdown", "0");
    Write(out, "SampleSet", "Soft");
    Write(out, "StackLeniency", "0.7");
    Write(out, "Mode", "3");
    Write(out, "LetterboxInBreaks", "0");
    Write(out, "SpecialStyle", "0");
    Write(out, "WidescreenStoryboard", "0");

    // Write editor properties.
    WriteBlock(out, "Editor");
    Write(out, "DistanceSpacing", "0.8");
    Write(out, "BeatDivisor", "2");
    Write(out, "GridSize", "32");
    Write(out, "TimelineZoom", "1");

    // Write metadata.
    WriteBlock(out, "Metadata");

    Write(out, "Title", sim->titleTr.length() ? sim->titleTr : sim->title);
    Write(out, "TitleUnicode", sim->title);
    Write(out, "Artist", sim->artistTr.length() ? sim->artistTr : sim->artist);
    Write(out, "ArtistUnicode", sim->artist);
    Write(out, "Creator", chart ? chart->artist : "Unknown");
    Write(out, "Version",
          chart ? GetDifficultyName(chart->difficulty) : "Normal");
    Write(out, "Source", "");
    Write(out, "Tags", "");
    Write(out, "BeatmapID", "0");
    Write(out, "BeatmapSetID", "-1");

    // Write difficulty.
    WriteBlock(out, "Difficulty");

    int numCols = chart ? chart->style->numCols : 4;

    Write(out, "HPDrainRate", "5");
    Write(out, "CircleSize", numCols);
    Write(out, "OverallDifficulty", chart ? chart->meter : 1);
    Write(out, "ApproachRate", "5");
    Write(out, "SliderMultiplier", "1.4");
    Write(out, "SliderTickRate", "1");

    // Write events.
    WriteBlock(out, "Events");

    Write(out, "//Background and Video events");
    if (sim->background.length())
        out << "0,0,\"" << sim->background.length() << "\",0,0\n";
    Write(out, "//Break Periods");
    Write(out, "//Storyboard Layer 0 (Background)");
    Write(out, "//Storyboard Layer 1 (Fail");
    Write(out, "//Storyboard Layer 2 (Pass");
    Write(out, "//Storyboard Layer 3 (Foreground)");
    Write(out, "//Storyboard Sound Samples");

    // Create timing data.
    auto tempo = chart->getTempo(sim);
    TimingData timing;
    timing.update(tempo);

    // Write timing points.
    WriteBlock(out, "TimingPoints");
    WriteTimingPoints(out, chart, timing, tempo);

    // Write notes.
    WriteBlock(out, "HitObjects");
    WriteNotes(out, chart, timing);

    out.close();
}

};  // namespace

bool SaveOsu(const Simfile* sim, bool backup) {
    if (sim->charts.empty()) {
        fs::path path = utf8ToPath(sim->dir);
        path.append(stringToUtf8(sim->file));
        path.append(".osu");
        SaveChart(path, sim, nullptr);
        HudInfo("Saved: %s", pathToUtf8(path.filename()).c_str());
    } else {
        std::map<std::string, int> duplicateCounters;
        for (auto chart : sim->charts) {
            fs::path path = fs::path(sim->dir.c_str());
            std::string file = " [";
            file += std::string(GetDifficultyName(chart->difficulty));
            int& counter = duplicateCounters[file];
            if (++counter > 1) {
                file += Str::fmt(" %1").arg(counter).str;
            }
            file += "].osu";
            path.append(stringToUtf8(sim->file));
            SaveChart(path, sim, chart);
        }
        HudInfo("Saved: %s", utf8ToPath(sim->file).c_str());
    }

    return true;
}

};  // namespace Osu
};  // namespace Vortex