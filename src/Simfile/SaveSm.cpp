#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/TimingData.h>

#include <Managers/StyleMan.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <list>
#include <filesystem>

namespace Vortex {
namespace Sm {
namespace {

enum SeperatorPos { END_OF_LINE, START_OF_LINE };
enum ForceWrite { ALWAYS, SONG_ONLY, NEVER };

static const int MEASURE_SUBDIV[] = {4, 8, 12, 16, 24, 32, 48, 64, 96, 192};
static const int NUM_MEASURE_SUBDIV = 10;
static const int ROWS_PER_NOTE_SECTION = 192;
static const int MIN_SECTIONS_PER_MEASURE = 4;

static double ToBeat(int row) {
    return static_cast<double>(row) / ROWS_PER_BEAT;
}

struct ExportData {
    Vector<int> diffs;
    std::ofstream file;
    const Simfile* sim;
    const Chart* chart;
    bool ssc;
};

// ================================================================================================
// Generic write functions.

static void GiveUnicodeWarning(const std::string& path,
                               const std::string& name) {
    if (Str::isUnicode(path)) {
        HudWarning(
            "The %s path contains unicode characters,\n"
            "which might not work on some versions of Stepmania/ITG.",
            name.c_str());
    }
}

static std::string Escape(const char* name, const char* str) {
    std::string s(str);
    // These characters must be escaped in the .sm/.ssc file
    Str::replace(s, "\\", "\\\\");
    Str::replace(s, ":", "\\:");
    Str::replace(s, ";", "\\;");
    Str::replace(s, "#", "\\#");
    return s;
}

static std::string Escape(const std::string& name, const std::string& _str) {
    std::string s(_str);
    // These characters must be escaped in the .sm/.ssc file
    Str::replace(s, "\\", "\\\\");
    Str::replace(s, ":", "\\:");
    Str::replace(s, ";", "\\;");
    Str::replace(s, "#", "\\#");
    return s;
}

static bool ShouldWrite(ExportData& data, ForceWrite when, bool hasValue,
                        bool sscOnly) {
    bool versionOk = (data.ssc || !sscOnly);
    bool writeOk = hasValue || (when == ALWAYS) ||
                   (when == SONG_ONLY && data.chart == nullptr);
    return versionOk && writeOk;
}

static void WriteTag(ExportData& data, const std::string& tag,
                     const std::string& value, ForceWrite when, bool sscOnly) {
    if (ShouldWrite(data, when, value.length() != 0, sscOnly)) {
        data.file << "#" << tag << ':' << value << ";\n";
    }
}

static void WriteTag(ExportData& data, const std::string& tag, double value,
                     ForceWrite when, bool sscOnly) {
    if (ShouldWrite(data, when, value != 0, sscOnly)) {
        data.file << std::fixed << std::setprecision(6) << "#" << tag << ':'
                  << value << ";\n";
    }
}

static void WriteTag(ExportData& data, const std::string& tag, int value,
                     ForceWrite when, bool sscOnly) {
    if (ShouldWrite(data, when, value != 0, sscOnly)) {
        data.file << "#" << tag << ':' << value << ";\n";
    }
}

static void WriteTextTag(ExportData& data, const std::string& tag,
                         const std::string& value, ForceWrite when,
                         bool sscOnly, const char* alt = nullptr) {
    if (alt && value.empty()) {
        WriteTag(data, tag, Escape(tag, alt), when, sscOnly);
    } else {
        WriteTag(data, tag, Escape(tag, value.c_str()), when, sscOnly);
    }
}

static void WritePathTag(ExportData& data, const char* tag,
                         const std::string& value, ForceWrite when,
                         bool sscOnly, const char* alt = nullptr) {
    GiveUnicodeWarning(value, tag);
    WriteTextTag(data, tag, value, when, sscOnly, alt);
}

template <typename T, typename F>
static void WriteTag(ExportData& data, const char* tag, const T& list,
                     SeperatorPos pos, char seperator, ForceWrite when,
                     bool sscOnly, F func) {
    if (ShouldWrite(data, when, list.size() != 0, sscOnly)) {
        auto& file = data.file;
        file << "#" << tag << ":";
        for (auto it = list.begin(); it != list.end();) {
            if (it != list.begin() && pos == START_OF_LINE) {
                file << seperator;
            }
            func(*it);
            if (++it != list.end() && pos == END_OF_LINE) {
                file << seperator;
            }
            if (list.size() > 1) {
                file << '\n';
            }
        }
        file << ";\n";
    }
}

template <typename T, typename F>
static void WriteSegments(ExportData& data, const char* tag, const Tempo* tempo,
                          SeperatorPos pos, char seperator, ForceWrite when,
                          bool sscOnly, F func) {
    auto begin = tempo->segments->begin<T>();
    auto end = tempo->segments->end<T>();
    if (ShouldWrite(data, when, begin != end, sscOnly)) {
        auto& file = data.file;
        file << "#" << tag << ":";
        for (auto it = begin; it != end; ++it) {
            if (it != begin && pos == START_OF_LINE) {
                file << seperator;
            }
            func(*it);
            if (it + 1 != end && pos == END_OF_LINE) {
                file << seperator;
            }
            if (begin + 1 < end) {
                file << '\n';
            }
        }
        file << ";\n";
    }
}

// ================================================================================================
// Tempo write functions.

static void WriteOffset(ExportData& data, const Tempo* tempo) {
    WriteTag(data, "OFFSET", tempo->offset, SONG_ONLY, false);
}

static void WriteBpms(ExportData& data, const Tempo* tempo) {
    WriteSegments<BpmChange>(data, "BPMS", tempo, START_OF_LINE, ',', SONG_ONLY,
                             false, [&](const BpmChange& change) {
                                 data.file << std::fixed << std::setprecision(6)
                                           << ToBeat(change.row) << '='
                                           << std::setprecision(6)
                                           << change.bpm;
                             });
}

static void WriteStops(ExportData& data, const Tempo* tempo) {
    WriteSegments<Stop>(data, "STOPS", tempo, START_OF_LINE, ',', SONG_ONLY,
                        false, [&](const Stop& stop) {
                            data.file << std::fixed << std::setprecision(6)
                                      << ToBeat(stop.row) << '='
                                      << stop.seconds;
                        });
}

static void WriteDelays(ExportData& data, const Tempo* tempo) {
    WriteSegments<Delay>(data, "DELAYS", tempo, END_OF_LINE, ',', NEVER, true,
                         [&](const Delay& delay) {
                             data.file << std::fixed << std::setprecision(6)
                                       << ToBeat(delay.row) << '='
                                       << delay.seconds;
                         });
}

static void WriteWarps(ExportData& data, const Tempo* tempo) {
    WriteSegments<Warp>(data, "WARPS", tempo, END_OF_LINE, ',', NEVER, true,
                        [&](const Warp& warp) {
                            data.file << std::fixed << std::setprecision(6)
                                      << ToBeat(warp.row) << '='
                                      << ToBeat(warp.numRows);
                        });
}

static void WriteSpeeds(ExportData& data, const Tempo* tempo) {
    WriteSegments<Speed>(data, "SPEEDS", tempo, END_OF_LINE, ',', NEVER, true,
                         [&](const Speed& speed) {
                             data.file << std::fixed << std::setprecision(6)
                                       << ToBeat(speed.row) << '='
                                       << speed.ratio << '=' << speed.delay
                                       << '=' << speed.unit;
                         });
}

static void WriteScrolls(ExportData& data, const Tempo* tempo) {
    WriteSegments<Scroll>(data, "SCROLLS", tempo, END_OF_LINE, ',', NEVER, true,
                          [&](const Scroll& scroll) {
                              data.file << std::fixed << std::setprecision(6)
                                        << ToBeat(scroll.row) << '='
                                        << scroll.ratio;
                          });
}

static void WriteTickCounts(ExportData& data, const Tempo* tempo) {
    WriteSegments<TickCount>(data, "TICKCOUNTS", tempo, END_OF_LINE, ',', NEVER,
                             true, [&](const TickCount& tick) {
                                 data.file << std::fixed << std::setprecision(6)
                                           << ToBeat(tick.row) << '='
                                           << tick.ticks;
                             });
}

static void WriteTimeSignatures(ExportData& data, const Tempo* tempo) {
    WriteSegments<TimeSignature>(
        data, "TIMESIGNATURES", tempo, END_OF_LINE, ',', NEVER, true,
        [&](const TimeSignature& sig) {
            data.file << std::fixed << std::setprecision(6) << ToBeat(sig.row)
                      << '=' << static_cast<int>(ToBeat(sig.rowsPerMeasure))
                      << '=' << sig.beatNote;
        });
}

static void WriteLabels(ExportData& data, const Tempo* tempo) {
    WriteSegments<Label>(data, "LABELS", tempo, END_OF_LINE, ',', NEVER, true,
                         [&](const Label& label) {
                             data.file << std::fixed << std::setprecision(6)
                                       << ToBeat(label.row) << '=' << label.str;
                         });
}

static void WriteAttacks(ExportData& data, const Tempo* tempo) {
    WriteTag(data, "ATTACKS", tempo->attacks, START_OF_LINE, ':', NEVER, true,
             [&](const Attack& attack) {
                 data.file << std::fixed << std::setprecision(6)
                           << "TIME=" << attack.time << ':'
                           << ((attack.unit == ATTACK_END) ? "END=" : "LEN=")
                           << attack.duration << ":"
                           << "MODS=" << attack.mods;
             });
}

static void WriteKeySounds(ExportData& data, const Tempo* tempo) {
    WriteTag(data, "KEYSOUNDS", tempo->keysounds, END_OF_LINE, ',', NEVER, true,
             [&](const std::string& str) { data.file << str; });
}

static void WriteCombos(ExportData& data, const Tempo* tempo) {
    WriteSegments<Combo>(data, "COMBOS", tempo, END_OF_LINE, ',', NEVER, true,
                         [&](const Combo& combo) {
                             data.file << std::fixed << std::setprecision(6)
                                       << ToBeat(combo.row) << '='
                                       << combo.hitCombo;
                             if (combo.hitCombo != combo.missCombo)
                                 data.file << '=' << combo.missCombo;
                         });
}

static void WriteFakes(ExportData& data, const Tempo* tempo) {
    WriteSegments<Fake>(data, "FAKES", tempo, END_OF_LINE, ',', NEVER, true,
                        [&](const Fake& fake) {
                            data.file << std::fixed << std::setprecision(6)
                                      << ToBeat(fake.row) << '='
                                      << ToBeat(fake.numRows);
                        });
}

static void WriteDisplayBpm(ExportData& data, const Tempo* tempo) {
    if (tempo->displayBpmType == BPM_CUSTOM) {
        if (tempo->displayBpmRange.min == tempo->displayBpmRange.max) {
            WriteTag(data, "DISPLAYBPM", tempo->displayBpmRange.min, ALWAYS,
                     false);
        } else {
            Str::fmt fmt = "%1:%2";
            fmt.arg(tempo->displayBpmRange.min, 6);
            fmt.arg(tempo->displayBpmRange.max, 6);
            WriteTag(data, "DISPLAYBPM", fmt, ALWAYS, false);
        }
    } else if (tempo->displayBpmType == BPM_RANDOM) {
        WriteTag(data, "DISPLAYBPM", "*", ALWAYS, false);
    }
}

static void WriteTempo(ExportData& data, const Tempo* tempo) {
    WriteOffset(data, tempo);

    WriteBpms(data, tempo);
    WriteStops(data, tempo);
    WriteDelays(data, tempo);
    WriteWarps(data, tempo);
    WriteSpeeds(data, tempo);
    WriteScrolls(data, tempo);
    WriteTickCounts(data, tempo);
    WriteTimeSignatures(data, tempo);

    WriteLabels(data, tempo);
    WriteAttacks(data, tempo);
    WriteKeySounds(data, tempo);
    WriteCombos(data, tempo);
    WriteFakes(data, tempo);
    WriteDisplayBpm(data, tempo);

    for (auto& misc : tempo->misc) {
        WriteTag(data, misc.tag.c_str(), misc.val, ALWAYS, false);
    }
}

// ================================================================================================
// Misc write functions.

static void WriteBgChanges(ExportData& data, const char* tag,
                           const Vector<BgChange>& bgs) {
    WriteTag(data, tag, bgs, END_OF_LINE, ',', ALWAYS, false,
             [&](const BgChange& bg) {
                 if (bg.effect.empty() && bg.file2.empty() &&
                     bg.transition.empty() && bg.color.empty() &&
                     bg.color2.empty()) {
                     data.file << std::fixed << std::setprecision(6)
                               << bg.startBeat << '=' << bg.file << '='
                               << bg.rate << "=0=0=1";
                 } else {
                     data.file
                         << std::fixed << std::setprecision(6) << bg.startBeat
                         << '=' << bg.file << '=' << bg.rate << '='
                         << (bg.transition == "CrossFade") << '='
                         << (bg.effect == "StretchRewind") << '='
                         << (bg.effect != "StretchNoLoop") << '=' << bg.effect
                         << '=' << bg.file2 << '=' << bg.transition << '='
                         << bg.color << '=' << bg.color2 << '=';
                 }
             });
}

// ================================================================================================
// Chart writing functions.

static char GetNoteChar(uint32_t type) {
    if (type == NOTE_STEP_OR_HOLD) {
        return '1';
    } else if (type == NOTE_MINE) {
        return 'M';
    } else if (type == NOTE_LIFT) {
        return 'L';
    } else if (type == NOTE_FAKE) {
        return 'F';
    }
    return '0';
}

static char GetHoldChar(uint32_t type) {
    return (type == NOTE_STEP_OR_HOLD) ? '2' : '4';
}

static std::string RadarToString(const Vector<double>& list) {
    std::string out;
    if (list.empty()) {
        out = "0,0,0,0,0";
    } else {
        for (double it : list) {
            if (out.length()) Str::append(out, ',');
            Str::appendVal(out, it, 0, 6);
        }
    }
    return out;
}

static const char* GetDifficultyString(Difficulty difficulty) {
    switch (difficulty) {
        case DIFF_BEGINNER:
            return "Beginner";
        case DIFF_EASY:
            return "Easy";
        case DIFF_MEDIUM:
            return "Medium";
        case DIFF_HARD:
            return "Hard";
        case DIFF_CHALLENGE:
            return "Challenge";
    };
    return "Edit";
}

static inline bool TestSectionCompression(const char* section, int width,
                                          int quant) {
    std::string zeroline(width, '0');
    float mod = static_cast<float>(ROWS_PER_NOTE_SECTION) / quant;
    for (int j = 0; j < ROWS_PER_NOTE_SECTION; ++j) {
        float rem = round(fmod(j, mod));
        // Check all the compressed rows and make sure they are empty
        if (rem > 0 && rem < static_cast<int>(mod) &&
            memcmp(section + j * width, zeroline.c_str(), width)) {
            return false;
        }
    }
    return quant >= MIN_SECTIONS_PER_MEASURE;
}

static void GetSectionCompression(const char* section, int width, int& count,
                                  int& pitch) {
    count = ROWS_PER_NOTE_SECTION;
    // Determines the best compression for the given section.
    // We actually don't care about custom snaps for this at all, since we want
    // to save 192nds for non-standard-compressible snaps.
    for (int i = 0; i < NUM_MEASURE_SUBDIV - 1; i++) {
        if (TestSectionCompression(section, width, MEASURE_SUBDIV[i])) {
            count = MEASURE_SUBDIV[i];
            break;
        }
    }
    pitch = (ROWS_PER_NOTE_SECTION * width) / count;
}

static void WriteSections(ExportData& data) {
    const Chart* chart = data.chart;

    int numCols = chart->style->numCols;
    int numPlayers = chart->style->numPlayers;

    if (numPlayers == 0 || numCols == 0) return;

    // Allocate a buffer for one uncompressed section of notes.
    int sectionSize = ROWS_PER_NOTE_SECTION * numCols;
    Vector<char> sectionVec;
    sectionVec.resize(sectionSize);
    char* section = sectionVec.data();

    // Export note data for each player.
    for (int pn = 0; pn < numPlayers; ++pn) {
        int startRow = 0;
        int remainingHolds = 0;

        Vector<const Note*> holdVec(numCols, nullptr);
        const Note** holds = holdVec.begin();

        std::list<uint32_t> quantVec;

        const Note* it = chart->notes.begin();
        const Note* end = chart->notes.end();

        // Write all notes for the current player in blocks of one section.
        for (; it != end || remainingHolds > 0;
             startRow += ROWS_PER_NOTE_SECTION) {
            memset(section, '0', sectionSize);
            int endRow = startRow + ROWS_PER_NOTE_SECTION;

            // Advance to the first note in the current section.
            for (; it != end && it->row < startRow; ++it);

            // Write the notes of the current player to the section data.
            for (; it != end && it->row < endRow; ++it) {
                if (static_cast<int>(it->player) == pn) {
                    int pos = (it->row - startRow) * numCols + it->col;
                    if (it->row == it->endrow) {
                        section[pos] = GetNoteChar(it->type);
                    } else {
                        section[pos] = GetHoldChar(it->type);
                        auto hold = holds[it->col];
                        if (hold) {
                            if (hold->endrow >= startRow &&
                                hold->endrow < endRow) {
                                int pos = (hold->endrow - startRow) * numCols +
                                          static_cast<int>(hold->col);
                                section[pos] = '3';
                                --remainingHolds;
                            }
                        }
                        holds[it->col] = it;
                        ++remainingHolds;
                    }
                }
            }

            // Write the remaining hold ends to the section data.
            if (remainingHolds > 0) {
                for (int col = 0; col < numCols; ++col) {
                    auto hold = holds[col];
                    if (hold) {
                        if (hold->endrow >= startRow && hold->endrow < endRow) {
                            int pos =
                                (hold->endrow - startRow) * numCols + hold->col;
                            section[pos] = '3';
                            holds[col] = nullptr;
                            --remainingHolds;
                        }
                    }
                }
            }

            // Write the current section to the file.
            int count, pitch;
            const char* m = section;
            quantVec.unique();
            GetSectionCompression(m, numCols, count, pitch);
            quantVec.clear();
            if (ROWS_PER_NOTE_SECTION % count != 0) {
                HudError(
                    "Bug: trying to save a non-supported number of rows. Data "
                    "loss expected.");
            }
            for (int k = 0; k < count; ++k, m += pitch) {
                for (int i = 0; i < numCols; i++) data.file << m[i];
                data.file << '\n';
            }

            // Write a comma if this is not the last section.
            if (it != end || remainingHolds > 0) data.file << ",\n";
        }

        // Write an ampersand if this is not the last player.
        if (pn != numPlayers - 1) data.file << "&\n";
    }
    data.file << ";\n";
}

static void WriteChart(ExportData& data) {
    const Chart* chart = data.chart;

    // Make sure each difficulty type is only exported once.
    Difficulty diff = chart->difficulty;
    int sd = chart->style->index * NUM_DIFFICULTIES + chart->difficulty;
    if (data.diffs.find(sd) != data.diffs.size()) {
        Difficulty oldDiff = diff;
        std::string oldDesc = chart->description();
        diff = DIFF_EDIT;
        std::string newDesc = chart->description();

        HudWarning("Duplicate difficulties, saving (%s) as (%s) instead",
                   oldDesc.c_str(), newDesc.c_str());
    } else if (chart->difficulty != DIFF_EDIT) {
        data.diffs.push_back(sd);
    }

    // Write the output chart data.
    data.file << "//--------------- " << chart->style->id << " - "
              << chart->artist << " ----------------\n";

    std::string chartStyle = Escape("chart style", chart->style->id);
    std::string chartArtist = Escape("chart artist", chart->artist);

    if (data.ssc) {
        WriteTag(data, "NOTEDATA", std::string(), ALWAYS, true);
        WriteTag(data, "STEPSTYPE", chartStyle, ALWAYS, true);
        WriteTag(data, "DESCRIPTION", chartArtist, ALWAYS, true);
        WriteTag(data, "DIFFICULTY", GetDifficultyString(diff), ALWAYS, true);
        WriteTag(data, "METER", chart->meter, ALWAYS, true);
        WriteTag(data, "RADARVALUES", RadarToString(chart->radar), ALWAYS,
                 true);

        if (chart->tempo) WriteTempo(data, chart->tempo);

        data.file << "#NOTES:\n";
    } else {
        data.file << "#NOTES:\n";
        data.file << "     " << chartStyle << ":\n";
        data.file << "     " << chartArtist << ":\n";
        data.file << "     " << GetDifficultyString(diff) << ":\n";
        data.file << "     " << chart->meter << ":\n";
        data.file << "     " << RadarToString(chart->radar) << ":\n";
    }

    WriteSections(data);
}

// ================================================================================================
// Simfile saving.

bool SaveSimfile(const Simfile* sim, bool ssc, bool backup) {
    ExportData data;
    data.ssc = ssc;
    data.chart = nullptr;
    data.sim = sim;

    fs::path path = utf8ToPath(sim->dir);
    path.append(stringToUtf8(sim->file));
    path.replace_extension(ssc ? ".ssc" : ".sm");
    fs::path oldPath = fs::path(path);
    oldPath.concat(".old");

    // If a backup file is requested, rename the existing sim before saving over
    // it.
    if (backup && fs::exists(path)) {
        std::error_code error;
        fs::rename(path, oldPath, error);
        if (error) {
            HudError("Could not backup \"%s\", error %s.",
                     pathToUtf8(path).c_str(), error.message().c_str());
        }
    }

    // Open the output file.
    data.file.open(path.c_str());
    if (data.file.fail()) return false;
    GiveUnicodeWarning(pathToUtf8(path.filename()), "sim");

    // Start with a version tag for SSC files.
    if (ssc) WriteTag(data, "VERSION", "0.83", ALWAYS, true);

    WriteTextTag(data, "TITLE", sim->title, ALWAYS, false, "unknown");
    WriteTextTag(data, "SUBTITLE", sim->subtitle, ALWAYS, false);
    WriteTextTag(data, "ARTIST", sim->artist, ALWAYS, false);
    WriteTextTag(data, "TITLETRANSLIT", sim->titleTr, ALWAYS, false);
    WriteTextTag(data, "SUBTITLETRANSLIT", sim->subtitleTr, ALWAYS, false);
    WriteTextTag(data, "ARTISTTRANSLIT", sim->artistTr, ALWAYS, false);
    WriteTextTag(data, "GENRE", sim->genre, ALWAYS, false);
    WriteTextTag(data, "CREDIT", sim->credit, ALWAYS, false);

    WritePathTag(data, "MUSIC", sim->music, ALWAYS, false);
    WritePathTag(data, "BANNER", sim->banner, ALWAYS, false);
    WritePathTag(data, "BACKGROUND", sim->background, ALWAYS, false);
    WritePathTag(data, "CDTITLE", sim->cdTitle, ALWAYS, false);

    WriteTag(data, "SAMPLESTART", sim->previewStart, ALWAYS, false);
    WriteTag(data, "SAMPLELENGTH", sim->previewLength, ALWAYS, false);
    WriteTag(data, "SELECTABLE", sim->isSelectable ? "YES" : "NO", ALWAYS,
             false);

    if (sim->previewLength > 0 && sim->previewLength < 3) {
        HudWarning(
            "The music preview is shorter than 3 seconds, which will default "
            "to 12 seconds in ITG.");
    } else if (sim->previewLength > 30) {
        HudWarning(
            "The music preview is longer than 30 seconds, which will default "
            "to 12 seconds in ITG.");
    }

    WriteTempo(data, sim->tempo);

    WriteBgChanges(data, "BGCHANGES", sim->bgChanges[0]);
    if (sim->bgChanges[1].size()) {
        WriteBgChanges(data, "BGCHANGES2", sim->bgChanges[1]);
    }
    WriteBgChanges(data, "FGCHANGES", sim->fgChanges);

    for (auto& chart : sim->charts) {
        data.chart = chart;
        WriteChart(data);
        data.chart = nullptr;
    }

    HudInfo("Saved: %s", pathToUtf8(path.filename()).c_str());

    return true;
}

};  // anonymous namespace.

bool SaveSm(const Simfile* sim, bool backup) {
    return SaveSimfile(sim, false, backup);
}

bool SaveSsc(const Simfile* sim, bool backup) {
    return SaveSimfile(sim, true, backup);
}

};  // namespace Sm
};  // namespace Vortex