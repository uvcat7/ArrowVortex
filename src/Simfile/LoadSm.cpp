#include <Core/Core.h>

#include <map>
#include <algorithm>
#include <numeric>

#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <System/Debug.h>
#include <System/File.h>

#include <Simfile/Parsing.h>
#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>

#include <Editor/Common.h>

#include <Managers/StyleMan.h>

namespace Vortex {
namespace Sm {

struct ParseData;

#define PARSE_ARGS ParseData &data, const std::string &tag, char *str

typedef void (*ParseFunc)(PARSE_ARGS);

static const int MEASURE_SUBDIV[] = {4, 8, 12, 16, 24, 32, 48, 64, 96, 192};
static const int NUM_MEASURE_SUBDIV = 10;
static const int ROWS_PER_NOTE_SECTION = 192;

struct ParseData {
    bool isSM5;
    int numKeySounds;

    Simfile* sim;
    Chart* chart;
    std::string styleId;

    std::map<std::string, std::string*> simMap1;
    std::map<std::string, ParseFunc> simMap2, chartMap;

    Tempo* tempo();
};

Tempo* ParseData::tempo() {
    if (chart) {
        if (chart->tempo == nullptr) {
            chart->tempo = new Tempo;
            isSM5 = true;
        }
        return chart->tempo;
    }
    return sim->tempo;
}

// ===================================================================================
// Shared data parsing functions.

static void ParseOffset(PARSE_ARGS) { ParseVal(str, data.tempo()->offset); }

static void ParseBpms(PARSE_ARGS) {
    BpmChange bpmc;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], bpmc.row) && ParseVal(v[1], bpmc.bpm)) {
            data.tempo()->segments->append(bpmc);
        }
    }
}

static void ParseStops(PARSE_ARGS) {
    Stop stop;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], stop.row) && ParseVal(v[1], stop.seconds)) {
            data.tempo()->segments->append(stop);
        }
    }
}

static void ParseDelays(PARSE_ARGS) {
    Delay delay;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], delay.row) && ParseVal(v[1], delay.seconds)) {
            data.tempo()->segments->append(delay);
        }
    }
}

static void ParseWarps(PARSE_ARGS) {
    Warp warp;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], warp.row) && ParseBeat(v[1], warp.numRows)) {
            data.tempo()->segments->append(warp);
        }
    }
}

static void ParseSpeeds(PARSE_ARGS) {
    Speed speed;
    for (char* v[4]; ParseNextItem(str, v, 4);) {
        if (ParseBeat(v[0], speed.row) && ParseVal(v[1], speed.ratio) &&
            ParseVal(v[2], speed.delay) && ParseVal(v[3], speed.unit)) {
            data.tempo()->segments->append(speed);
        }
    }
}

static void ParseScrolls(PARSE_ARGS) {
    Scroll scroll;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], scroll.row) && ParseVal(v[1], scroll.ratio)) {
            data.tempo()->segments->append(scroll);
        }
    }
}

static void ParseTickCounts(PARSE_ARGS) {
    TickCount tc;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], tc.row) && ParseVal(v[1], tc.ticks)) {
            data.tempo()->segments->append(tc);
        }
    }
}

static void ParseTimeSignatures(PARSE_ARGS) {
    TimeSignature ts;
    for (char* v[3]; ParseNextItem(str, v, 3);) {
        if (ParseBeat(v[0], ts.row) && ParseVal(v[1], ts.rowsPerMeasure) &&
            ParseVal(v[2], ts.beatNote)) {
            ts.rowsPerMeasure *= ROWS_PER_BEAT;
            data.tempo()->segments->append(ts);
        }
    }
}

static void ParseCombos(PARSE_ARGS) {
    Combo combo;
    for (char* v[3]; ParseNextItem(str, v, 3);) {
        if (ParseBeat(v[0], combo.row) && ParseVal(v[1], combo.hitCombo)) {
            if (!ParseVal(v[2], combo.missCombo))
                combo.missCombo = combo.hitCombo;
            data.tempo()->segments->append(combo);
        }
    }
}

static void ParseFakes(PARSE_ARGS) {
    Fake fake;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], fake.row) && ParseBeat(v[1], fake.numRows)) {
            data.tempo()->segments->append(fake);
        }
    }
}

