#include <Core/Core.h>

#include <map>
#include <vector>
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
namespace Qua {
namespace {

// ================================================================================================
// Exporting utilities.
//
// Quaver reads .qua files with a strict YAML parser, so the output has to be
// laid out the way the game writes it: two spaces of indent inside a list item,
// and quotes around any scalar that could be read as something other than a
// plain string.

static int ToMilliseconds(double time) {
    // Adding a half and truncating rounds towards zero on the negative side,
    // which is a millisecond out for anything before the start of the audio.
    return static_cast<int>(llround(time * 1000.0));
}

static void WriteString(std::ofstream& out, const char* name,
                        const std::string& val) {
    bool quote = val.empty();
    for (char c : val) {
        if (c == ':' || c == '#' || c == '\'' || c == '"' || c == '{' ||
            c == '[' || c == '&' || c == '*' || c == '%' || c == '@') {
            quote = true;
            break;
        }
    }
    if (!quote &&
        (val.front() == ' ' || val.front() == '-' || val.back() == ' ')) {
        quote = true;
    }

    out << name << ": ";
    if (quote) {
        out << '\'';
        for (char c : val) {
            if (c == '\'') out << '\'';  // YAML escapes a quote by doubling it
            out << c;
        }
        out << '\'';
    } else {
        out << val;
    }
    out << "\n";
}

static void WriteString(std::ofstream& out, const char* name, const char* val) {
    WriteString(out, name, std::string(val));
}

// ================================================================================================
// Timing exporting.
//
// Same approach as the osu exporter: Quaver has no stops either, so a stop is
// approximated by a temporary slow BPM that is reverted afterwards.

struct ExportTP {
    double time;
    double spr;
};

static int FindNextNoteRow(const NoteList& list, int row) {
    const Note* it =
        std::lower_bound(list.begin(), list.end(), row,
                         [](const Note& a, int row) { return a.row < row; });
    while (it != list.end() && it->type != NOTE_STEP_OR_HOLD) ++it;
    return (it != list.end()) ? it->row : INT_MAX;
}

static void ConvertStop(std::vector<ExportTP>& tps,
                        const TimingData::Event* cur,
                        const TimingData::Event* next, const Chart* chart) {
    int endRow = cur->row + 48;

    int nextNote = FindNextNoteRow(chart->notes, cur->row + 1);
    if (nextNote < endRow) endRow = nextNote;
    if (next && next->row < endRow) endRow = next->row;

    double timeDiff =
        (cur->endTime - cur->time) + (endRow - cur->row) * cur->spr;
    double rowDiff = endRow - cur->row;
    tps.push_back({cur->time, timeDiff / rowDiff});

    tps.push_back({cur->endTime, cur->spr});
}

static void WriteTimingPoints(std::ofstream& out, const Chart* chart,
                              const TimingData& timing) {
    std::vector<ExportTP> tps;
    tps.reserve(32);

    auto end = timing.events.end();
    for (auto it = timing.events.begin(); it != end; ++it) {
        if (it->endTime > it->time) {
            auto next = it + 1;
            ConvertStop(tps, &*it, (next == end) ? nullptr : &*next, chart);
        } else {
            tps.push_back({it->time, it->spr});
        }
    }

    if (tps.empty()) {
        out << "TimingPoints: []\n";
        return;
    }

    out << "TimingPoints:\n";
    for (auto& tp : tps) {
        double secPerBeat = tp.spr * 48.0;
        if (secPerBeat <= 0.0) continue;

        // A timing point that falls before the start of the song is moved
        // forward by whole measures, which keeps the beat alignment intact.
        double time = tp.time;
        if (time < 0) {
            double secPerMeasure = secPerBeat * 4.0;
            time = secPerMeasure - fmod(-time, secPerMeasure);
        }

        out << "- StartTime: " << ToMilliseconds(time) << "\n";
        out << "  Bpm: " << (60.0 / secPerBeat) << "\n";
    }
}

// ================================================================================================
// Notes exporting.

static void WriteNotes(std::ofstream& out, const Chart* chart,
                       const TimingData& timing) {
    // Quaver has no mines, so those are the only notes that get dropped.
    bool any = false;
    for (auto& note : chart->notes) {
        if (note.type != NOTE_MINE) {
            any = true;
            break;
        }
    }
    if (!any) {
        out << "HitObjects: []\n";
        return;
    }

    out << "HitObjects:\n";

    TempoTimeTracker tracker(timing);
    for (auto& note : chart->notes) {
        if (note.type == NOTE_MINE) continue;

        double time = tracker.advance(note.row);
        double endtime = time;
        if (note.endrow > note.row) endtime = timing.rowToTime(note.endrow);

        // Lanes are 1-based in Quaver.
        out << "- StartTime: " << ToMilliseconds(time) << "\n";
        out << "  Lane: " << (note.col + 1) << "\n";
        if (endtime > time) {
            out << "  EndTime: " << ToMilliseconds(endtime) << "\n";
        }
        out << "  KeySounds: []\n";
    }
}

// ================================================================================================
// Chart exporting.

static void SaveChart(fs::path path, const Simfile* sim, const Chart* chart) {
    std::ofstream out(path.c_str());
    if (out.fail()) return;

    int numCols = chart ? chart->style->numCols : 4;

    WriteString(out, "AudioFile", sim->music);
    // Quaver counts the preview in milliseconds too.
    out << "SongPreviewTime: "
        << (sim->previewStart > 0.0 ? ToMilliseconds(sim->previewStart) : 0)
        << "\n";
    WriteString(out, "BackgroundFile", sim->background);
    WriteString(out, "BannerFile", sim->banner);
    out << "MapId: -1\n";
    out << "MapSetId: -1\n";
    out << "Mode: Keys" << numCols << "\n";
    WriteString(out, "Title", sim->title);
    WriteString(out, "Artist", sim->artist);
    WriteString(out, "Source", "");
    WriteString(out, "Tags", "");
    WriteString(out, "Creator", chart ? chart->artist : std::string());
    WriteString(out, "DifficultyName",
                chart ? GetDifficultyName(chart->difficulty) : "Normal");
    WriteString(out, "Description", "Created with ArrowVortex");
    out << "BPMDoesNotAffectScrollVelocity: true\n";
    out << "InitialScrollVelocity: 1\n";
    out << "EditorLayers: []\n";
    out << "CustomAudioSamples: []\n";
    out << "SoundEffects: []\n";

    if (chart) {
        auto tempo = chart->getTempo(sim);
        TimingData timing;
        timing.update(tempo);

        WriteTimingPoints(out, chart, timing);
        out << "SliderVelocities: []\n";
        WriteNotes(out, chart, timing);
    } else {
        out << "TimingPoints: []\n";
        out << "SliderVelocities: []\n";
        out << "HitObjects: []\n";
    }

    out.close();
}

};  // anonymous namespace

bool SaveQua(const Simfile* sim, bool backup) {
    // Quaver, like osu!, keeps one difficulty per file.
    fs::path path = utf8ToPath(sim->dir);
    path.append(stringToUtf8(sim->file));
    path.replace_extension(".qua");

    if (sim->charts.empty()) {
        SaveChart(path, sim, nullptr);
    } else {
        std::map<std::string, int> duplicateCounters;
        for (auto chart : sim->charts) {
            auto diffName = std::string(GetDifficultyName(chart->difficulty));
            fs::path chartPath = utf8ToPath(sim->dir);
            chartPath.append(stringToUtf8(sim->file));
            chartPath.replace_extension();
            chartPath.concat(" [");
            chartPath.concat(diffName);
            int& counter = duplicateCounters[diffName];
            if (++counter > 1) {
                chartPath.concat(Str::fmt(" %1").arg(counter).str);
            }
            chartPath.concat("].qua");
            SaveChart(chartPath, sim, chart);
        }
    }
    HudInfo("Saved: %s", pathToUtf8(path.filename()).c_str());
    return true;
}

};  // namespace Qua
};  // namespace Vortex
