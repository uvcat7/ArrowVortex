#include <Editor/Editor.h>

#include <map>

#include <Core/Xmr.h>
#include <Core/Gui.h>
#include <Core/Draw.h>
#include <Core/Shader.h>
#include <Core/StringUtils.h>
#include <Core/WideString.h>

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

#include <algorithm>
#include <fstream>

namespace Vortex {

extern std::string VerifySaveLoadIdentity(const Simfile& simfile);

namespace {

struct DialogEntry {
    EditorDialog* ptr;
    bool requestOpen;
};

static const char loadFilters[] =
    "Supported Media (*.sm, *.ssc, *.dwi, *.osu, *.osz, *.ogg, *.mp3, "
    "*.wav)\0*.sm;*.ssc;*.dwi;*.osu;*.osz;*.ogg;*.mp3;*.wav\0"
    "Stepmania/ITG (*.sm)\0*.sm\0"
    "Stepmania 5 (*.ssc)\0*.ssc\0"
    "Dance With Intensity (*.dwi)\0*.dwi\0"
    "Osu!mania (*.osu, *.osz)\0*.osu;*.osz\0"
    "Ogg Vorbis (*.ogg)\0*.ogg\0"
    "MP3 Audio (*.mp3)\0*.mp3\0"
    "Waveform (*.wav)\0*.wav\0"
    "All Files (*.*)\0*.*\0";

static const char saveFilters[] =
    "Stepmania/ITG (*.sm)\0*.sm\0"
    "Stepmania 5 (*.ssc)\0*.ssc\0"
    "Osu!mania (*.osu)\0*.osu\0"
    "All Files (*.*)\0*.*\0";

static const int MAX_RECENT_FILES = 10;

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
    if (format == SIM_SSC) return "ssc";
    if (format == SIM_OSU) return "osu";
    return "sm";
}

static SimFormat ToSimFormat(const std::string& str) {
    if (str == "ssc") return SIM_SSC;
    if (str == "osu") return SIM_OSU;
    return SIM_SM;
}

// ================================================================================================
// EditorImpl :: member data.

struct EditorImpl : public Editor, public InputHandler {
    GuiContext* gui_;
    DialogEntry myDialogs[NUM_DIALOG_IDS];
    int myChanges;
    Texture myLogo;
    Vector<std::string> myRecentFiles;

    int myFontSize;
    std::string myFontPath;

    bool myUseMultithreading;
    bool myUseVerticalSync;

    BackgroundStyle myBackgroundStyle;
    SimFormat myDefaultSaveFormat;

    // ================================================================================================
    // EditorImpl :: constructor and destructor.

    ~EditorImpl() = default;

    EditorImpl() {
        gSystem->setWindowTitle("ArrowVortex");

        for (auto& dialog : myDialogs) {
            dialog.ptr = nullptr;
            dialog.requestOpen = false;
        }

        gui_ = nullptr;
        myChanges = 0;

        myUseMultithreading = true;
        myUseVerticalSync = true;

        myBackgroundStyle = BG_STYLE_STRETCH;
        myDefaultSaveFormat = SIM_SM;

        myFontPath = "assets/NotoSansJP-Medium.ttf";
        myFontSize = 13;
    }

    // ================================================================================================
    // EditorImpl :: initialization / shutdown.

    void init() {
        loadRecentFiles();

        // Load the editor settings.
        XmrDoc settings;
        settings.loadFile("settings/settings.txt");
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
        Selection::create();
        Editing::create(settings);
        View::create(settings);
        Notefield::create();
        NotefieldPreview::create(settings);
        TempoBoxes::create(settings);
        Waveform::create(settings);
        Statusbar::create(settings);
        Minimap::create();
        Menubar::create();

        // Load the editor logo.
        myLogo = Texture("assets/arrow vortex logo.png", false, Texture::ALPHA);

        // Update the menubar with the initial settings.
        gMenubar->update(Menubar::ALL_PROPERTIES);

        // Open the saved pinned dialogs.
        openPinnedDialogs(settings);

        // openSimfile("D:\\Development\\ArrowVortex\\test\\sm\\Alchemist
        // (Double)\\alchemist.sm"); openSimfile("D:\\Installed
        // Games\\OpenITG\\Songs\\Really Long Stuff\\90,000 Miles\\90000
        // Miles.sm"); openDialog(DIALOG_NEW_CHART);
    }