static void ParseLabels(PARSE_ARGS) {
    Label label;
    for (char* v[2]; ParseNextItem(str, v, 2);) {
        if (ParseBeat(v[0], label.row)) {
            label.str = v[1];
            data.tempo()->segments->append(label);
        }
    }
}

static void ParseAttacks(PARSE_ARGS) {
    double tmp;
    Attack attack;
    for (char* v[2]; ParseNextItem(str, v, 2, ':');) {
        if (Str::equal(v[0], "MODS")) {
            attack.mods = v[1];
            data.tempo()->attacks.push_back(attack);
            attack.mods.clear();
        } else if (ParseVal(v[1], tmp)) {
            if (Str::equal(v[0], "TIME")) {
                attack.time = tmp;
            } else if (Str::equal(v[0], "LEN")) {
                attack.duration = tmp;
                attack.unit = ATTACK_LENGTH;
            } else if (Str::equal(v[0], "END")) {
                attack.duration = tmp;
                attack.unit = ATTACK_END;
            }
        }
    }
}

static void ParseKeySounds(PARSE_ARGS) {
    for (char* v; ParseNextItem(str, v);) {
        data.tempo()->keysounds.push_back(v);
    }
}

// ===================================================================================
// Song property parsing functions.

static void ParseDisplayBpm(PARSE_ARGS) {
    Tempo* tempo = data.tempo();
    if (Str::equal(str, "*")) {
        tempo->displayBpmType = BPM_RANDOM;
        tempo->displayBpmRange = {0, 0};
    } else {
        char* colon = strstr(str, ":");
        double min, max;
        if (colon) {
            *colon = 0;
            if (ParseVal(str, min) && ParseVal(colon + 1, max)) {
                tempo->displayBpmType = BPM_CUSTOM;
                tempo->displayBpmRange = {min, max};
            }
        } else if (ParseVal(str, min)) {
            tempo->displayBpmType = BPM_CUSTOM;
            tempo->displayBpmRange = {min, min};
        }
    }
}

static void ParseBgChanges(PARSE_ARGS) {
    BgChange change;
    for (char* v[11]; ParseNextItem(str, v, 11);) {
        change.rate = 1.0;
        change.startBeat = 0.0;
        change.color2 = v[10];
        change.color = v[9];
        change.transition = v[8];
        change.file2 = v[7];
        change.effect = v[6];
        if (change.effect.empty()) {
            int loop = 0;
            ParseVal(v[5], loop);
            if (loop == 0) change.effect = "StretchNoLoop";
        }
        if (change.effect.empty()) {
            int rewind = 0;
            ParseVal(v[4], rewind);
            if (rewind != 0) change.effect = "StretchRewind";
        }
        if (change.transition.empty()) {
            int crossfade = 0;
            ParseVal(v[3], crossfade);
            if (crossfade != 0) change.transition = "CrossFade";
        }
        ParseVal(v[2], change.rate);
        change.file = v[1];
        ParseVal(v[0], change.startBeat);

        if (tag == "FGCHANGES") {
            data.sim->fgChanges.push_back(change);
        } else if (tag == "BGCHANGES2") {
            data.sim->bgChanges[1].push_back(change);
        } else {
            data.sim->bgChanges[0].push_back(change);
        }
    }
}

// ===================================================================================
// Note parsing functions.

static void ParseRadar(Chart* chart, char* str) {
    double val;
    for (char* v; ParseNextItem(str, v);) {
        if (ParseVal(v, val)) {
            chart->radar.push_back(val);
        }
    }
}

Difficulty ToDiff(const char* str) {
    if (Str::iequal(str, "beginner")) {
        return DIFF_BEGINNER;
    } else if (Str::iequal(str, "easy")) {
        return DIFF_EASY;
    } else if (Str::iequal(str, "medium")) {
        return DIFF_MEDIUM;
    } else if (Str::iequal(str, "hard")) {
        return DIFF_HARD;
    } else if (Str::iequal(str, "challenge")) {
        return DIFF_CHALLENGE;
    }
    return DIFF_EDIT;
}

struct ReadNoteData {
    int player, numCols;
    NoteList* notes;
    Vector<int> holdPos;
    Vector<NoteType> holdType;
    Vector<int> quants;
};

