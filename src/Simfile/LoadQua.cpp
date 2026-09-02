#include <Core/Core.h>

#include <map>
#include <vector>
#include <algorithm>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

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
// Quaver .qua files.
//
// The format is YAML, but Quaver only ever writes a small, regular subset of
// it: scalars at the top level, and two lists of maps (TimingPoints and
// HitObjects) whose first key is prefixed with "- ". That is all this reader
// understands, which is enough for every file the game itself produces.
//
//   Mode: Keys4
//   Title: ...
//   TimingPoints:
//   - StartTime: 1000
//     Bpm: 158
//   HitObjects:
//   - StartTime: 1234
//     Lane: 1
//   - StartTime: 2345
//     Lane: 3
//     EndTime: 3456
//
// Times are milliseconds and lanes are 1-based, unlike osu!, where the lane is
// derived from an x coordinate.

struct TimingPoint {
    double bpm;
};

struct QuaFile {
    struct HitObject {
        int lane;              // 0-based once parsed
        double time, endtime;  // seconds
    };

    int numCols;
    double previewTime = -1.0;
    std::string difficultyName;
    std::string musicPath;
    std::string background;
    std::string banner;
    std::string songArtist;
    std::string songTitle;
    std::string stepArtist;
    std::string filename;

    std::map<double, TimingPoint> timingPoints;
    std::vector<HitObject> hitObjects;
};

// ================================================================================================
// Parsing utility functions.

// Strips leading and trailing spaces, and the quotes Quaver puts around empty
// or special strings.
static std::string CleanValue(const std::string& in) {
    size_t b = 0, e = in.size();
    while (b < e && (in[b] == ' ' || in[b] == '\t' || in[b] == '\r')) ++b;
    while (e > b &&
           (in[e - 1] == ' ' || in[e - 1] == '\t' || in[e - 1] == '\r'))
        --e;
    std::string out(in.begin() + b, in.begin() + e);
    if (out.size() >= 2 && ((out.front() == '\'' && out.back() == '\'') ||
                            (out.front() == '"' && out.back() == '"'))) {
        out = std::string(out.begin() + 1, out.end() - 1);
    }
    return out;
}

// Splits "  - StartTime: 1234" into indent, "StartTime", "1234", and reports
// whether the line opens a new list item.
struct Line {
    int indent;
    bool isItem;
    std::string key, value;
    bool valid;
};

static Line SplitLine(const std::string& raw) {
    Line out;
    out.indent = 0;
    out.isItem = false;
    out.valid = false;

    size_t i = 0;
    while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t')) {
        out.indent += (raw[i] == '\t') ? 4 : 1;
        ++i;
    }

    // A comment or a blank line carries nothing.
    if (i >= raw.size() || raw[i] == '#' || raw[i] == '\r') return out;

    if (raw[i] == '-' && i + 1 < raw.size() && raw[i + 1] == ' ') {
        out.isItem = true;
        i += 2;
        while (i < raw.size() && raw[i] == ' ') ++i;
    }

    size_t colon = raw.find(':', i);
    if (colon == std::string::npos) return out;

    out.key = CleanValue(raw.substr(i, colon - i));
    out.value = CleanValue(raw.substr(colon + 1));
    out.valid = !out.key.empty();
    return out;
}

static double ReadDouble(const std::string& s) {
    return s.empty() ? 0.0 : atof(s.c_str());
}

static int ReadInt(const std::string& s) {
    return s.empty() ? 0 : atoi(s.c_str());
}

// "Keys4" / "Keys7" -> 4 / 7. Quaver has no other modes today, but reading the
// digits rather than matching whole strings keeps this working if it gains one.
static int ModeToColumns(const std::string& mode) {
    int cols = 0;
    for (char c : mode) {
        if (c >= '0' && c <= '9') cols = cols * 10 + (c - '0');
    }
    return (cols >= 1 && cols <= 18) ? cols : 4;
}

// ================================================================================================
// Main parser.

enum Section { SEC_NONE, SEC_TIMING, SEC_OBJECTS, SEC_OTHER };

static void FlushTimingPoint(QuaFile& out, double startTime, double bpm,
                             bool haveAny) {
    if (!haveAny) return;
    if (bpm <= 0.1 || bpm >= 10000.0) return;
    out.timingPoints[startTime * 0.001] = {bpm};
}

static void FlushHitObject(QuaFile& out, double startTime, double endTime,
                           int lane, bool haveAny) {
    if (!haveAny || lane < 1) return;
    double t = startTime * 0.001;
    double e = (endTime > startTime) ? endTime * 0.001 : t;
    out.hitObjects.push_back({lane - 1, t, e});
}

