#include <Editor/Editor.h>

#include <map>
#include <format>

#include <Core/Xmr.h>
#include <Core/Gui.h>
#include <Core/Draw.h>
#include <Core/Shader.h>
#include <Core/StringUtils.h>

#include <System/System.h>
#include <System/File.h>
#include <System/Debug.h>

#include <Editor/Music.h>
#include <Editor/Menubar.h>
#include <Editor/Action.h>
#include <Editor/Shortcuts.h>

#include <Editor/View.h>
#include <Editor/Notefield.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/TempoBoxes.h>
#include <Editor/Waveform.h>
#include <Editor/Editing.h>
#include <Editor/Selection.h>
#include <Editor/Minimap.h>
#include <Editor/TextOverlay.h>
#include <Editor/Statusbar.h>
#include <Editor/History.h>
#include <Editor/StreamGenerator.h>

#include <Managers/StyleMan.h>
#include <Managers/TempoMan.h>
#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>
#include <Managers/NoteskinMan.h>

#include <Dialogs/SongProperties.h>
#include <Dialogs/ChartList.h>
#include <Dialogs/ChartProperties.h>
#include <Dialogs/NewChart.h>
#include <Dialogs/AdjustTempo.h>
#include <Dialogs/AdjustTempoSM5.h>
#include <Dialogs/AdjustSync.h>
#include <Dialogs/DancingBot.h>
#include <Dialogs/TempoBreakdown.h>
#include <Dialogs/LabelBreakdown.h>
#include <Dialogs/GenerateNotes.h>
#include <Dialogs/WaveformSettings.h>
#include <Dialogs/Zoom.h>
#include <Dialogs/CustomSnap.h>
#include <Dialogs/PreviewSettings.h>
#include <Dialogs/EditSegment.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include <SDL3/SDL.h>

namespace Vortex {

extern std::string VerifySaveLoadIdentity(const Simfile& simfile);

namespace {

struct DialogEntry {
    EditorDialog* ptr;
    bool requestOpen;
};

#define LOAD_FILTERS_COUNT 10
static SDL_DialogFileFilter loadFilters[] = {
    {"Supported Media (*.sm, *.ssc, *.dwi, *.osu, *.qua, *.ogg, *.mp3, *.wav)",
     "sm;ssc;dwi;osu;qua;ogg;mp3;wav"},
    {"Stepmania/ITG (*.sm)", "sm"},
    {"Stepmania 5 (*.ssc)", "ssc"},
    {"Dance With Intensity (*.dwi)", "dwi"},
    {"Osu!mania (*.osu)", "osu"},
    {"Quaver (*.qua)", "qua"},
    {"Ogg Vorbis (*.ogg)", "ogg"},
    {"MP3 Audio (*.mp3)", "mp3"},
    {"Waveform (*.wav)", "wav"},
    {"All Files (*.*)", "*"},
};

#define SAVE_FILTERS_COUNT 6
static SDL_DialogFileFilter saveFilters[] = {
    {"Stepmania/ITG (*.sm)", "sm"}, {"Stepmania 5 (*.ssc)", "ssc"},
    {"Osu!mania (*.osu)", "osu"},   {"Dance With Intensity (*.dwi)", "dwi"},
    {"Quaver (*.qua)", "qua"},      {"All Files (*.*)", "*"},
};
struct DialogSegment {
    Segment::Type type;
    int row;
    bool requestOpen;
};

struct DialogFocus {
    int dialogId;
    const char* name;
    bool requestFocus = false;
};

static const size_t MAX_RECENT_FILES = 10;

static std::string ClipboardGet() { return gSystem->getClipboardText(); }

static void ClipboardSet(std::string text) { gSystem->setClipboardText(text); }

static const char* ToString(BackgroundStyle style) {
    if (style == BG_STYLE_LETTERBOX) return "letterbox";
    if (style == BG_STYLE_CROP) return "crop";
    return "stretch";
}

static BackgroundStyle ToBackgroundStyle(const std::string& str) {
    if (str == "letterbox") return BG_STYLE_LETTERBOX;
    if (str == "crop") return BG_STYLE_CROP;
    return BG_STYLE_STRETCH;
}

static const char* ToString(SimFormat format) {
    if (format == SIM_SM) return "sm";
    if (format == SIM_SSC) return "ssc";
    if (format == SIM_OSU) return "osu";
    return "none";
}

static SimFormat ToSimFormat(const std::string& str) {
    if (str == "sm") return SIM_SM;
    if (str == "ssc") return SIM_SSC;
    if (str == "osu") return SIM_OSU;
    return SIM_NONE;
}

static std::string getSettingsDir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] == '/') return std::string(xdg) + "/arrowvortex/";
    const char* home = std::getenv("HOME");
    if (home) return std::string(home) + "/.config/arrowvortex/";
    return "settings/";
}