static void ReadNoteRow(ReadNoteData& data, int row, char* p,
                        int quantization) {
    for (int col = 0; col < data.numCols; ++col, ++p) {
        if (*p == '0') {
            // Do nothing.
        } else if (*p == '1') {
            data.notes->append({row, row, (uint32_t)col, (uint32_t)data.player,
                                NOTE_STEP_OR_HOLD, (uint32_t)quantization});
        } else if (*p == '2' || *p == '4') {
            data.notes->append({row, row, (uint32_t)col, (uint32_t)data.player,
                                NOTE_STEP_OR_HOLD, (uint32_t)quantization});
            data.holdType[col] = (*p == '2') ? NOTE_STEP_OR_HOLD : NOTE_ROLL;
            data.holdPos[col] = data.notes->size();
            data.quants[col] = quantization;
        } else if (*p == '3') {
            int holdPos = data.holdPos[col];
            if (holdPos) {
                auto* hold = data.notes->begin() + holdPos - 1;
                hold->endrow = row;
                hold->type = data.holdType[col];
                // Make sure we set the note to its largest quantization to
                // avoid data loss
                if (data.quants[col] > 0 && hold->quant > 0) {
                    hold->quant = min(192u, quantization * hold->quant /
                                                gcd(quantization, hold->quant));
                } else  // There was some error, so always play safe and use 192
                {
                    HudError("Bug: couldn't get hold quantization in row %d",
                             row);
                    hold->quant = 192;
                }
                data.holdPos[col] = 0;
                data.quants[col] = 0;
            }
        } else if (*p == 'M') {
            data.notes->append({row, row, (uint32_t)col, (uint32_t)data.player,
                                NOTE_MINE, (uint32_t)quantization});
        } else if (*p == 'L') {
            data.notes->append({row, row, (uint32_t)col, (uint32_t)data.player,
                                NOTE_LIFT, (uint32_t)quantization});
        } else if (*p == 'F') {
            data.notes->append({row, row, (uint32_t)col, (uint32_t)data.player,
                                NOTE_FAKE, (uint32_t)quantization});
        }
    }
}

static void ParseNotes(ParseData& data, Chart* chart, const std::string& style,
                       char* notes) {
    char* p = notes;

    // Derive the column count from the first note row.
    int numPlayers = 1;
    int numCols = 0;
    while (*p == ' ' || *p == '\n') ++p;
    for (; *p && *p != '\n'; ++p) {
        if (*p == '[') {
            while (*p && *p != ']') ++p;
        } else {
            ++numCols;
        }
    }

    // Create an empty note line for quick comparison.
    std::string emptylinestr(numCols, '0');
    const char* emptyline = emptylinestr.c_str();

    // Read the note data.
    ReadNoteData readNoteData;
    readNoteData.player = 0;
    readNoteData.numCols = numCols;
    readNoteData.notes = &chart->notes;
    readNoteData.holdPos.resize(numCols, 0);
    readNoteData.quants.resize(numCols, 0);
    readNoteData.holdType.resize(numCols, NOTE_STEP_OR_HOLD);

    int numSections = 0;
    for (p = notes; *p;) {
        readNoteData.player = numPlayers - 1;
        int section = numSections;
        char* measureText = p;

        // Read the current section up until the comma (next section) or
        // ampersand (next player).
        while (*p && *p != ',' && *p != '&') ++p;
        if (*p == ',') {
            ++numSections;
            *p++ = 0;
        } else if (*p == '&') {
            ++numPlayers;
            numSections = 0;
            *p++ = 0;
        }

        // Remove whitespace from the section.
        int numSymbols = 0;
        for (char *read = measureText, *write = measureText; *read; ++read) {
            if (*read == '[') {
                ++data.numKeySounds;
                while (*read && *read != ']') ++read;
            } else if (*read != ' ' && *read != '\n') {
                *write = *read;
                ++numSymbols, ++write;
            }
        }

        // Read notes in the current section.
        Vector<int> holds(numCols, 0);
        int numLines = numSymbols / numCols;
        int quantization = numLines;
        if (numLines > 0) {
            int startRow = section * ROWS_PER_NOTE_SECTION;
            char* line = measureText;
            int row = startRow;

            // Try to find custom quantizations in any 192nd snap measures
            if (numLines == ROWS_PER_NOTE_SECTION) {
                // Nothing better to than to check them all by hand
                for (int i = 4; i < ROWS_PER_NOTE_SECTION; i++) {
                    bool valid = true;
                    line = measureText;
                    float mod = (float)ROWS_PER_NOTE_SECTION / i;
                    for (int j = 0; valid && j < ROWS_PER_NOTE_SECTION;
                         ++j, line += numCols) {
                        float rem = round(fmod(j, mod));
                        // Check all the compressed rows and make sure they are
                        // empty
                        if (rem > 0 && rem < static_cast<int>(mod) &&
                            memcmp(line, emptyline, numCols) != 0) {
                            valid = false;
                            break;
                        }
                    }
                    // The first (smallest) match is always the best
                    if (valid) {
                        quantization = i;
                        break;
                    }
                }
            }

            line = measureText;
            int ofs = ROWS_PER_NOTE_SECTION / numLines;
            for (int i = 0; i < numLines; ++i) {
                if (memcmp(line, emptyline, numCols) != 0) {
                    ReadNoteRow(readNoteData, row, line, quantization);
                }

                // Handle abnormal numbers of lines loading
                if (ROWS_PER_NOTE_SECTION % numLines != 0) {
                    ofs = ((int)round(192.0f / numLines * (i + 1)) -
                           (int)round(192.0f / numLines * i));
                }
                line += numCols;
                row += ofs;
            }
        }
    }

    // Make sure the notes are sorted.
    if (!std::is_sorted(chart->notes.begin(), chart->notes.end(),
                        LessThanRowCol<Note, Note>)) {
        std::sort(chart->notes.begin(), chart->notes.end(),
                  LessThanRowCol<Note, Note>);
    }

    // Find a style for the chart based on the style id, columns, and players.
    chart->style =
        gStyle->findStyle(chart->description(), numCols, numPlayers, style);
}