    void shutdown() {
        saveRecentFiles();

        // Save the editor settings.
        XmrDoc settings;
        saveGeneralSettings(settings);
        gStatusbar->saveSettings(settings);
        gEditing->saveSettings(settings);
        gWaveform->saveSettings(settings);
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
        settings.saveFile("settings/settings.txt", xmrSaveSettings);
    }

    // ================================================================================================
    // EditorImpl :: load / save settings.

    void loadSettings(XmrDoc& settings) {
        XmrNode* general = settings.child("general");
        if (general) {
            general->get("useMultithreading", &myUseMultithreading);
            general->get("useVerticalSync", &myUseVerticalSync);

            const char* saveFormat = general->get("defaultSaveFormat");
            if (saveFormat) myDefaultSaveFormat = ToSimFormat(saveFormat);
        }

        XmrNode* view = settings.child("view");
        if (view) {
            const char* bgStyle = view->get("backgroundStyle");
            if (bgStyle) myBackgroundStyle = ToBackgroundStyle(bgStyle);
        }

        XmrNode* interface = settings.child("interface");
        if (interface) {
            interface->get("fontSize", &myFontSize);

            const char* path = interface->get("fontPath");
            if (path == nullptr) return;

            if (std::ifstream testPath(Widen(path).str()); testPath.good())
                myFontPath = path;
        }
    }

    void saveGeneralSettings(XmrNode& settings) {
        XmrNode* general = settings.addChild("general");

        general->addAttrib("useMultithreading", myUseMultithreading);
        general->addAttrib("useVerticalSync", myUseVerticalSync);
        general->addAttrib("defaultSaveFormat", ToString(myDefaultSaveFormat));

        XmrNode* view = settings.addChild("view");

        view->addAttrib("backgroundStyle", ToString(myBackgroundStyle));

        XmrNode* interface = settings.addChild("interface");

        interface->addAttrib("fontPath", myFontPath.c_str());
        interface->addAttrib("fontSize", static_cast<long>(myFontSize));
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
        myRecentFiles = File::getLines("settings/recent files.txt", &success);
        myRecentFiles.truncate(MAX_RECENT_FILES);
    }