static bool ParseFile(QuaFile& out, const std::string& text) {
    Section section = SEC_NONE;

    // Pending list item.
    bool haveItem = false;
    double itemStart = 0.0, itemEnd = 0.0, itemBpm = 0.0;
    int itemLane = 0;

    size_t pos = 0;
    while (pos <= text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();
        std::string raw = text.substr(pos, eol - pos);
        pos = eol + 1;

        Line line = SplitLine(raw);
        if (!line.valid) {
            if (eol >= text.size()) break;
            continue;
        }

        // A key at column zero always closes whatever list was open.
        if (line.indent == 0 && !line.isItem) {
            if (section == SEC_TIMING) {
                FlushTimingPoint(out, itemStart, itemBpm, haveItem);
            } else if (section == SEC_OBJECTS) {
                FlushHitObject(out, itemStart, itemEnd, itemLane, haveItem);
            }
            haveItem = false;

            const std::string& k = line.key;
            if (k == "TimingPoints") {
                section = SEC_TIMING;
            } else if (k == "HitObjects") {
                section = SEC_OBJECTS;
            } else if (k == "SliderVelocities" || k == "EditorLayers" ||
                       k == "CustomAudioSamples" || k == "SoundEffects" ||
                       k == "Bookmarks") {
                section = SEC_OTHER;  // lists we do not read
            } else {
                section = SEC_NONE;
                if (k == "AudioFile") {
                    out.musicPath = line.value;
                } else if (k == "SongPreviewTime") {
                    out.previewTime = ReadDouble(line.value);
                } else if (k == "BackgroundFile") {
                    out.background = line.value;
                } else if (k == "BannerFile") {
                    out.banner = line.value;
                } else if (k == "Title") {
                    out.songTitle = line.value;
                } else if (k == "Artist") {
                    out.songArtist = line.value;
                } else if (k == "Creator") {
                    out.stepArtist = line.value;
                } else if (k == "DifficultyName") {
                    out.difficultyName = line.value;
                } else if (k == "Mode") {
                    out.numCols = ModeToColumns(line.value);
                }
            }
            continue;
        }

        // Inside a list: "- Key: value" starts a new item, deeper lines add to
        // the current one.
        if (section == SEC_TIMING || section == SEC_OBJECTS) {
            if (line.isItem) {
                if (section == SEC_TIMING) {
                    FlushTimingPoint(out, itemStart, itemBpm, haveItem);
                } else {
                    FlushHitObject(out, itemStart, itemEnd, itemLane, haveItem);
                }
                haveItem = true;
                itemStart = itemEnd = itemBpm = 0.0;
                itemLane = 0;
            }
            if (!haveItem) continue;

            const std::string& k = line.key;
            if (k == "StartTime") {
                itemStart = ReadDouble(line.value);
            } else if (k == "EndTime") {
                itemEnd = ReadDouble(line.value);
            } else if (k == "Bpm") {
                itemBpm = ReadDouble(line.value);
            } else if (k == "Lane") {
                itemLane = ReadInt(line.value);
            }
        }

        if (eol >= text.size()) break;
    }

    // Whatever was still open when the file ended.
    if (section == SEC_TIMING) {
        FlushTimingPoint(out, itemStart, itemBpm, haveItem);
    } else if (section == SEC_OBJECTS) {
        FlushHitObject(out, itemStart, itemEnd, itemLane, haveItem);
    }

    return true;
}

// ================================================================================================
// Timing point conversion. Mirrors the osu importer: the first point defines
// the offset, the rest become BPM changes on the row they fall on.

static void ConvertTimingPoints(Simfile* sim, QuaFile& qua) {
    auto tempo = sim->tempo;
    if (qua.timingPoints.empty()) return;

    auto it = qua.timingPoints.begin();
    auto end = qua.timingPoints.end();

    BpmChange initialBpm;
    initialBpm.bpm = it->second.bpm;
    tempo->segments->append(initialBpm);

    double spb = 60.0 / it->second.bpm;
    double lastBpm = it->second.bpm;

    // The first timing point lands on row zero.
    tempo->offset = -it->first;

    double prevTime = it->first, prevRow = 0.0;
    for (++it; it != end; ++it) {
        double deltaTime = it->first - prevTime;
        double curRow = prevRow + 48.0 * deltaTime / spb;

        int row = static_cast<int>(round(curRow));
        if (lastBpm != it->second.bpm) {
            tempo->segments->append(BpmChange(row, it->second.bpm));
            lastBpm = it->second.bpm;
        }

        spb = 60.0 / it->second.bpm;
        prevTime = it->first;
        prevRow = curRow;
    }
}

// ================================================================================================
// Note conversion.

static bool LessThan(const QuaFile::HitObject& a, const QuaFile::HitObject& b) {
    if (a.time != b.time) return a.time < b.time;
    return a.lane < b.lane;
}

