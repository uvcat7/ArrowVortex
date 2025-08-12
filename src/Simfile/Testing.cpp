#include <Core/StringUtils.h>

#include <Simfile/Parsing.h>

#include <System/File.h>

#include <math.h>

// #define ENABLE_TESTING

namespace Vortex {

// ================================================================================================
// Verify functions.

#ifdef ENABLE_TESTING

static bool FuzzyEquals(double a, double b) { return abs(a - b) < 0.001; }

static bool Check(const char* name, double a, double b) {
    bool equal = FuzzyEquals(a, b);
    if (!equal) HudError("%s: %f | %f", name, a, b);
    return equal;
}

static bool Check(const char* name, int a, int b) {
    if (a != b) HudError("%s: %i | %i", name, (int)a, (int)b);
    return (a == b);
}

static bool Check(const char* name, uint32_t a, uint32_t b) {
    if (a != b) HudError("%s: %i | %i", name, (int)a, (int)b);
    return (a == b);
}

static bool Check(const char* name, const std::string& a,
                  const std::string& b) {
    if (a != b) HudError("%s: %s | %s", name, a.c_str(), b.c_str());
    return (a == b);
}

template <typename T, typename Equal>
void VerifyVector(const Vector<T>& a, const Vector<T>& b, const char* name,
                  const char* subname, Equal eq) {
    if (a.size() != b.size()) {
        HudError("%s :: %s mismatch in size: %i and %i", name, subname,
                 a.size(), b.size());
    } else {
        for (int i = 0; i < a.size(); ++i) {
            if (!eq(a[i], b[i])) {
                HudError("%s :: %s mismatch at element %i", name, subname, i);
            }
        }
    }
}

static void VerifyStr(const std::string& a, const std::string& b,
                      const char* name, const char* subname) {
    if (a != b) {
        HudError("%s :: %s mismatch (%s | %s)", name, subname, a.c_str(),
                 b.c_str());
    }
}

static void VerifyVal(double a, double b, const char* name,
                      const char* subname) {
    if (FuzzyEquals(a, b) == false) {
        HudError("%s :: %s mismatch (%f | %f)", name, subname, a, b);
    }
}

template <typename T>
void VerifyBasic(const T& a, const T& b, const char* name,
                 const char* subname) {
    if (!(a == b)) {
        HudError("%s :: %s mismatch", name, subname);
    }
}

template <typename T, typename Equal>
void VerifyValue(const T& a, const T& b, const char* name, const char* subname,
                 Equal eq) {
    if (!eq(a, b)) {
        HudError("%s :: %s mismatch", name, subname);
    }
}

#define CHECK(memberName) Check(#memberName, a.memberName, b.memberName)

#define CHECK_SELF Check("value", a, b)

#define VERIFY_STR(memberName) \
    VerifyStr(a.memberName, b.memberName, name, #memberName);

#define VERIFY_VAL(memberName) \
    VerifyVal(a.memberName, b.memberName, name, #memberName);

#define VERIFY_BASIC(memberName) \
    VerifyBasic(a.memberName, b.memberName, name, #memberName);

#define VERIFY_COMPLEX(memberName, memberType) \
        VerifyValue(a.memberName, b.memberName, name, #memberName, [](const memberType& a, const memberType& b)

#define VERIFY_VECTOR(memberName, memberType) \
        VerifyVector(a.memberName, b.memberName, name, #memberName, [](const memberType& a, const memberType& b)

static bool VerifyBgChange(const BgChange& a, const BgChange& b) {
    return CHECK(effect) && CHECK(file) && CHECK(file2) && CHECK(transition) &&
           CHECK(startBeat) && CHECK(rate);
}

static void VerifyTempo(const char* name, const Tempo& a, const Tempo& b) {
    VERIFY_VAL(offset);

    VERIFY_VECTOR(bpms, BpmChange) { return CHECK(row) && CHECK(val); });
    VERIFY_VECTOR(stops, Stop) { return CHECK(row) && CHECK(len); });
    VERIFY_VECTOR(delays, Delay) { return CHECK(row) && CHECK(len); });
    VERIFY_VECTOR(warps, Warp) { return CHECK(row) && CHECK(len); });
    VERIFY_VECTOR(speeds, Speed) {
        return CHECK(row) && CHECK(delay) && CHECK(ratio) && CHECK(unit);
    });
    VERIFY_VECTOR(scrolls, Scroll) {
        return CHECK(row) && CHECK(ratio) && CHECK(ratio);
    });
    VERIFY_VECTOR(tickCounts, TickCount) { return CHECK(row) && CHECK(ticks); });
    VERIFY_VECTOR(timeSignatures, TimeSignature) {
        return CHECK(row) && CHECK(num) && CHECK(denom);
    });
}

