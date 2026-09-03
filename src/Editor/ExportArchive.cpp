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
};

const ArchiveFormat OSZ = {".osz", "osu! beatmap (*.osz)", "osz", SIM_OSU,
                           ".osu"};

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

// What the file points at, skipping what is not set and what is not there.
// osu! knows a background and nothing else, so the banner and the CD title a
// simfile may carry would be dead weight in the archive. The same file can be
// named twice, so each one is added once.
void CollectAssets(const Simfile* sim, std::vector<std::string>& out) {
    const std::string* fields[] = {&sim->music, &sim->background};

    for (auto field : fields) {
        if (field->empty()) continue;
        if (std::find(out.begin(), out.end(), *field) != out.end()) continue;

        fs::path path = utf8ToPath(sim->dir);
        path.append(stringToUtf8(*field));
        if (fs::exists(path)) out.push_back(*field);
    }
}

};  // anonymous namespace

void ExportArchive() {
    if (!gSimfile->isOpen()) {
        HudError("No simfile is open");
        return;
    }

    const ArchiveFormat& fmt = OSZ;
    auto sim = gSimfile->get();

    // Ask where the archive goes, with the name the games use.
    SDL_DialogFileFilter filters[] = {
        {fmt.filterName, fmt.filterPattern},
        {"All Files (*.*)", "*"},
    };
    int filterIndex = 0;
    fs::path suggested = utf8ToPath(ArchiveName(sim) + fmt.extension);
    fs::path path =
        gSystem->saveFileDlg("Export", filters, 2, &filterIndex, suggested);
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

    int numCharts = 0;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(temp, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != fmt.chartExtension) continue;

        if (zip.addFile(entry.path(), pathToUtf8(entry.path().filename()))) {
            ++numCharts;
        }
    }

    std::vector<std::string> assets;
    CollectAssets(sim, assets);
    for (auto& asset : assets) {
        fs::path source = utf8ToPath(sim->dir);
        source.append(stringToUtf8(asset));
        if (!zip.addFile(source, asset)) {
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
