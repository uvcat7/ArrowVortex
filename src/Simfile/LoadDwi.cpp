#include <Core/Core.h>
#include <Core/StringUtils.h>

#include <algorithm>

#include <System/File.h>

#include <Simfile/Parsing.h>
#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>

#include <Managers/StyleMan.h>

#include <Editor/Common.h>

namespace Vortex {
namespace Dwi {

// ===================================================================================
// Note parsing.

enum DwiNote {
    NO_NOTE = 0,

    NOTE_L,
    NOTE_UL,
    NOTE_D,
    NOTE_U,
    NOTE_UR,
    NOTE_R,

    NOTE_COUNT
};

static void SetColumnMap(int* map, int cols, int pad) {
    int ofs = pad * 4;
    for (int i = 0; i < NOTE_COUNT; ++i) map[i] = -1;
    switch (cols) {
        case 4:
            map[NOTE_L] = 0;
            map[NOTE_D] = 1;
            map[NOTE_U] = 2;
            map[NOTE_R] = 3;
            break;
        case 6:
            map[NOTE_L] = 0;
            map[NOTE_UL] = 1;
            map[NOTE_D] = 2;
            map[NOTE_U] = 3;
            map[NOTE_UR] = 4;
            map[NOTE_R] = 5;
            break;
        case 8:
            map[NOTE_L] = ofs + 0;
            map[NOTE_D] = ofs + 1;
            map[NOTE_U] = ofs + 2;
            map[NOTE_R] = ofs + 3;
            break;
    };
}

static void CharToColumnPair(char c, const int* map, int& first, int& second) {
    DwiNote a = NO_NOTE, b = NO_NOTE;
    switch (c) {
        case '0':
            a = NO_NOTE;
            b = NO_NOTE;
            break;
        case '1':
            a = NOTE_D;
            b = NOTE_L;
            break;
        case '2':
            a = NOTE_D;
            b = NO_NOTE;
            break;
        case '3':
            a = NOTE_D;
            b = NOTE_R;
            break;
        case '4':
            a = NOTE_L;
            b = NO_NOTE;
            break;
        case '5':
            a = NO_NOTE;
            b = NO_NOTE;
            break;
        case '6':
            a = NOTE_R;
            b = NO_NOTE;
            break;
        case '7':
            a = NOTE_U;
            b = NOTE_L;
            break;
        case '8':
            a = NOTE_U;
            b = NO_NOTE;
            break;
        case '9':
            a = NOTE_U;
            b = NOTE_R;
            break;
        case 'A':
            a = NOTE_U;
            b = NOTE_D;
            break;
        case 'B':
            a = NOTE_L;
            b = NOTE_R;
            break;
        case 'C':
            a = NOTE_UL;
            b = NO_NOTE;
            break;
        case 'D':
            a = NOTE_UR;
            b = NO_NOTE;
            break;
        case 'E':
            a = NOTE_L;
            b = NOTE_UL;
            break;
        case 'F':
            a = NOTE_UL;
            b = NOTE_D;
            break;
        case 'G':
            a = NOTE_UL;
            b = NOTE_U;
            break;
        case 'H':
            a = NOTE_UL;
            b = NOTE_R;
            break;
        case 'I':
            a = NOTE_L;
            b = NOTE_UR;
            break;
        case 'J':
            a = NOTE_D;
            b = NOTE_UR;
            break;
        case 'K':
            a = NOTE_U;
            b = NOTE_UR;
            break;
        case 'L':
            a = NOTE_UR;
            b = NOTE_R;
            break;
        case 'M':
            a = NOTE_UL;
            b = NOTE_UR;
            break;
    };
    first = map[a];
    second = map[b];
}

static char* ReadNoteRow(char* p, NoteList& out, int row, const int* map,
                         int* holds, int quantization) {
    enum { STEP_BIT = 1, HOLD_BIT = 2 };

    int cols[8] = {};
    const char *begin, *end;
    if (*p == '<') {
        begin = ++p;
        for (++p; *p && *p != '>'; ++p);
        end = p;
        if (*p == '>') ++p;
    } else {
        begin = p++;
        if (p[0] == '!' && p[1]) p += 2;
        end = p;
    }
    char fill = STEP_BIT;
    for (const char* n = begin; n != end; ++n) {
        if (*n == '!') {
            fill = HOLD_BIT;
        } else {
            int a, b;
            CharToColumnPair(*n, map, a, b);
            if (a >= 0) cols[a] |= fill;
            if (b >= 0) cols[b] |= fill;
        }
    }
    for (int col = 0; col < 8; ++col) {
        if (cols[col] != 0) {
            if (holds[col]) {
                auto hold = out.begin() + holds[col] - 1;
                hold->endrow = row;
                hold->type = NOTE_STEP_OR_HOLD;
                holds[col] = 0;
            } else {
                out.append({row, row, static_cast<uint32_t>(col), 0,
                            NOTE_STEP_OR_HOLD,
                            static_cast<uint32_t>(quantization)});
                if (cols[col] & HOLD_BIT) {
                    holds[col] = out.size();
                }
            }
        }
    }
    return p;
}

static Difficulty ConvertDifficulty(const std::string& str) {
    if (str == "BEGINNER") return DIFF_BEGINNER;
    if (str == "BASIC") return DIFF_EASY;
    if (str == "ANOTHER") return DIFF_MEDIUM;
    if (str == "MANIAC") return DIFF_HARD;
    if (str == "SMANIAC") return DIFF_CHALLENGE;
    return DIFF_EDIT;
}

static bool ParseNotes(Simfile* sim, char* p, int numPads, int numCols,
                       const char* style) {
    Chart* chart = new Chart;

    // Split chart info "a:b:c:d:..." into parameters.
    int numParams = 0;
    char* params[4] = {};
    while (numParams < 4 && ParseNextItem(p, params[numParams], ':'))
        ++numParams;

    // Check if the chart has enough parameters.
    if (numParams != 2 + numPads) {
        delete chart;
        return false;
    }

    // Parse the note data.
    int map[NOTE_COUNT];
    for (int pad = 0; pad < numPads; ++pad) {
        SetColumnMap(map, numCols, pad);
        char* n = params[2 + pad];
        int holds[8] = {};
        int quantization = 24, row = 0;
        while (*n) {
            switch (*n) {
                case '\t':
                case '\n':
                case ' ':
                case '\r':
                    ++n;
                    break;
                case '(':
                    quantization = 12;
                    ++n;
                    break;
                case '[':
                    quantization = 8;
                    ++n;
                    break;
                case '{':
                    quantization = 4;
                    ++n;
                    break;
                case '`':
                    quantization = 1;
                    ++n;
                    break;
                case ')':
                    quantization = 24;
                    ++n;
                    break;
                case ']':
                    quantization = 24;
                    ++n;
                    break;
                case '}':
                    quantization = 24;
                    ++n;
                    break;
                case '\'':
                    quantization = 32;
                    ++n;
                    break;
                default:
                    n = ReadNoteRow(n, chart->notes, row, map, holds,
                                    quantization);
                    row += quantization;
                    break;
            };
        }
    }

    // Make sure the notes are sorted.
    if (!std::is_sorted(chart->notes.begin(), chart->notes.end(),
                        LessThanRowCol<Note, Note>)) {
        std::sort(chart->notes.begin(), chart->notes.end(),
                  LessThanRowCol<Note, Note>);
    }

    // Assign chart parameters.
    chart->style = gStyle->findStyle(chart->description(), numCols, 1, style);
    chart->difficulty = ConvertDifficulty(params[0]);
    chart->meter = atoi(params[1]);

    sim->charts.push_back(chart);

    return true;
}

// ===================================================================================
// Tag parsing.

static void ParseBpms(Tempo* tempo, char* list) {
    for (char* v[2]; ParseNextItem(list, v, 2);) {
        BpmChange bpmc;
        if (ParseBeat(v[0], bpmc.row) && ParseVal(v[1], bpmc.bpm)) {
            bpmc.row /= 4;
            tempo->segments->append(bpmc);
        }
    }
}

static void ParseStops(Tempo* tempo, char* list) {
    for (char* v[2]; ParseNextItem(list, v, 2);) {
        Stop stop;
        if (ParseBeat(v[0], stop.row) && ParseVal(v[1], stop.seconds)) {
            stop.row /= 4;
            stop.seconds *= 0.001;
            tempo->segments->append(stop);
        }
    }
}

static void ParseDisplayBpm(Tempo* tempo, char* bpm) {
    if (strcmp(bpm, "*") == 0) {
        tempo->displayBpmType = BPM_RANDOM;
        tempo->displayBpmRange = {0, 0};
    } else {
        char* dots = strstr(bpm, "..");
        double min, max;
        if (dots) {
            *dots = 0;
            if (ParseVal(bpm, min) && ParseVal(dots + 2, max)) {
                tempo->displayBpmType = BPM_CUSTOM;
                tempo->displayBpmRange = {min, max};
            }
        } else if (ParseVal(bpm, min)) {
            tempo->displayBpmType = BPM_CUSTOM;
            tempo->displayBpmRange = {min, min};
        }
    }
}

static void ParseTag(Simfile* sim, std::string tag, char* val) {
    if (tag == "SINGLE") {
        ParseNotes(sim, val, 1, 4, "dance-single");
    } else if (tag == "DOUBLE") {
        ParseNotes(sim, val, 2, 8, "dance-double");
    } else if (tag == "COUPLE") {
        ParseNotes(sim, val, 2, 8, "dance-couple");
    } else if (tag == "SOLO") {
        ParseNotes(sim, val, 1, 6, "dance-solo");
    } else if (tag == "FILE") {
        sim->music = val;
    } else if (tag == "GAP") {
        double gap;
        if (ParseVal(val, gap)) {
            sim->tempo->offset = gap * -0.001;
        }
    } else if (tag == "BPM") {
        BpmChange bpmc;
        if (ParseVal(val, bpmc.bpm)) {
            sim->tempo->segments->insert(bpmc);
        }
    } else if (tag == "CHANGEBPM") {
        ParseBpms(sim->tempo, val);
    } else if (tag == "FREEZE") {
        ParseStops(sim->tempo, val);
    } else if (tag == "TITLE") {
        sim->title = val;
    } else if (tag == "ARTIST") {
        sim->artist = val;
    } else if (tag == "GENRE") {
        sim->genre = val;
    } else if (tag == "DISPLAYBPM") {
        ParseDisplayBpm(sim->tempo, val);
    }
}

// ===================================================================================
// File loading.

bool LoadDwi(fs::path path, Simfile* sim) {
    // Read the file.
    std::string str;
    if (!ParseSimfile(str, path)) return false;

    // Read the tags.
    char *tag, *val, *p = &str[0];
    while (ParseNextTag(p, tag, val)) {
        ParseTag(sim, tag, val);
    }

    // Some things are not stored in the .dwi file, look for them
    // Basic guesses only
    auto paths = File::findFiles(sim->dir, false);
    auto simname = sim->file;
    simname = simname.substr(0, simname.find_last_of("."));
    Str::toLower(simname);
    for (auto& path : paths) {
        std::string f = pathToUtf8(path.filename());
        std::string fl(f);
        Str::toLower(fl);

        if (fl.starts_with("!") && sim->cdTitle.empty()) {
            sim->cdTitle = f;
        } else if (fl.compare(simname + ".mp3") == 0 && sim->music.empty()) {
            sim->music = f;
        } else if (fl.compare(simname + ".png") == 0) {
            sim->banner = f;
        } else if (fl.compare(simname + "-bg.png") == 0) {
            sim->background = f;
        }
    }

    sim->format = SIM_DWI;

    return true;
}

};  // namespace Dwi
};  // namespace Vortex