static void VerifyShared(const char* name, const Shared& a, const Shared& b) {
    VerifyTempo(name, a, b);

    VERIFY_VECTOR(labels, Label) { return CHECK(row) && CHECK(str); });
    VERIFY_VECTOR(attacks, Attack) {
        return CHECK(duration) && CHECK(mods) && CHECK(time) && CHECK(unit);
    });
    VERIFY_VECTOR(keysounds, std::string) { return CHECK_SELF; });
    VERIFY_VECTOR(combos, Combo) {
        return CHECK(row) && CHECK(hitCombo) && CHECK(missCombo);
    });
    VERIFY_VECTOR(fakes, Fake) { return CHECK(row) && CHECK(beats); });

    VERIFY_VAL(displayBpmType);
    VERIFY_COMPLEX(displayBpmRange, BpmRange) {
        return CHECK(min) && CHECK(max);
    });

    VERIFY_VECTOR(misc, Property) { return CHECK(tag) && CHECK(val); });
}

static void VerifyMetadata(const char* name, const Metadata& a,
                           const Metadata& b) {
    VERIFY_STR(title);
    VERIFY_STR(titleTr);
    VERIFY_STR(subtitle);
    VERIFY_STR(subtitleTr);
    VERIFY_STR(artist);
    VERIFY_STR(artistTr);
    VERIFY_STR(genre);
    VERIFY_STR(credit);

    VERIFY_STR(music);
    VERIFY_STR(banner);
    VERIFY_STR(background);
    VERIFY_STR(cdTitle);
    VERIFY_STR(lyricsPath);

    VERIFY_VECTOR(fgChanges, BgChange) { return VerifyBgChange(a, b); });
    VERIFY_VECTOR(bgChanges[0], BgChange) { return VerifyBgChange(a, b); });
    VERIFY_VECTOR(bgChanges[1], BgChange) { return VerifyBgChange(a, b); });

    VERIFY_VAL(previewStart);
    VERIFY_VAL(previewLength);

    VERIFY_VAL(isSelectable);
}

static void VerifyChart(const Chart& a, const Chart& b) {
    std::string nameStr = a.description();
    const char* name = nameStr.c_str();

    VERIFY_BASIC(style);
    VERIFY_STR(artist);
    VERIFY_VAL(difficulty);
    VERIFY_VAL(meter);
    VERIFY_VECTOR(radar, double) { return CHECK_SELF; });

    if (a.shared && b.shared) {
        VerifyShared(name, *a.shared, *b.shared);
    } else if (a.shared && !b.shared) {
        HudError("chart shared mismatch, a has it, b does not");
    } else if (b.shared && !a.shared) {
        HudError("chart shared mismatch, b has it, a does not");
    }

    VERIFY_VECTOR(notes, Note) {
        return CHECK(row) && CHECK(endrow) && CHECK(col) && CHECK(player) &&
               CHECK(type);
    });
}

// ================================================================================================
// Main function.

std::string VerifySaveLoadIdentity(const Simfile& simfile) {
    std::string error;
    Simfile a = simfile, b;

    a.dir = "verify";
    File::CreateFolder(a.dir);
    a.dir += "\\";
    a.file += ".verify";

    std::string path;
    switch (simfile.loadFormat) {
        case LOAD_SSC:
            path = a.dir + a.file + ".ssc";
            SaveSimfile(a, SAVE_SSC);
            break;
        case LOAD_DWI:
        case LOAD_SM:
            path = a.dir + a.file + ".sm";
            SaveSimfile(a, SAVE_SM);
            break;
        case LOAD_OSU:
            if (simfile.charts.size()) {
                path = a.dir + a.file + " [";
                Str::append(path, ToString(simfile.charts[0]->difficulty));
                Str::append(path, "].osu");
            } else {
                path = a.dir + a.file + ".osu";
            }
            SaveSimfile(a, SAVE_OSU);
            break;
    };

    if (!LoadSimfile(b, path)) {
        HudError("TestSaveLoadIdentity failed: could not load file");
    } else {
        VerifyShared("Simfile", a, b);
        VerifyMetadata("Simfile", a, b);
        if (a.charts.size() != a.charts.size()) {
            HudError("charts mismatch in size: %i and %i", a.charts.size(),
                     b.charts.size());
        } else {
            for (int i = 0; i < a.charts.size(); ++i) {
                VerifyChart(*a.charts[i], *b.charts[i]);
            }
        }
    }

    a.charts.clear();

    HudInfo("Verification complete!");

    return path;
}

#endif  // ENABLE_TESTING

};  // namespace Vortex