static void ensureSettingsDirExists() {
    std::error_code ec;
    fs::create_directories(utf8ToPath(getSettingsDir()), ec);
}

// ================================================================================================
// EditorImpl :: member data.

struct EditorImpl : public Editor, public InputHandler {
    GuiContext* gui_;
    DialogEntry myDialogs[NUM_DIALOG_IDS];
    DialogFocus myDialogFocus;
    DialogSegment mySegmentEditor;
    int myChanges;
    Texture myLogo;
    std::vector<std::string> myRecentFiles;

    int myFontSize;
    std::string myFontPath;

    bool myUseMultithreading;
    bool myUseVerticalSync;

    BackgroundStyle myBackgroundStyle;
    std::vector<SimFormat> myDefaultSaveFormat;

    // ================================================================================================
    // EditorImpl :: constructor and destructor.

    ~EditorImpl() = default;

    EditorImpl() {
        gSystem->setWindowTitle("ArrowVortex");

        for (auto& dialog : myDialogs) {
            dialog.ptr = nullptr;
            dialog.requestOpen = false;
        }
        mySegmentEditor.requestOpen = false;
        myDialogFocus.requestFocus = false;

        gui_ = nullptr;
        myChanges = 0;

        myUseMultithreading = true;
        myUseVerticalSync = true;

        myBackgroundStyle = BG_STYLE_STRETCH;
        myDefaultSaveFormat = {SIM_SM};

        myFontPath = "assets/NotoSansJP-Medium.ttf";
        myFontSize = 13;
    }

    // ================================================================================================
    // EditorImpl :: initialization / shutdown.

    void init() {
        ensureSettingsDirExists();
        loadRecentFiles();

        // Load the editor settings.
        XmrDoc settings;
        settings.loadFile(fs::path(getSettingsDir() + "settings.txt"));
        loadSettings(settings);

        // Disable v-sync if requested.
        if (!myUseVerticalSync) gSystem->disableVsync();

        // Initialize the drawing / gui system.
        GuiMain::init();
        GuiMain::setClipboardFunctions(ClipboardGet, ClipboardSet);

        gui_ = GuiContext::create();

        // Initialize the default text style.
        TextStyle text;
        text.font = Font(myFontPath.c_str(), Text::HINT_AUTO);
        text.fontSize = myFontSize;
        text.textColor = Colors::white;
        text.shadowColor = RGBAtoColor32(0, 0, 0, 128);
        text.makeDefault();

        // Create the text overlay, so other editor components can show HUD
        // messages.
        TextOverlay::create();

        // Create the history, because simfile components have to register their
        // callbacks.
        History::create();

        // Create the simfile components.
        StyleMan::create();
        NoteskinMan::create(settings);
        SimfileMan::create();
        MetadataMan::create();
        TempoMan::create();
        ChartMan::create();
        NotesMan::create();

        // Create the editor components.
        Shortcuts::create();
        Music::create(settings);
        Selection::create(settings);
        Editing::create(settings);
        View::create(settings);
        Notefield::create(settings);
        NotefieldPreview::create(settings);
        TempoBoxes::create(settings);
        Waveform::create(settings);
        Statusbar::create(settings);
        Minimap::create();
        Menubar::create();
        gSystem->createMenu();

        // Load the editor logo.
        myLogo = Texture("assets/arrow vortex logo.png", false, Texture::ALPHA);

        // Update the menubar with the initial settings.
        gMenubar->update(Menubar::ALL_PROPERTIES);

        // Open the saved pinned dialogs.
        openPinnedDialogs(settings);
    }

