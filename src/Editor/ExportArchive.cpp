#include <Editor/ExportArchive.h>

#include <Core/Core.h>
#include <Core/StringUtils.h>

#include <System/File.h>
#include <System/System.h>
#include <System/Zip.h>

#include <Managers/SimfileMan.h>

#include <SDL3/SDL_dialog.h>

#include <string>
#include <vector>

namespace Vortex {
namespace {

struct ArchiveFormat {
    const char* extension;
    const char* filterName;
    const char* filterPattern;
    SimFormat chartFormat;
    const char* chartExtension;

    // osu! and Quaver read a flat archive; StepMania expects the song folder
    // inside it, which is how a pack is put together.
    bool inFolder;

    // osu! and Quaver know only the background, so a banner or a CD title
    // would be dead weight in their archives.
    bool allArtwork;
};

const ArchiveFormat OSZ = {
    ".osz", "osu! beatmap (*.osz)", "osz", SIM_OSU, ".osu", false, false};
const ArchiveFormat SM = {
    ".zip", "StepMania (*.zip)", "zip", SIM_SM, ".sm", true, true};
const ArchiveFormat SSC = {
    ".zip", "StepMania 5 (*.zip)", "zip", SIM_SSC, ".ssc", true, true};

// Strips the characters a filename cannot hold.
std::string FileSafe(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') {
            continue;
        }
        out.push_back(c);
    }
    while (out.length() && out.back() == ' ') out.pop_back();
    return out;
}

// "Artist - Title", the way both games name a downloaded archive. The
// romanised names are preferred, the same way the exporters prefer them.
std::string ArchiveName(const Simfile* sim) {
    const std::string& artist =
        sim->artistTr.length() ? sim->artistTr : sim->artist;
    const std::string& title =
        sim->titleTr.length() ? sim->titleTr : sim->title;

    std::string out = FileSafe(artist + " - " + title);
    if (out == " - ") out = "song";
    return out;
}

// A folder of our own, so that the chart files written for the archive do not
// land in the song folder beside the ones the user keeps there.
fs::path MakeTempDir() {
    std::error_code ec;
    fs::path dir = fs::temp_directory_path(ec);
    if (ec) return fs::path();

    dir.append("arrowvortex-export");
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    return ec ? fs::path() : dir;
}

// The artwork and audio the song refers to, skipping what is not set and what
// is not there. The same file can be named twice - a background used as the
// banner as well - so each one is added once.
void CollectAssets(const Simfile* sim, std::vector<std::string>& out,
                   bool allArtwork) {
    const std::string* fields[] = {&sim->music, &sim->background, &sim->banner,
                                   &sim->cdTitle};
    int numFields = allArtwork ? 4 : 2;

    for (int i = 0; i < numFields; ++i) {
        const std::string* field = fields[i];
        if (field->empty()) continue;
        if (std::find(out.begin(), out.end(), *field) != out.end()) continue;

        fs::path path = utf8ToPath(sim->dir);
        path.append(stringToUtf8(*field));
        if (fs::exists(path)) out.push_back(*field);
    }
}

const ArchiveFormat& FormatFor(SimFormat format) {
    return (format == SIM_SM) ? SM : (format == SIM_SSC) ? SSC : OSZ;
}

// The folder the song lives in, which is what a StepMania archive is named
// after and what it holds.
std::string SongFolder(const Simfile* sim) {
    std::string folder = pathToUtf8(utf8ToPath(sim->dir).filename());
    return folder.empty() ? ArchiveName(sim) : folder;
}

};  // anonymous namespace

std::string SuggestedArchiveName(SimFormat format) {
    auto sim = gSimfile->get();
    if (!sim) return std::string();

    const ArchiveFormat& fmt = FormatFor(format);
    return (fmt.inFolder ? SongFolder(sim) : ArchiveName(sim)) + fmt.extension;
}

void ExportArchive(SimFormat format, const std::string& name) {
    if (!gSimfile->isOpen()) {
        HudError("No simfile is open");
        return;
    }

    const ArchiveFormat& fmt = FormatFor(format);
    auto sim = gSimfile->get();

    // Ask where the archive goes, with the name the games use.
    SDL_DialogFileFilter filters[] = {
        {fmt.filterName, fmt.filterPattern},
        {"All Files (*.*)", "*"},
    };
    int filterIndex = 0;
    std::string folder = SongFolder(sim);
    fs::path path = gSystem->saveFileDlg("Export", filters, 2, &filterIndex,
                                         utf8ToPath(name));
    if (path.empty()) return;
    if (!path.has_extension()) path.concat(fmt.extension);

    // Write the chart files somewhere of our own first.
    fs::path temp = MakeTempDir();
    if (temp.empty()) {
        HudError("Could not create a folder for the export");
        return;
    }
    if (!gSimfile->exportTo(pathToUtf8(temp), fmt.chartFormat)) {
        HudError("Could not write the chart files");
        return;
    }

    ZipWriter zip;
    if (!zip.open(path)) {
        HudError("Could not create %s", pathToUtf8(path.filename()).c_str());
        return;
    }

    std::string prefix;
    if (fmt.inFolder) {
        prefix = FileSafe(folder) + "/";
        zip.addFolder(prefix);
    }

    int numCharts = 0;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(temp, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != fmt.chartExtension) continue;

        if (zip.addFile(entry.path(),
                        prefix + pathToUtf8(entry.path().filename()))) {
            ++numCharts;
        }
    }

    std::vector<std::string> assets;
    CollectAssets(sim, assets, fmt.allArtwork);
    for (auto& asset : assets) {
        fs::path source = utf8ToPath(sim->dir);
        source.append(stringToUtf8(asset));
        if (!zip.addFile(source, prefix + asset)) {
            HudWarning("Could not add %s to the archive", asset.c_str());
        }
    }

    bool ok = zip.close();
    fs::remove_all(temp, ec);

    if (!ok) {
        HudError("Could not finish %s", pathToUtf8(path.filename()).c_str());
        return;
    }

    HudInfo("Exported %s: %i charts, %i other files",
            pathToUtf8(path.filename()).c_str(), numCharts,
            static_cast<int>(assets.size()));
}

};  // namespace Vortex