    void saveRecentFiles() {
        std::ofstream out("settings/recent files.txt");
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

    static std::string findSimfile(const Path& path, bool ignoreAudio) {
        std::string out;

        // Make a list of loadable extensions, from high priority to low
        // priority.
        static const char* extList[] = {"ssc", "sm",  "dwi", "osu",
                                        "ogg", "mp3", "wav"};
        const char** extEnd = extList + (ignoreAudio ? 4 : 7);

        // Check if the path is a directory.
        if (fs::is_directory(path.str)) {
            // If so, look for loadable files in the given directory.
            auto curPriority = extEnd;
            for (auto& file : File::findFiles(path, false)) {
                std::string ext = file.ext();
                Str::toLower(ext);
                auto priority = std::find(extList, extEnd, ext);
                if (priority != extEnd && priority < curPriority) {
                    curPriority = priority;
                    out = file.str;
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
            Str::fmt msg("Do you want to save changes to %1?");
            msg.arg(title);

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
        std::string filters(loadFilters, sizeof(loadFilters));
        std::string path =
            gSystem->openFileDlg("Open file", std::string(), filters);
        return openSimfile(path);
    }

    bool openSimfile(const std::string& path) override {
        bool result = false;
        if (path.length() && closeSimfile()) {
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
            return openSimfile(myRecentFiles[recentFileIndex]);
        }
        return false;
    }

    bool openNextSimfile(bool iterateForward) override {
        // Check if a simfile is currently open.
        if (gSimfile->isClosed()) return false;

        // Make a list of all simfiles in the current pack.
        Path packDir = gSimfile->getDir();
        std::string curDir = packDir.dirWithoutSlash();
        packDir.pop();
        auto songDirs = File::findDirs(packDir, false);

        // Find the current simfile.
        int index = -1;
        for (int i = 0; i < songDirs.size(); ++i) {
            if (songDirs[i].str == curDir) {
                index = i;
            }
        }
        if (index == -1) {
            HudError("Could not locate the current simfile.");
            return false;
        }

        // Find the previous/next simfile with a different directory.
        std::string path;
        if (iterateForward) {
            while (++index < songDirs.size()) {
                path = findSimfile(songDirs[index], true);
                if (path.length()) break;
            }
            if (index == songDirs.size()) {
                HudInfo("This is the last simfile.");
                return false;
            }
        } else {
            while (--index >= 0) {
                path = findSimfile(songDirs[index], true);
                if (path.length()) break;
            }
            if (index < 0) {
                HudInfo("This is the first simfile.");
                return false;
            }
        }

        // Open the simfile.
        return openSimfile(path);
    }

    bool saveSimfile(bool showSaveAsDialog) override {
        // Check if a simfile is currently open.
        if (gSimfile->isClosed()) return true;

        SimFormat saveFmt = myDefaultSaveFormat;

        std::string dir = gSimfile->getDir();
        std::string file = gSimfile->getFile();

        // Give priority to the load format.
        SimFormat fmt = gSimfile->get()->format;
        if (fmt == SIM_SM || fmt == SIM_DWI) {
            saveFmt = (myDefaultSaveFormat == SIM_SSC) ? SIM_SSC : SIM_SM;
        } else if (fmt == SIM_SSC) {
            saveFmt = SIM_SSC;
        } else if (fmt == SIM_OSU || fmt == SIM_OSZ) {
            saveFmt = SIM_OSU;
        }

        // If the path is empty, ask a path from the user.
        if (file.empty() || showSaveAsDialog) {
            // Set the default filter index based on the save format.
            int filterIndex;
            switch (saveFmt) {
                default:
                case SIM_SM:
                    filterIndex = 1;
                    break;
                case SIM_SSC:
                    filterIndex = 2;
                    break;
                case SIM_OSU:
                    filterIndex = 3;
                    break;
            };

            // Show the save file dialog.
            std::string str = dir + file;
            std::string filters(saveFilters, sizeof(saveFilters));
            str = gSystem->saveFileDlg("save file", str, filters, &filterIndex);
            if (str.empty()) return false;

            // Update the song directory and filename.
            Path tmp(str);
            dir = tmp.dir();
            file = tmp.name();

            // Update the save format based on the selected filter index.
            switch (filterIndex) {
                case 1:
                    saveFmt = SIM_SM;
                    break;
                case 2:
                    saveFmt = SIM_SSC;
                    break;
                case 3:
                    saveFmt = SIM_OSU;
                    break;
                default:
                    if (tmp.hasExt("ssc")) {
                        saveFmt = SIM_SSC;
                    } else if (tmp.hasExt("osu")) {
                        saveFmt = SIM_OSU;
                    } else {
                        saveFmt = SIM_SM;
                    }
                    break;
            };
        }

        // Save the simfile.
        if (!gSimfile->save(dir, file, saveFmt)) {
            HudError("Could not save %s", file.c_str());
        }

        // Signal to the edit history that the current state is the saved state.
        gHistory->onFileSaved();

        return true;
    }

    // ================================================================================================
    // EditorImpl :: recent files.

    void addToRecentfiles(std::string path) {
        myRecentFiles.erase_values(path);
        myRecentFiles.insert(0, path, 1);
        myRecentFiles.truncate(MAX_RECENT_FILES);
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

    void handleDialogs() {
        for (int id = 0; id < NUM_DIALOG_IDS; ++id) {
            if (myDialogs[id].requestOpen) {
                handleDialogOpening(static_cast<DialogId>(id), {0, 0, 0, 0});
            }
        }
    }

    void handleDialogOpening(DialogId id, recti rect) {
        auto& entry = myDialogs[id];
        if (entry.ptr) return;

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
        };

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

        entry.ptr = dlg;
        entry.requestOpen = false;
    }

    void onDialogClosed(int id) override { myDialogs[id].ptr = nullptr; }

    // ================================================================================================
    // EditorImpl :: event handling.

    void onCommandLineArgs(const std::string* args, int numArgs) override {
        if (numArgs >= 2 && args[1].length()) {
            Path argPath(gSystem->getRunDir(), args[1]);
            std::string simfilePath = findSimfile(argPath, false);
            openSimfile(simfilePath);
        }
    }

    void onFileDrop(FileDrop& evt) override {
        if (evt.count >= 1) {
            Path dropPath(evt.files[0]);
            std::string simfilePath = findSimfile(dropPath, false);
            openSimfile(simfilePath);
        }
    }

    void onMenuAction(int id) override {
        Action::perform(static_cast<Action::Type>(id));
    }

    void onExitProgram() override {
        if (closeSimfile()) {
            gSystem->terminate();
        }
    }

    void notifyChanges() {
        if (!myChanges) return;

        for (auto dialog : myDialogs) {
            if (dialog.ptr) dialog.ptr->onChanges(myChanges);
        }

        gSimfile->onChanges(myChanges);
        gView->onChanges(myChanges);
        gMusic->onChanges(myChanges);
        gMinimap->onChanges(myChanges);
        gEditing->onChanges(myChanges);
        gNotefield->onChanges(myChanges);
        gTempoBoxes->onChanges(myChanges);
        gWaveform->onChanges(myChanges);

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
        Draw::fill({0, 0, size.x, size.y}, RGBAtoColor32(38, 38, 38, 255));
        Draw::sprite(myLogo, {size.x / 2, size.y / 2},
                     RGBAtoColor32(255, 255, 255, 26));
    }

    void tick() override {
        InputEvents& events = gSystem->getEvents();
        handleInputs(events);
        notifyChanges();

        vec2i windowSize = gSystem->getWindowSize();
        recti r = {0, 0, windowSize.x, windowSize.y};

        gTextOverlay->handleInputs(events);

        GuiMain::setViewSize(r.w, r.h);
        GuiMain::frameStart(deltaTime.count(), events);

        vec2i view = gSystem->getWindowSize();

        handleDialogs();

        gui_->tick({0, 0, view.x, view.y}, deltaTime.count(), events);

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

        if (GuiMain::isCapturingMouse()) {
            gSystem->setCursor(GuiMain::getCursorIcon());
        }

        gTextOverlay->tick();
        gHistory->handleInputs(events);
        gMinimap->handleInputs(events);
        gEditing->handleInputs(events);

        if (gSimfile->isOpen()) {
            gView->tick();
        }

        gSelection->handleInputs(events);

        if (gSimfile->isOpen()) {
            gMusic->tick();
            gMinimap->tick();
            gTempoBoxes->tick();
            gWaveform->tick();
        }

        updateTitle();
        notifyChanges();

        if (gSimfile->isOpen()) {
            gNotefield->draw();
            gMinimap->draw();
            gStatusbar->draw();
        } else {
            drawLogo();
        }

        gui_->draw();

        gTextOverlay->draw();

        GuiMain::frameEnd();
    }

    bool hasMultithreading() const override { return myUseMultithreading; }

    void setBackgroundStyle(int style) override {
        myBackgroundStyle = static_cast<BackgroundStyle>(style);
        gMenubar->update(Menubar::VIEW_BACKGROUND);
    }

    int getBackgroundStyle() const override { return myBackgroundStyle; }

    int getDefaultSaveFormat() const override { return myDefaultSaveFormat; }

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