    void shutdown() {
        saveRecentFiles();

        // Save the editor settings.
        XmrDoc settings;
        saveGeneralSettings(settings);
        gStatusbar->saveSettings(settings);
        gSelection->saveSettings(settings);
        gEditing->saveSettings(settings);
        gWaveform->saveSettings(settings);
        gNotefield->saveSettings(settings);
        gNotefieldPreview->saveSettings(settings);
        gTempoBoxes->saveSettings(settings);
        gView->saveSettings(settings);
        gMusic->saveSettings(settings);
        gNoteskin->saveSettings(settings);
        saveDialogSettings(settings);

        // Destroy the gui context first, because some dialogs refer to editor
        // components.
        delete gui_;

        // Destroy the editor components.
        Minimap::destroy();
        Statusbar::destroy();
        Editing::destroy();
        Waveform::destroy();
        TempoBoxes::destroy();
        Notefield::destroy();
        NotefieldPreview::destroy();
        View::destroy();
        Selection::destroy();
        Music::destroy();
        History::destroy();
        Menubar::destroy();
        Shortcuts::destroy();
        TextOverlay::destroy();

        // Destroy the simfile components.
        NotesMan::destroy();
        ChartMan::destroy();
        TempoMan::destroy();
        MetadataMan::destroy();
        SimfileMan::destroy();
        NoteskinMan::destroy();
        StyleMan::destroy();

        // Destroy goo last, because some editor components use goo graphics
        // objects.
        GuiMain::shutdown();

        // Export the editor settings.
        XmrSaveSettings xmrSaveSettings;
        settings.saveFile((getSettingsDir() + "settings.txt").c_str(),
                          xmrSaveSettings);
    }

    // ================================================================================================
    // EditorImpl :: load / save settings.

    void loadSettings(XmrDoc& settings) {
        XmrNode* general = settings.child("general");
        if (general) {
            general->get("useMultithreading", &myUseMultithreading);
            general->get("useVerticalSync", &myUseVerticalSync);

            std::vector<SimFormat> saveFormats;
            auto saveFormat = general->attrib("defaultSaveFormat");
            if (saveFormat) {
                for (int i = 0; i < saveFormat->numValues; ++i) {
                    auto toFormat = ToSimFormat(saveFormat->values[i]);
                    if (toFormat != SIM_NONE) {
                        saveFormats.push_back(toFormat);
                    }
                }
            }
            if (saveFormats.empty()) saveFormats.push_back(SIM_SM);
            myDefaultSaveFormat = saveFormats;
        }

        XmrNode* view = settings.child("view");
        if (view) {
            const char* bgStyle = view->get("backgroundStyle");
            if (bgStyle) myBackgroundStyle = ToBackgroundStyle(bgStyle);
        }

        XmrNode* interface = settings.child("interface");
        if (interface) {
            interface->get("fontSize", &myFontSize);

            fs::path path = fs::path(interface->get("fontPath"));
            if (std::ifstream testPath(path.c_str());
                !path.empty() && testPath.good())
                myFontPath = pathToUtf8(path);

            bool winMax = false;
            interface->get("windowMaximized", &winMax);
            if (winMax)
                gSystem->setWindowState(true);
            else {
                int winSize[2] = {0, 0};
                if (interface->get("windowSize", winSize, 2)) {
                    vec2i size = {winSize[0], winSize[1]};
                    gSystem->setWindowSize(size);
                }
            }
        }
    }

    void saveGeneralSettings(XmrNode& settings) {
        XmrNode* general = settings.addChild("general");

        general->addAttrib("useMultithreading", myUseMultithreading);
        general->addAttrib("useVerticalSync", myUseVerticalSync);

        std::vector<const char*> formats;
        for (SimFormat f : myDefaultSaveFormat) formats.push_back(ToString(f));
        general->addAttrib("defaultSaveFormat", formats.data(), formats.size());

        XmrNode* view = settings.addChild("view");

        view->addAttrib("backgroundStyle", ToString(myBackgroundStyle));

        XmrNode* interface = settings.addChild("interface");

        interface->addAttrib("fontPath", myFontPath.c_str());
        interface->addAttrib("fontSize", static_cast<long>(myFontSize));

        bool windowState = gSystem->getWindowState();
        if (windowState) {
            interface->addAttrib("windowMaximized", true);
        } else {
            vec2i ws = gSystem->getWindowSize();
            long windowSize[] = {ws.x, ws.y};
            interface->addAttrib("windowSize", windowSize, 2);
        }
    }