static void ConvertNotes(Simfile* sim, QuaFile& qua, Chart& chart) {
    if (!std::is_sorted(qua.hitObjects.begin(), qua.hitObjects.end(),
                        LessThan)) {
        std::sort(qua.hitObjects.begin(), qua.hitObjects.end(), LessThan);
    }

    TimingData timing;
    timing.update(sim->tempo);
    TempoRowTracker tracker(timing);
    for (auto& obj : qua.hitObjects) {
        int col = std::clamp(obj.lane, 0, qua.numCols - 1);

        int row = tracker.advance(obj.time);
        if (obj.endtime > obj.time) {
            int endrow = timing.timeToRow(obj.endtime);
            chart.notes.append({row, endrow, static_cast<uint32_t>(col), 0,
                                NOTE_STEP_OR_HOLD, 192});
        } else {
            chart.notes.append({row, row, static_cast<uint32_t>(col), 0,
                                NOTE_STEP_OR_HOLD, 192});
        }
    }
}

static void DestroyFiles(std::vector<QuaFile*>& files) {
    for (auto file : files) delete file;
}

static void ParseDir(std::vector<QuaFile*>& out, fs::path dir) {
    for (auto& file : File::findFiles(dir, false, ".qua")) {
        bool success;
        std::string str = File::getText(file, &success);
        if (str.empty() || !success) continue;

        out.push_back(new QuaFile);
        out.back()->numCols = 4;
        ParseFile(*out.back(), str);
        out.back()->filename = pathToUtf8(file.filename());
    }
}

// ================================================================================================
// Difficulty selection.

// Quaver difficulty names are free-form - "Insane", "4K Another", the name of
// the chart's author - so there is nothing to match them against. The charts
// are sorted by how many notes they hold and handed the difficulty slots in
// that order, which is the fallback the osu! reader uses when it recognises
// none of the names it is given.
static void AssignDifficulties(Simfile* sim) {
    auto& charts = sim->charts;
    if (charts.empty()) return;

    std::sort(charts.begin(), charts.end(), [](const Chart* a, const Chart* b) {
        return a->notes.size() < b->notes.size();
    });

    for (int i = 0; i < static_cast<int>(charts.size()); ++i) {
        charts[i]->difficulty = static_cast<Difficulty>(
            std::min(i, static_cast<int>(NUM_DIFFICULTIES) - 1));
    }
}

};  // anonymous namespace

bool LoadQua(fs::path path, Simfile* sim) {
    // Every .qua in the folder is one difficulty of the same song, the way
    // osu! spreads its difficulties over several .osu files.
    std::vector<QuaFile*> files;
    ParseDir(files, utf8ToPath(sim->dir));
    if (files.empty()) return false;

    // Prefer the file that was actually opened as the source of the metadata.
    QuaFile* mainFile = nullptr;
    for (auto file : files) {
        if (file->filename == sim->file) mainFile = file;
    }
    if (!mainFile) mainFile = files[0];

    sim->title = mainFile->songTitle;
    sim->artist = mainFile->songArtist;
    sim->music = mainFile->musicPath;
    sim->background = mainFile->background;
    sim->banner =
        mainFile->banner.empty() ? mainFile->background : mainFile->banner;
    sim->credit = mainFile->stepArtist;

    // Quaver stores where the preview starts but not how long it runs, so
    // only the start carries over; the length is filled in once the music is
    // loaded and its duration is known.
    if (mainFile->previewTime > 0.0) {
        sim->previewStart = mainFile->previewTime * 0.001;
        sim->previewLength = 0.0;
    }

    ConvertTimingPoints(sim, *mainFile);

    for (auto file : files) {
        if (file->hitObjects.empty()) continue;

        Chart* chart = new Chart;
        chart->style =
            gStyle->findStyle(file->difficultyName, file->numCols, 1);
        chart->artist = file->stepArtist;
        chart->meter = 1;
        ConvertNotes(sim, *file, *chart);

        sim->charts.push_back(chart);
    }

    AssignDifficulties(sim);

    // Strip the difficulty tag from the filename, so that saving does not
    // keep appending a new one on every round trip.
    sim->file = mainFile->filename;
    size_t begin = Str::findLast(sim->file, '[');
    size_t end = (begin == std::string::npos)
                     ? std::string::npos
                     : Str::find(sim->file, ']', begin);
    if (end != std::string::npos) {
        Str::erase(sim->file, static_cast<int>(begin),
                   static_cast<int>(end + 1 - begin));
        // The tag sits before the extension, so the space it leaves behind
        // is in the middle of the name rather than at the end.
        size_t dot = Str::findLast(sim->file, '.');
        if (dot != std::string::npos && dot > 0 && sim->file[dot - 1] == ' ') {
            Str::erase(sim->file, static_cast<int>(dot) - 1, 1);
        } else if (sim->file.length() && sim->file.back() == ' ') {
            Str::pop_back(sim->file);
        }
    }

    sim->format = SIM_QUA;

    DestroyFiles(files);
    return !sim->charts.empty();
}

};  // namespace Qua
};  // namespace Vortex