static void ParseNotes(PARSE_ARGS) {
    char* p = str;

    // Split chart info "a:b:c:d:..." into parameters.
    int numParams = 0;
    char* params[6] = {};
    for (; numParams < 6 && *p; ++numParams) {
        while (*p == ' ' || *p == '\n') ++p;
        params[numParams] = p;
        while (*p && *p != ':') ++p;
        if (*p == ':') *p++ = 0;
    }
    for (int i = numParams; i < 6; ++i) {
        params[i] = p;
    }

    // Make sure there is a chart.
    if (!data.chart) data.chart = new Chart;

    // Read the chart parameters.
    char* notes = nullptr;
    if (numParams == 5 || numParams == 6) {
        // Stepmania 3.95/ITG notes format.
        data.styleId = params[0];
        data.chart->artist = params[1];
        data.chart->difficulty = ToDiff(params[2]);
        data.chart->meter = atoi(params[3]);
        ParseRadar(data.chart, params[4]);
        notes = params[5];
    } else if (numParams == 0 || numParams == 1) {
        // Stepmania 5 notes format.
        notes = params[0];
    }
    ParseNotes(data, data.chart, data.styleId, notes);

    // Add the chart to the chart list.
    data.sim->charts.push_back(data.chart);
    data.chart = nullptr;
    data.styleId.clear();
}

// ================================================================================================
// Tag parsing.

typedef std::map<std::string, std::string*> StrMap;
typedef std::map<std::string, ParseFunc> FuncMap;

std::string UnescapeTag(std::string s) {
    Str::replace(s, "\\\\", "\\");
    Str::replace(s, "\\;", ";");
    Str::replace(s, "\\:", ":");
    Str::replace(s, "\\#", "#");
    return s;
}

static void MapSharedTags(FuncMap& map) {
    map["OFFSET"] = ParseOffset;

    map["BPMS"] = ParseBpms;
    map["STOPS"] = ParseStops;
    map["DELAYS"] = ParseDelays;
    map["WARPS"] = ParseWarps;
    map["SPEEDS"] = ParseSpeeds;
    map["SCROLLS"] = ParseScrolls;
    map["TICKCOUNTS"] = ParseTickCounts;
    map["TIMESIGNATURES"] = ParseTimeSignatures;

    map["LABELS"] = ParseLabels;
    map["ATTACKS"] = ParseAttacks;
    map["KEYSOUNDS"] = ParseKeySounds;
    map["COMBOS"] = ParseCombos;
    map["FAKES"] = ParseFakes;
    map["DISPLAYBPM"] = ParseDisplayBpm;

    map["NOTES"] = ParseNotes;
    map["NOTES2"] = ParseNotes;
}