    void saveDialogSettings(XmrNode& settings) {
        XmrNode* dialogs = settings.addChild("dialogs");

        for (int id = 0; id < NUM_DIALOG_IDS; ++id) {
            auto dialog = myDialogs[id].ptr;
            if (dialog && dialog->isPinned()) {
                auto r = dialog->getInnerRect();
                if (r.w > 0 && r.h > 0) {
                    auto name =
                        EditorDialog::getName(static_cast<DialogId>(id));
                    XmrNode* node = dialogs->addChild(name);
                    long vals[] = {r.x, r.y, r.w, r.h};
                    node->addAttrib("rect", vals, 4);
                }
            }
        }
    }

    // ================================================================================================
    // EditorImpl :: recent files.

    void loadRecentFiles() {
        bool success;
        myRecentFiles = File::getLines(
            fs::path(getSettingsDir() + "recent files.txt"), &success);
        std::erase(myRecentFiles, "");
        myRecentFiles.resize(std::min(MAX_RECENT_FILES, myRecentFiles.size()));
    }

    void saveRecentFiles() {
        std::ofstream out((getSettingsDir() + "recent files.txt").c_str());
        if (out.good()) {
            for (int i = 0; i < myRecentFiles.size(); ++i) {
                auto& file = myRecentFiles[i];
                out.write(file.c_str(), file.length());
                if (i != myRecentFiles.size() - 1) out << '\n';
            }
        }
    }

    // ================================================================================================
    // EditorImpl :: saving and loading of simfiles.

    static fs::path findSimfile(fs::path path, bool ignoreAudio) {
        fs::path out;

        // Make a list of loadable extensions, from high priority to low
        // priority.
        static const char* extList[] = {".ssc", ".sm",  ".dwi", ".osu",
                                        ".qua", ".ogg", ".mp3", ".wav"};
        const char** extEnd = extList + (ignoreAudio ? 5 : 8);

        // Check if the path is a directory.
        if (fs::is_directory(path)) {
            // If so, look for loadable files in the given directory.
            auto curPriority = extEnd;
            for (auto& file : File::findFiles(path, false)) {
                std::string ext(pathToUtf8(file.extension()));
                Str::toLower(ext);
                auto priority = std::find(extList, extEnd, ext);
                if (priority != extEnd && priority < curPriority) {
                    curPriority = priority;
                    out = file;
                }
            }
            if (out.empty()) {
                HudError("%s", "Could not find any simfiles or music.");
            }
        } else {
            out = path;
        }

        return out;
    }

    bool closeSimfile() override {
        // Check if a simfile is currently open.
        if (gSimfile->isClosed()) return true;

        // Check if the user wants to discard unsaved changes.
        if (gHistory->hasUnsavedChanges()) {
            std::string title = gSimfile->get()->title;
            if (title.empty()) title = "the current file";
            std::string msg =
                std::format("Do you want to save changes to {}?", title);

            int res = gSystem->showMessageDlg(
                "ArrowVortex", msg, System::T_YES_NO_CANCEL, System::I_NONE);
            if (res == System::R_CANCEL) {
                return false;
            } else if (res == System::R_YES) {
                gEditor->saveSimfile(false);
            }
        }

        // Close the simfile and reset the editor state.
        gSimfile->close();
        gMenubar->update(Menubar::OPEN_FILE);

        gView->setCursorTime(0.0);

        myLogo = Texture("assets/arrow vortex logo.png");

        return true;
    }

    bool openSimfile() override {
        return openSimfile(gSystem->openFileDlg(
            "Open file", loadFilters, LOAD_FILTERS_COUNT, std::string()));
    }

    bool openSimfile(fs::path path) override {
        bool result = false;
        if (!path.empty() && closeSimfile()) {
            if (gSimfile->load(path)) {
                addToRecentfiles(path);
                result = true;
            }
            gView->setCursorTime(0.0);
        }
        gMenubar->update(Menubar::OPEN_FILE);
        return result;
    }

    bool openSimfile(int recentFileIndex) override {
        if (recentFileIndex >= 0 || recentFileIndex < myRecentFiles.size()) {
            return openSimfile(utf8ToPath(myRecentFiles[recentFileIndex]));
        }
        return false;
    }

