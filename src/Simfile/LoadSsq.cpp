#include <Core/Core.h>
#include <Core/StringUtils.h>
#include <Core/ByteStream.h>

#include <Editor/Common.h>

#include <Managers/StyleMan.h>

#include <Simfile/Parsing.h>
#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/Segments.h>

#include <System/File.h>

#include <algorithm>
#include <vector>
#include <fstream>

namespace Vortex {
namespace Ssq {

enum : int16_t {
    CHUNK_TIMINGS = 1,
    CHUNK_TRIGGERS = 2,
    CHUNK_STEPS = 3,
    CHUNK_META = 9,
};

const double MEASURE_LENGTH = 4096;
const int DEFAULT_RATE = 150;

struct RawChunk {
    int16_t chunkType, config;
    std::vector<uint8_t> data;
};

struct TimingPoint {
    int position, ticks;
};

struct StepEvent {
    int position;
    uint8_t panels, extraPanels, extraInfo;
    bool hasExtraData;
};

struct MetaInfo {
    std::string title, subtitle, artist;
};

static bool ReadAllChunks(const std::vector<uint8_t>& file,
                          std::vector<RawChunk>& out) {
    ReadStream in(file.data(), file.size());
    while (in.bytesleft() >= 4) {
        int32_t length = in.read<int32_t>();
        if (!in.success()) return false;
        if (length == 0) break;
        if (length < 8) return false;

        RawChunk chunk;
        chunk.chunkType = in.read<int16_t>();
        chunk.config = in.read<int16_t>();
        chunk.data.resize(length - 8);
        in.read(chunk.data.data(), length - 8);
        if (!in.success()) return false;

        out.push_back(std::move(chunk));
    }
    return true;
}

static std::vector<TimingPoint> ParseTimings(const std::vector<uint8_t>& data) {
    ReadStream in(data.data(), data.size());
    int32_t count = in.read<int32_t>();

    std::vector<int32_t> position(count), ticks(count);
    for (int i = 0; i < count; ++i) position[i] = in.read<int32_t>();
    for (int i = 0; i < count; ++i) ticks[i] = in.read<int32_t>();

    std::vector<TimingPoint> out(count);
    for (int i = 0; i < count; ++i) out[i] = {position[i], ticks[i]};
    return out;
}

static std::vector<StepEvent> ParseSteps(const std::vector<uint8_t>& data) {
    ReadStream in(data.data(), data.size());
    int32_t count = in.read<int32_t>();

    std::vector<int32_t> metric(count);
    for (int i = 0; i < count; ++i) metric[i] = in.read<int32_t>();

    std::vector<uint8_t> panels(count);
    in.read(panels.data(), count);
    if (count & 1) in.skip(1);  // pad to an even byte count

    std::vector<StepEvent> out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        StepEvent ev{metric[i], panels[i], 0, 0, panels[i] == 0};
        if (ev.hasExtraData) {
            ev.extraPanels = in.read<uint8_t>();
            ev.extraInfo = in.read<uint8_t>();
        }
        out.push_back(ev);
    }
    return out;
}

static MetaInfo ParseMeta(const std::vector<uint8_t>& data) {
    MetaInfo meta;
    size_t pos = 0;
    std::string fields[12];
    for (int i = 0; i < 12 && pos < data.size(); ++i) {
        std::string s;
        while (pos < data.size() && data[pos] != 0)
            s += static_cast<char>(data[pos++]);
        if (pos < data.size()) ++pos;
        fields[i] = s;
    }
    meta.title = fields[0];
    meta.subtitle = fields[1];
    meta.artist = fields[2];
    return meta;
}

// SSQ 4096 per measure -> SM 192 per measure.
// 192/4096 reduces to 3/64, rounded to nearest.
static int PositionToRow(int position) {
    long long r = static_cast<long long>(position) * 3;
    return static_cast<int>(r >= 0 ? (r + 32) / 64 : -((-r + 32) / 64));
}

static Difficulty ParseDifficulty(int chartId) {
    switch ((chartId >> 8) & 0xFF) {
        case 0x01:
            return DIFF_EASY;
        case 0x02:
            return DIFF_MEDIUM;
        case 0x03:
            return DIFF_HARD;
        case 0x04:
            return DIFF_BEGINNER;
        case 0x06:
            return DIFF_CHALLENGE;
        case 0x10:
            return DIFF_MEDIUM;  // Couple charts use this value.
        default:
            return DIFF_EDIT;
    }
}

static bool ParseStyle(int chartId, int& numCols, int& numPlayers) {
    numPlayers = (chartId & 0x00F0) >> 4;
    numCols = (chartId & 0x000F) * numPlayers;
    return numCols != 0;
}

static std::string GetStyleId(int numCols, int numPlayers) {
    if (numPlayers == 1) {
        if (numCols <= 4) return "dance-single";
        if (numCols <= 6) return "dance-solo";
        return "dance-double";
    }
    return "dance-couple";
}

bool LoadSsq(fs::path path, Simfile* sim) {
    bool success;
    std::vector<uint8_t> bytes = File::getBytes(path, &success);
    if (!success) return false;

    std::vector<RawChunk> chunks;
    if (!ReadAllChunks(bytes, chunks)) return false;

    // Get Tempo Data
    std::vector<TimingPoint> timings;
    int rate = DEFAULT_RATE;
    bool haveRate = false;
    for (auto& c : chunks) {
        if (c.chunkType != CHUNK_TIMINGS) continue;
        if (!haveRate) {
            rate = c.config;
            haveRate = true;
        }
        auto pts = ParseTimings(c.data);
        timings.insert(timings.end(), pts.begin(), pts.end());
    }
    if (timings.empty()) return false;

    std::sort(timings.begin(), timings.end(), [](auto& a, auto& b) {
        return a.ticks != b.ticks ? a.ticks < b.ticks : a.position < b.position;
    });

    double initialBpm = SIM_DEFAULT_BPM;
    for (size_t i = 0; i + 1 < timings.size(); ++i) {
        int dOffset = timings[i + 1].position - timings[i].position;
        int dTicks = timings[i + 1].ticks - timings[i].ticks;
        if (dOffset != 0 && dTicks != 0) {
            initialBpm = (dOffset * 240.0 * rate) / (MEASURE_LENGTH * dTicks);
            break;
        }
    }
    double startTime = static_cast<double>(timings[0].ticks) / rate;
    double startBeat =
        PositionToRow(timings[0].position) / static_cast<double>(ROWS_PER_BEAT);
    sim->tempo->offset = -(startTime - startBeat * (60.0 / initialBpm));

    for (size_t i = 0; i + 1 < timings.size(); ++i) {
        int row = PositionToRow(timings[i].position);
        int dOffset = timings[i + 1].position - timings[i].position;
        int dTicks = timings[i + 1].ticks - timings[i].ticks;

        if (dOffset == 0) {
            if (dTicks > 0)
                sim->tempo->segments->append(
                    Stop(row, static_cast<double>(dTicks) / rate));
        } else if (dTicks != 0) {
            double bpm = (dOffset * 240.0 * rate) / (MEASURE_LENGTH * dTicks);
            sim->tempo->segments->append(BpmChange(row, bpm));
        }
    }

    // Get Charts
    for (auto& c : chunks) {
        if (c.chunkType != CHUNK_STEPS) continue;

        int chartId = c.config;
        int numCols, numPlayers;
        if (!ParseStyle(chartId, numCols, numPlayers)) continue;
        std::string styleId = GetStyleId(numCols, numPlayers);

        auto steps = ParseSteps(c.data);

        Chart* chart = new Chart;
        chart->style = gStyle->findStyle(styleId, numCols, numPlayers, styleId);
        chart->difficulty = ParseDifficulty(chartId);

        std::vector<int> lastPos(numCols, 0);

        for (auto& ev : steps) {
            int row = PositionToRow(ev.position);
            uint8_t panels = ev.hasExtraData ? ev.extraPanels : ev.panels;

            // Shock Note (Mines)
            if ((panels & 0x0F) == 0x0F) {  // Pad 1
                for (uint32_t col = 0; col < 4; ++col) {
                    chart->notes.append({row, row, col, 0, NOTE_MINE, 192});
                }
                panels &= 0xF0;
            }
            if ((panels & 0xF0) == 0xF0) {  // Pad 2
                for (uint32_t col = 4; col < 8; ++col) {
                    chart->notes.append({row, row, col, 0, NOTE_MINE, 192});
                }
                panels &= 0x0F;
            }
            if (panels == 0x00) continue;

            // Normal Notes
            for (uint32_t col = 0; col < numCols; ++col) {
                if (!(panels & (1 << col))) continue;

                // Freeze Note - Marker for Hold end.
                if (ev.hasExtraData && ev.extraInfo == 0x01) {
                    int startPos = lastPos[col];
                    if (startPos) {
                        auto* hold = chart->notes.begin() + startPos - 1;
                        hold->endrow = row;
                        lastPos[col] = 0;
                    }
                } else {
                    chart->notes.append(
                        {row, row, col, 0, NOTE_STEP_OR_HOLD, 192});
                    lastPos[col] = chart->notes.size();
                }
            }
        }

        sim->charts.push_back(chart);
    }

    // Get Metadata
    for (auto& c : chunks) {
        if (c.chunkType != CHUNK_META) continue;
        MetaInfo meta = ParseMeta(c.data);
        sim->title = meta.title;
        sim->subtitle = meta.subtitle;
        sim->artist = meta.artist;
        break;
    }

    return !sim->charts.empty();
}

};  // namespace Ssq
};  // namespace Vortex