static void MapSimfileTags(StrMap& str, FuncMap& map, Simfile* sim) {
    MapSharedTags(map);

    str["TITLE"] = &sim->title;
    str["TITLETRANSLIT"] = &sim->titleTr;
    str["SUBTITLE"] = &sim->subtitle;
    str["SUBTITLETRANSLIT"] = &sim->subtitleTr;
    str["ARTIST"] = &sim->artist;
    str["ARTISTTRANSLIT"] = &sim->artistTr;
    str["GENRE"] = &sim->genre;
    str["CREDIT"] = &sim->credit;
    str["MUSIC"] = &sim->music;
    str["BANNER"] = &sim->banner;
    str["BACKGROUND"] = &sim->background;
    str["CDTITLE"] = &sim->cdTitle;
    str["LYRICSPATH"] = &sim->lyricsPath;

    map["FGCHANGES"] = ParseBgChanges;
    map["BGCHANGES"] = ParseBgChanges;
    map["BGCHANGES1"] = ParseBgChanges;
    map["BGCHANGES2"] = ParseBgChanges;

    map["VERSION"] = [](PARSE_ARGS) { data.isSM5 = true; };
    map["SAMPLESTART"] = [](PARSE_ARGS) {
        ParseVal(str, data.sim->previewStart);
    };
    map["SAMPLELENGTH"] = [](PARSE_ARGS) {
        ParseVal(str, data.sim->previewLength);
    };
    map["SELECTABLE"] = [](PARSE_ARGS) {
        ParseBool(str, data.sim->isSelectable);
    };

    map["NOTEDATA"] = [](PARSE_ARGS) { data.chart = new Chart; };
}

static void MapChartTags(FuncMap& map) {
    MapSharedTags(map);

    map["DESCRIPTION"] = [](PARSE_ARGS) {
        data.chart->artist = UnescapeTag(str);
    };
    map["DIFFICULTY"] = [](PARSE_ARGS) {
        data.chart->difficulty = ToDiff(str);
    };
    map["METER"] = [](PARSE_ARGS) { ParseVal(str, data.chart->meter); };
    map["RADARVALUES"] = [](PARSE_ARGS) { ParseRadar(data.chart, str); };
    map["STEPSTYPE"] = [](PARSE_ARGS) { data.styleId = str; };
}

static void ParseTag(ParseData& data, std::string tag, char* val) {
    if (data.chart) {
        // Check if the tag is in the chart function map.
        auto func = data.chartMap.find(tag);
        if (func != data.chartMap.end()) {
            func->second(data, tag, val);
            return;
        }
    } else {
        // Check if the tag is in the sim string map.
        auto str = data.simMap1.find(tag);
        if (str != data.simMap1.end()) {
            *str->second = UnescapeTag(val);
            return;
        }

        // Check if the tag is in the sim function map.
        auto func = data.simMap2.find(tag);
        if (func != data.simMap2.end()) {
            func->second(data, tag, val);
            return;
        }
    }

    // Add the tag to the misc properties.
    data.tempo()->misc.push_back({tag, val});
}

// ===================================================================================
// File importing

bool LoadSm(const std::string& path, Simfile* sim) {
    ParseData data;

    data.sim = sim;
    data.chart = nullptr;
    data.isSM5 = Str::endsWith(path, ".ssc", false);
    data.numKeySounds = 0;

    MapSimfileTags(data.simMap1, data.simMap2, sim);
    MapChartTags(data.chartMap);

    // Read the file.
    std::string str;
    if (!ParseSimfile(str, path)) return false;

    // Read the tags.
    char *tag, *val, *p = &str[0];
    while (ParseNextTag(p, tag, val)) {
        ParseTag(data, tag, val);
    }

    // Show a warning if keysounds were present.
    if (data.numKeySounds > 0) {
        HudWarning(
            "Simfile has %i keysound(s), which are not yet supported and were "
            "ignored.",
            data.numKeySounds);
    }

    // If there is a pending chart, release it.
    if (data.chart) delete data.chart;

    // Derive the load format from the loaded properties.
    sim->format = (data.isSM5 ? SIM_SSC : SIM_SM);

    return true;
}

};  // namespace Sm
};  // namespace Vortex