    bool openNextSimfile(bool iterateForward) override {
        // Check if a simfile is currently open.
        if (gSimfile->isClosed()) return false;

        // Make a list of all simfiles in the current pack.
        fs::path this_dir = utf8ToPath(gSimfile->getDir());
        fs::path packDir = this_dir.parent_path();
        auto songDirs = File::findDirs(packDir, false);

        // Find the current simfile.
        int index = -1;
        for (int i = 0; i < songDirs.size(); ++i) {
            if (fs::equivalent(songDirs[i], this_dir)) {
                index = i;
            }
        }
        if (index == -1) {
            HudError("Could not locate the current simfile.");
            return false;
        }

        // Find the previous/next simfile with a different directory.
        fs::path path;
        int start_index = index;
        if (iterateForward) {
            while (++index != start_index) {
                if (index == songDirs.size()) {
                    HudInfo("Looping to the first simfile.");
                    index = -1;
                    continue;
                }
                path = findSimfile(songDirs[index], true);
                if (!path.empty()) break;
            }
        } else {
            while (--index != start_index) {
                if (index < 0) {
                    HudInfo("Looping to the last simfile.");
                    index = songDirs.size();
                    continue;
                }
                path = findSimfile(songDirs[index], true);
                if (!path.empty()) break;
            }
        }
        if (index == start_index) {
            HudInfo("No valid simfiles found.");
            return false;
        } else
            return openSimfile(path);
    }

    bool saveSimfile(bool showSaveAsDialog) override {
        // Check if a simfile is currently open.
        if (gSimfile->isClosed()) return true;

        std::string dir = gSimfile->getDir();
        std::string file = gSimfile->getFile();

        // Save As is single file.
        SimFormat saveFmt = myDefaultSaveFormat[0];
        fs::path save_path = utf8ToPath(dir);
        save_path.append(stringToUtf8(file));

        // If the path is empty, ask a path from the user.
        if (save_path.empty() || showSaveAsDialog) {
            // Set the default filter index based on the save format.
            // SDL doesn't support this currently.
            int filterIndex;
            switch (saveFmt) {
                default:
                case SIM_SM:
                    filterIndex = 0;
                    break;
                case SIM_SSC:
                    filterIndex = 1;
                    break;
                case SIM_OSU:
                    filterIndex = 2;
                    break;
                case SIM_DWI:
                    filterIndex = 3;
                    break;
                case SIM_QUA:
                    filterIndex = 4;
                    break;
            };

            // Show the save file dialog.
            save_path = gSystem->saveFileDlg("Save file", saveFilters,
                                             SAVE_FILTERS_COUNT, &filterIndex,
                                             fs::path());
            dir = pathToUtf8(save_path.parent_path());
            file = pathToUtf8(save_path.filename());
            auto ext = pathToUtf8(save_path.extension());

            if (save_path.empty()) return false;

            // Update the save format based on the selected filter index.
            // SDL3 returns 0-based filter indices (getFilterIndex subtracts 1).
            switch (filterIndex) {
                case 0:
                    saveFmt = SIM_SM;
                    break;
                case 1:
                    saveFmt = SIM_SSC;
                    break;
                case 2:
                    saveFmt = SIM_OSU;
                    break;
                case 3:
                    saveFmt = SIM_DWI;
                    break;
                case 4:
                    saveFmt = SIM_QUA;
                    break;
                default:
                    if (ext == ".ssc") {
                        saveFmt = SIM_SSC;
                    } else if (ext == ".osu") {
                        saveFmt = SIM_OSU;
                    } else if (ext == ".dwi") {
                        saveFmt = SIM_DWI;
                    } else if (ext == ".qua") {
                        saveFmt = SIM_QUA;
                    } else {
                        saveFmt = SIM_SM;
                    }
                    break;
            };

            // Save the simfile.
            if (!gSimfile->save(dir, file, saveFmt)) {
                HudError("Could not save %s", file.c_str());
            }

            return true;
        }

        // Saving multiple formats.
        std::vector<SimFormat> save = myDefaultSaveFormat;
        SimFormat fmt = gSimfile->get()->format;
        if (fmt == SIM_NONE) {
            fmt = save[0];
        }
        save.erase(std::remove(save.begin(), save.end(), fmt), save.end());
        save.insert(save.begin(), fmt);  // Give priority to the load format.

        for (auto saveFmt : save) {
            if (!gSimfile->save(dir, file, saveFmt)) {
                HudError("Could not save %s", file.c_str());
            }
        }

        // Signal to the edit history that the current state is the saved state.
        gHistory->onFileSaved();

        return true;
    }

    // ================================================================================================
    // EditorImpl :: recent files.

    void addToRecentfiles(fs::path path) {
        std::string spath = pathToUtf8(path);
        std::erase(myRecentFiles, spath);
        myRecentFiles.insert(myRecentFiles.begin(), 1, spath);
        myRecentFiles.resize(std::min(MAX_RECENT_FILES, myRecentFiles.size()));
        gMenubar->update(Menubar::RECENT_FILES);
    }

    void clearRecentFiles() override {
        myRecentFiles.clear();
        gMenubar->update(Menubar::RECENT_FILES);
    }

    int getNumRecentFiles() override { return myRecentFiles.size(); }

    const std::string& getRecentFile(int index) override {
        return myRecentFiles[index];
    }

    // ================================================================================================
    // EditorImpl :: dialog management.

    void openPinnedDialogs(XmrDoc& settings) {
        XmrNode* dialogs = settings.child("dialogs");
        if (dialogs) {
            for (int id = 0; id < NUM_DIALOG_IDS; ++id) {
                auto name = EditorDialog::getName(static_cast<DialogId>(id));
                XmrNode* node = dialogs->child(name);
                int r[4] = {0, 0, 0, 0};
                if (node && node->get("rect", r, 4)) {
                    recti rect = {r[0], r[1], r[2], r[3]};
                    handleDialogOpening(static_cast<DialogId>(id), rect);
                }
            }
        }
    }

    void openDialog(int dialogId) override {
        myDialogs[dialogId].requestOpen = true;
    }

    void openSegmentDialog(Segment::Type type, int row) override {
        auto& entry = myDialogs[DIALOG_EDIT_SEGMENT];
        if (entry.ptr) entry.ptr->requestClose();
        entry.requestOpen = true;
        mySegmentEditor.type = type;
        mySegmentEditor.row = row;
        mySegmentEditor.requestOpen = true;
    }

    void setDialogFocus(int dialogId, const char* name) override {
        myDialogFocus.dialogId = dialogId;
        myDialogFocus.name = name;
        myDialogFocus.requestFocus = true;
    }

    void handleDialogOpens() {
        for (int id = 0; id < NUM_DIALOG_IDS; ++id) {
            if (myDialogs[id].requestOpen) {
                handleDialogOpening(static_cast<DialogId>(id), {0, 0, 0, 0});
            }
        }
    }

    void handleDialogFocus() {
        if (myDialogFocus.requestFocus) {
            auto dlg = myDialogs[myDialogFocus.dialogId].ptr;
            if (dlg) dlg->setFocus(myDialogFocus.name);
            myDialogFocus.requestFocus = false;
        }
    }

    void handleSegmentEditor() {
        auto& entry = myDialogs[DIALOG_EDIT_SEGMENT];
        if (!entry.ptr) return;

        auto dlg = static_cast<DialogEditSegment*>(entry.ptr);

        // Set Type
        if (mySegmentEditor.requestOpen) {
            dlg->setSegment(mySegmentEditor.type, mySegmentEditor.row);
            mySegmentEditor.requestOpen = false;
        }

        // Set Position
        auto meta = Segment::meta[mySegmentEditor.type];
        auto coords = gView->getNotefieldCoords();
        int offset =
            gTempoBoxes->getStackWidth(meta->side, mySegmentEditor.row);
        int x = meta->side ? coords.xr + offset + 16
                           : coords.xl - offset - 10 - dlg->getFixedWidth();
        int y =
            gView->rowToY(mySegmentEditor.row) - (dlg->getFixedHeight() / 2);

        dlg->setPosition(x, y);
    }

    void handleDialogOpening(DialogId id, recti rect) {
        auto& entry = myDialogs[id];
        if (entry.ptr) {
            entry.requestOpen = false;
            return;
        }

        EditorDialog* dlg = nullptr;
        switch (id) {
            case DIALOG_ADJUST_SYNC:
                dlg = new DialogAdjustSync;
                break;
            case DIALOG_ADJUST_TEMPO:
                dlg = new DialogAdjustTempo;
                break;
            case DIALOG_ADJUST_TEMPO_SM5:
                dlg = new DialogAdjustTempoSM5;
                break;
            case DIALOG_CHART_LIST:
                dlg = new DialogChartList;
                break;
            case DIALOG_CHART_PROPERTIES:
                dlg = new DialogChartProperties;
                break;
            case DIALOG_DANCING_BOT:
                dlg = new DialogDancingBot;
                break;
            case DIALOG_GENERATE_NOTES:
                dlg = new DialogGenerateNotes;
                break;
            case DIALOG_NEW_CHART:
                dlg = new DialogNewChart;
                break;
            case DIALOG_SONG_PROPERTIES:
                dlg = new DialogSongProperties;
                break;
            case DIALOG_TEMPO_BREAKDOWN:
                dlg = new DialogTempoBreakdown;
                break;
            case DIALOG_LABEL_BREAKDOWN:
                dlg = new DialogLabelBreakdown;
                break;
            case DIALOG_WAVEFORM_SETTINGS:
                dlg = new DialogWaveformSettings;
                break;
            case DIALOG_ZOOM:
                dlg = new DialogZoom;
                break;
            case DIALOG_CUSTOM_SNAP:
                dlg = new DialogCustomSnap;
                break;
            case DIALOG_PREVIEW_SETTINGS:
                dlg = new DialogPreviewSettings;
                break;
            case DIALOG_EDIT_SEGMENT:
                dlg = new DialogEditSegment;
                break;
        };

        if (dlg == nullptr) {
            HudError("Tried to open an invalid dialog, id %d",
                     static_cast<int>(id));
            return;
        }

        dlg->setId(id);

        if (rect.w > 0 && rect.h > 0) {
            dlg->setPosition(rect.x, rect.y);
            dlg->setWidth(rect.w);
            dlg->setHeight(rect.h);
            dlg->requestPin();
        } else {
            vec2i windowSize = gSystem->getWindowSize();
            int x = windowSize.x / 2 - dlg->getOuterRect().w / 2;
            int y = windowSize.y / 2 - dlg->getOuterRect().h / 2;
            dlg->setPosition(x, y);
        }

        if (!myDialogFocus.requestFocus) setDialogFocus(id, "initial");

        entry.ptr = dlg;
        entry.requestOpen = false;
    }

    void onDialogClosed(int id) override { myDialogs[id].ptr = nullptr; }

    // ================================================================================================
    // EditorImpl :: event handling.

    void onCommandLineArgs(const std::string* args, int numArgs) override {
        if (numArgs >= 2 && args[1].length()) {
            fs::path path = utf8ToPath(gSystem->getRunDir());
            path.append(stringToUtf8(args[1]));
            openSimfile(findSimfile(path, false));
        }
    }

    void onFileDrop(FileDrop& evt) override {
        if (evt.count >= 1) {
            fs::path path = utf8ToPath(evt.files[0]);
            if (!openSimfile(findSimfile(path, false))) {
                if (canConvertAudio(pathToUtf8(path).c_str())) {
                    gMusic->startAudioConversion(path, true);
                }
            }
        }
    }

    void onMenuAction(int id) override {
        Action::perform(static_cast<Action::Type>(id));
    }

    bool onExitProgram() override {
        bool result = closeSimfile();
        if (result) {
            gSystem->terminate();
        }
        return result;
    }

    void notifyChanges() {
        if (!myChanges) return;

        gSimfile->onChanges(myChanges);
        gView->onChanges(myChanges);
        gMusic->onChanges(myChanges);
        gMinimap->onChanges(myChanges);
        gEditing->onChanges(myChanges);
        gNotefield->onChanges(myChanges);
        gTempoBoxes->onChanges(myChanges);
        gWaveform->onChanges(myChanges);

        for (auto dialog : myDialogs) {
            if (dialog.ptr) dialog.ptr->onChanges(myChanges);
        }

        myChanges = 0;
    }

    void onKeyPress(KeyPress& press) override {
        // if(press.key == Key::V)
        //{
        //	VerifySaveLoadIdentity(*gSimfile->getSimfile());
        // }
    }

    // ================================================================================================
    // EditorImpl :: misc functions.

    void reportChanges(int changes) override { myChanges |= changes; }

    void updateTitle() {
        std::string title, subtitle;
        auto meta = gSimfile->get();
        if (meta) {
            title = meta->title;
            subtitle = meta->subtitle;
        }
        bool hasChanges = gHistory->hasUnsavedChanges();
        if (title.length() || subtitle.length()) {
            if (title.length() && subtitle.length()) title = title + " ";
            title = title + subtitle;
            if (hasChanges) title = title + "*";
            title = title + " :: ArrowVortex";
        } else {
            title = "ArrowVortex";
            if (hasChanges) title = title + "*";
        }
        gSystem->setWindowTitle(title);
    }

    void drawLogo() {
        vec2i size = gSystem->getWindowSize();
        vec2i logo_size = myLogo.size();
        Draw::fill({0, 0, size.x, size.y}, RGBAtoColor32(38, 38, 38, 255));
        Draw::sprite(myLogo,
                     {size.x / 2 - logo_size.x / 2,
                      size.y / 2 - logo_size.y / 2, logo_size.x, logo_size.y},
                     RGBAtoColor32(255, 255, 255, 26));
    }

    void tick() override {
        InputEvents& events = gSystem->getEvents();
        handleInputs(events);
        notifyChanges();

        int menu_h = gMenubar->getMenubarHeight();

        vec2i windowSize = gSystem->getWindowSize();
        recti r = {0, 0, windowSize.x, windowSize.y};

        GuiMain::setViewSize(r.w, r.h + menu_h);
        GuiMain::frameStart(deltaTime.count(), events);

        gMenubar->handleInputs(events);
        gTextOverlay->handleInputs(events);

        vec2i view = gSystem->getWindowSize();

        gui_->closeDialogs();
        handleDialogOpens();
        handleSegmentEditor();

        gui_->tick({0, 0, view.x, view.y}, deltaTime.count(), events);

        handleDialogFocus();

        if (!GuiMain::isCapturingText()) {
            for (KeyPress* press = nullptr; events.next(press);) {
                Action::Type action =
                    gShortcuts->getAction(press->keyflags, press->key);
                if (action) {
                    Action::perform(action);
                    press->handled = true;
                }
            }
        }

        if (!GuiMain::isCapturingMouse()) {
            for (MouseScroll* scroll = nullptr; events.next(scroll);) {
                Action::Type action =
                    gShortcuts->getAction(scroll->keyflags, scroll->up);
                if (action) {
                    Action::perform(action);
                    scroll->handled = true;
                }
            }
        }

        gTextOverlay->tick();
        gHistory->handleInputs(events);
        gMinimap->handleInputs(events);
        gEditing->handleInputs(events);

        if (gSimfile->isOpen()) {
            gView->tick();
        }

        gSelection->handleInputs(events);

        gMusic->tick();

        if (gSimfile->isOpen()) {
            gMinimap->tick();
            gTempoBoxes->tick();
            gWaveform->tick();
        }

        updateTitle();
        notifyChanges();

        if (GuiMain::isCapturingMouse()) {
            gSystem->setCursor(GuiMain::getCursorIcon());
        } else {
            gSystem->setCursor(gSystem->getCursor());
        }

        if (gSimfile->isOpen()) {
            gNotefield->draw();
            gMinimap->draw();
            gStatusbar->draw();
        } else {
            drawLogo();
        }

        gui_->draw();

        gTextOverlay->draw();

        gMenubar->draw();

        GuiMain::frameEnd();
    }

    bool hasMultithreading() const override { return myUseMultithreading; }

    void setBackgroundStyle(int style) override {
        myBackgroundStyle = static_cast<BackgroundStyle>(style);
        gMenubar->update(Menubar::VIEW_BACKGROUND);
    }

    int getBackgroundStyle() const override { return myBackgroundStyle; }

    std::vector<SimFormat> getDefaultSaveFormats() const override {
        return myDefaultSaveFormat;
    }

    GuiContext* getGui() const override { return gui_; }

};  // EditorImpl
};  // anonymous namespace

// ================================================================================================
// Editor API.

Editor* gEditor = nullptr;

void Editor::create() {
    gEditor = new EditorImpl;
    static_cast<EditorImpl*>(gEditor)->init();
}

void Editor::destroy() {
    static_cast<EditorImpl*>(gEditor)->shutdown();
    delete static_cast<EditorImpl*>(gEditor);
    gEditor = nullptr;
}

};  // namespace Vortex
