#include <Editor/Menubar.h>

#include <Core/WideString.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <Managers/NoteskinMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/StyleMan.h>

#include <Editor/Action.h>
#include <Editor/Shortcuts.h>
#include <Editor/Editor.h>
#include <Editor/Statusbar.h>
#include <Editor/Notefield.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/View.h>
#include <Editor/Waveform.h>
#include <Editor/Editing.h>
#include <Editor/Minimap.h>
#include <Editor/TempoBoxes.h>

#include <System/System.h>
#include <System/Debug.h>

namespace Vortex {

namespace {

#define MENU ((MenuBarImpl*)gMenubar)

// ================================================================================================
// MenuBarImpl :: member data.

struct MenuBarImpl : public Menubar {
    typedef void (*UpdateFunction)();
    typedef System::MenuItem Item;

    Item* myFileMenu;
    Item* myVisualSyncMenu;
    Item* myViewMenu;
    Item* myPreviewMenu;
    Item* myMinimapMenu;
    Item* myBgStyleMenu;
    Item* myStatusMenu;
    Item* myEditMenu;

    UpdateFunction myUpdateFunctions[NUM_PROPERTIES];

    // ================================================================================================
    // MenuBarImpl :: constructor and destructor.

    ~MenuBarImpl() = default;

    MenuBarImpl() { registerUpdateFunctions(); }

    // ================================================================================================
    // MenuBarImpl :: menu construction functions.

    static Item* newMenu() { return System::MenuItem::create(); }

    static void sep(Item* menu) { menu->addSeperator(); }

    static void add(Item* menu, Action::Type action, const char* str) {
        std::string notation = gShortcuts->getNotation(action, false);
        if (notation.length()) {
            std::string combined(str);
            Str::append(combined, '\t');
            Str::append(combined, notation);
            menu->addItem(action, combined);
        } else {
            menu->addItem(action, str);
        }
    }

    static void add(Item* menu, Action::Type action, const wchar_t* str) {
        add(menu, action, Narrow(str).c_str());
    }

    static void add(Item* menu, int item, const wchar_t* str) {
        menu->addItem(item, Narrow(str).c_str());
    }

    static void add(Item* menu, int item, const char* str) {
        menu->addItem(item, str);
    }

    static void sub(Item* menu, Item* sub, const char* str) {
        menu->addSubmenu(sub, str);
    }

    void init(Item* menu) override {
        using namespace Action;

        // File menu.
        Item* hFile = newMenu();
        add(hFile, FILE_OPEN, "Open...");
        add(hFile, 0 /*dummy*/, "Recent files");
        add(hFile, FILE_CLOSE, "Close");
        sep(hFile);
        add(hFile, FILE_SAVE, "Save");
        add(hFile, FILE_SAVE_AS, "Save as...");
        sep(hFile);
        add(hFile, OPEN_DIALOG_SONG_PROPERTIES, "Properties...");
        sep(hFile);
        add(hFile, EXIT_PROGRAM, "Exit");
        myFileMenu = hFile;

        // Edit menu.
        Item* hEdit = myEditMenu = newMenu();
        add(hEdit, EDIT_UNDO, "Undo\tCtrl+Z");
        add(hEdit, EDIT_REDO, "Redo\tCtrl+Y");
        sep(hEdit);
        add(hEdit, EDIT_CUT, "Cut\tCtrl+X");
        add(hEdit, EDIT_COPY, "Copy\tCtrl+C");
        add(hEdit, EDIT_PASTE, "Paste\tCtrl+V");
        add(hEdit, EDIT_DELETE, "Delete\tDelete");
        sep(hEdit);
        add(hEdit, SELECT_ALL, "Select all\tCtrl+A");
        add(hEdit, SELECT_REGION, "Select region");
        sep(hEdit);
        add(hEdit, TOGGLE_JUMP_TO_NEXT_NOTE, "Enable jump to next note");
        add(hEdit, TOGGLE_UNDO_REDO_JUMP, "Enable undo/redo jump");
        add(hEdit, TOGGLE_TIME_BASED_COPY, "Enable time-based copy");

        // Chart > Convert menu.
        Item* hChartConvert = newMenu();
        add(hChartConvert, CHART_CONVERT_ROUTINE_TO_COUPLES,
            L"Routine \x2192 ITG Couple");
        add(hChartConvert, CHART_CONVERT_COUPLES_TO_ROUTINE,
            L"ITG Couple \x2192 Routine");

        // Chart menu.
        Item* hChart = newMenu();
        add(hChart, OPEN_DIALOG_CHART_LIST, "Chart list...");
        add(hChart, OPEN_DIALOG_CHART_PROPERTIES, "Properties...");
        add(hChart, OPEN_DIALOG_DANCING_BOT, "Dancing bot...");
        sep(hChart);
        add(hChart, OPEN_DIALOG_NEW_CHART, "New chart...");
        sep(hChart);
        add(hChart, CHART_PREVIOUS, "Previous chart");
        add(hChart, CHART_NEXT, "Next chart");
        sep(hChart);
        sub(hChart, hChartConvert, "Convert");
        sep(hChart);
        add(hChart, CHART_DELETE, "Delete chart");

        // Notes > Select > Quantization menu.
        Item* hSelectQuant = newMenu();
        add(hSelectQuant, SELECT_QUANT_4, "4th");
        add(hSelectQuant, SELECT_QUANT_8, "8th");
        add(hSelectQuant, SELECT_QUANT_12, "12th");
        add(hSelectQuant, SELECT_QUANT_16, "16th");
        add(hSelectQuant, SELECT_QUANT_24, "24th");
        add(hSelectQuant, SELECT_QUANT_32, "32nd");
        add(hSelectQuant, SELECT_QUANT_48, "48th");
        add(hSelectQuant, SELECT_QUANT_64, "64th");
        add(hSelectQuant, SELECT_QUANT_192, "192nd");

        // Notes > Select menu.
        Item* hSelection = newMenu();
        sub(hSelection, hSelectQuant, "Quantization");
        sep(hSelection);
        add(hSelection, SELECT_ALL_STEPS, "Steps");
        add(hSelection, SELECT_ALL_MINES, "Mines");
        add(hSelection, SELECT_ALL_HOLDS, "Holds");
        add(hSelection, SELECT_ALL_ROLLS, "Rolls");
        add(hSelection, SELECT_ALL_FAKES, "Fakes");
        add(hSelection, SELECT_ALL_LIFTS, "Lifts");
        sep(hSelection);
        add(hSelection, SELECT_REGION_BEFORE_CURSOR, "Before cursor");
        add(hSelection, SELECT_REGION_AFTER_CURSOR, "After cursor");

        // Notes > Convert menu.
        Item* hNoteConvert = newMenu();
        add(hNoteConvert, CHANGE_NOTES_TO_MINES, L"Notes \x2192 Mines");
        add(hNoteConvert, CHANGE_NOTES_TO_FAKES, L"Notes \x2192 Fakes");
        add(hNoteConvert, CHANGE_NOTES_TO_LIFTS, L"Notes \x2192 Lifts");
        sep(hNoteConvert);
        add(hNoteConvert, CHANGE_MINES_TO_NOTES, L"Mines \x2192 Notes");
        add(hNoteConvert, CHANGE_MINES_TO_FAKES, L"Mines \x2192 Fakes");
        add(hNoteConvert, CHANGE_MINES_TO_LIFTS, L"Mines \x2192 Lifts");
        sep(hNoteConvert);
        add(hNoteConvert, CHANGE_FAKES_TO_NOTES, L"Fakes \x2192 Notes");
        add(hNoteConvert, CHANGE_LIFTS_TO_NOTES, L"Lifts \x2192 Notes");
        sep(hNoteConvert);
        add(hNoteConvert, CHANGE_BETWEEN_HOLDS_AND_ROLLS,
            L"Holds \x2194 Rolls");
        add(hNoteConvert, CHANGE_HOLDS_TO_STEPS, L"Holds \x2192 Steps");
        add(hNoteConvert, CHANGE_HOLDS_TO_MINES, L"Holds \x2192 Mines");
        sep(hNoteConvert);
        add(hNoteConvert, CHANGE_BETWEEN_PLAYER_NUMBERS, L"Switch Player");
        add(hNoteConvert, CHANGE_NOTE_SIDE, L"Switch Sides");

        // Notes > Mirror menu.
        Item* hNoteMirror = newMenu();
        add(hNoteMirror, MIRROR_NOTES_HORIZONTALLY, L"Horizontally");
        add(hNoteMirror, MIRROR_NOTES_VERTICALLY, L"Vertically");
        add(hNoteMirror, MIRROR_NOTES_FULL, L"Both");

        // Notes > Expand menu.
        Item* hNoteExpand = newMenu();
        add(hNoteExpand, SCALE_NOTES_2_TO_1, "2:1 (8th to 4th)");
        add(hNoteExpand, SCALE_NOTES_3_TO_2, "3:2 (12th to 8th)");
        add(hNoteExpand, SCALE_NOTES_4_TO_3, "4:3 (16th to 12th)");

        // Notes > Compress menu.
        Item* hNoteCompress = newMenu();
        add(hNoteCompress, SCALE_NOTES_1_TO_2, "1:2 (4th to 8th)");
        add(hNoteCompress, SCALE_NOTES_2_TO_3, "2:3 (8th to 12th)");
        add(hNoteCompress, SCALE_NOTES_3_TO_4, "3:4 (12th to 16th)");

        // Notes menu.
        Item* hNotes = newMenu();
        sub(hNotes, hSelection, "Select");
        sub(hNotes, hNoteConvert, "Convert");
        sub(hNotes, hNoteMirror, "Mirror");
        sub(hNotes, hNoteExpand, "Expand");
        sub(hNotes, hNoteCompress, "Compress");
        add(hNotes, OPEN_DIALOG_GENERATE_NOTES, "Generate...");

        // Tempo > Select menu.
        Item* hSelectTempo = newMenu();
        add(hSelectTempo, SELECT_TEMPO_BPM, "BPM");
        add(hSelectTempo, SELECT_TEMPO_STOP, "Stop");
        add(hSelectTempo, SELECT_TEMPO_DELAY, "Delay");
        add(hSelectTempo, SELECT_TEMPO_WARP, "Warp");
        add(hSelectTempo, SELECT_TEMPO_TIME_SIG, "Time Sig.");
        add(hSelectTempo, SELECT_TEMPO_TICK_COUNT, "Tick Count");
        add(hSelectTempo, SELECT_TEMPO_COMBO, "Combo");
        add(hSelectTempo, SELECT_TEMPO_SPEED, "Speed");
        add(hSelectTempo, SELECT_TEMPO_SCROLL, "Scroll");
        add(hSelectTempo, SELECT_TEMPO_FAKE, "Fake");
        add(hSelectTempo, SELECT_TEMPO_LABEL, "Label");

        // Tempo > Visual sync menu
        myVisualSyncMenu = newMenu();
        add(myVisualSyncMenu, SET_VISUAL_SYNC_CURSOR_ANCHOR, "Cursor row");
        add(myVisualSyncMenu, SET_VISUAL_SYNC_RECEPTOR_ANCHOR, "Receptors row");

        // Tempo menu.
        Item* hTempo = newMenu();
        sub(hTempo, hSelectTempo, "Select");
        sep(hTempo);
        add(hTempo, OPEN_DIALOG_ADJUST_SYNC, "Adjust sync...");
        add(hTempo, OPEN_DIALOG_ADJUST_TEMPO, "Adjust tempo...");
        add(hTempo, OPEN_DIALOG_ADJUST_TEMPO_SM5, "Adjust tempo SM5...");
        sep(hTempo);
        add(hTempo, SWITCH_TO_SYNC_MODE, "Sync mode");
        add(hTempo, OPEN_DIALOG_LABEL_BREAKDOWN, "Labels...");
        add(hTempo, OPEN_DIALOG_TEMPO_BREAKDOWN, "Breakdown...");
        sub(hTempo, myVisualSyncMenu, "Visual sync anchor");

        // Audio > Volume menu.
        Item* hAudioVol = newMenu();
        add(hAudioVol, VOLUME_RESET, "Default");
        sep(hAudioVol);
        add(hAudioVol, VOLUME_INCREASE, "Louder");
        add(hAudioVol, VOLUME_DECREASE, "Softer");
        sep(hAudioVol);
        add(hAudioVol, VOLUME_MUTE, "Mute");

        // Audio > Speed menu.
        Item* hAudioSpeed = newMenu();
        add(hAudioSpeed, SPEED_RESET, "Default");
        sep(hAudioSpeed);
        add(hAudioSpeed, SPEED_INCREASE, "Faster");
        add(hAudioSpeed, SPEED_DECREASE, "Slower");

        // Audio menu.
        Item* hAudio = newMenu();
        sub(hAudio, hAudioVol, "Volume");
        sub(hAudio, hAudioSpeed, "Speed");
        sep(hAudio);
        add(hAudio, TOGGLE_BEAT_TICK, "Beat tick");
        add(hAudio, TOGGLE_NOTE_TICK, "Note tick");
        sep(hAudio);
        add(hAudio, CONVERT_MUSIC_TO_OGG, "Convert to ogg");

        // View > Preview menu.
        myPreviewMenu = newMenu();
        add(myPreviewMenu, OPEN_DIALOG_PREVIEW_SETTINGS, "Options");
        add(myPreviewMenu, PREVIEW_TOGGLE_ENABLED, "Enabled");
        sep(myPreviewMenu);
        add(myPreviewMenu, PREVIEW_TOGGLE_SHOW_BEAT_LINES, "Show beat lines");
        add(myPreviewMenu, PREVIEW_TOGGLE_REVERSE_SCROLL, "Reverse scroll");
        sep(myPreviewMenu);
        add(myPreviewMenu, PREVIEW_VIEW_CMOD, "Time based (C-mod)");
        add(myPreviewMenu, PREVIEW_VIEW_XMOD, "Row based (X-mod)");
        add(myPreviewMenu, PREVIEW_VIEW_VARIABLE, "Variable");

        // View > Minimap menu.
        Item* hViewMm = myMinimapMenu = newMenu();
        add(hViewMm, MINIMAP_SET_NOTES, "Notes");
        add(hViewMm, MINIMAP_SET_DENSITY, "Density");

        // View > Background menu.
        Item* hViewBg = myBgStyleMenu = newMenu();
        add(hViewBg, BACKGROUND_HIDE, "Hide");
        sep(hViewBg);
        add(hViewBg, BACKGROUND_INCREASE_ALPHA, "More visible");
        add(hViewBg, BACKGROUND_DECREASE_ALPHA, "Less visible");
        sep(hViewBg);
        add(hViewBg, BACKGROUND_SET_STRETCH, "Stretch");
        add(hViewBg, BACKGROUND_SET_LETTERBOX, "Letterbox");
        add(hViewBg, BACKGROUND_SET_CROP, "Crop");

        // View > Zoom menu.
        Item* hViewZoom = newMenu();
        add(hViewZoom, OPEN_DIALOG_ZOOM, "Options");
        sep(hViewZoom);
        add(hViewZoom, ZOOM_RESET, "Reset");
        sep(hViewZoom);
        add(hViewZoom, ZOOM_IN, "Zoom in");
        add(hViewZoom, ZOOM_OUT, "Zoom out");
        add(hViewZoom, SCALE_INCREASE, "Scale increase");
        add(hViewZoom, SCALE_DECREASE, "Scale decrease");

        // View > Snap menu.
        Item* hViewSnap = newMenu();
        add(hViewSnap, SNAP_RESET, "Reset");
        sep(hViewSnap);
        add(hViewSnap, OPEN_DIALOG_CUSTOM_SNAP, "Set Snap");
        add(hViewSnap, SNAP_PREVIOUS, "Previous");
        add(hViewSnap, SNAP_NEXT, "Next");

        // View > Cursor menu.
        Item* hViewCursor = newMenu();
        add(hViewCursor, CURSOR_UP, "Up");
        add(hViewCursor, CURSOR_DOWN, "Down");
        sep(hViewCursor);
        add(hViewCursor, CURSOR_PREVIOUS_BEAT, "Previous beat");
        add(hViewCursor, CURSOR_NEXT_BEAT, "Next beat");
        add(hViewCursor, CURSOR_PREVIOUS_MEASURE, "Previous measure");
        add(hViewCursor, CURSOR_NEXT_MEASURE, "Next measure");
        sep(hViewCursor);
        add(hViewCursor, CURSOR_STREAM_START, "Stream start");
        add(hViewCursor, CURSOR_STREAM_END, "Stream end");
        sep(hViewCursor);
        add(hViewCursor, CURSOR_SELECTION_START, "Selection start");
        add(hViewCursor, CURSOR_SELECTION_END, "Selection end");
        sep(hViewCursor);
        add(hViewCursor, CURSOR_CHART_START, "First beat");
        add(hViewCursor, CURSOR_CHART_END, "Last beat");

        // View > Statusbar menu.
        myStatusMenu = newMenu();
        add(myStatusMenu, TOGGLE_STATUS_CHART, "Show chart");
        add(myStatusMenu, TOGGLE_STATUS_SNAP, "Show snap");
        add(myStatusMenu, TOGGLE_STATUS_BPM, "Show BPM");
        add(myStatusMenu, TOGGLE_STATUS_ROW, "Show row");
        add(myStatusMenu, TOGGLE_STATUS_BEAT, "Show beat");
        add(myStatusMenu, TOGGLE_STATUS_MEASURE, "Show measure");
        add(myStatusMenu, TOGGLE_STATUS_TIME, "Show time");
        add(myStatusMenu, TOGGLE_STATUS_TIMING_MODE, "Show timing mode");
        add(myStatusMenu, TOGGLE_STATUS_SCROLL, "Show scroll mod");
        add(myStatusMenu, TOGGLE_STATUS_SPEED, "Show speed mod");

        // View menu.
        myViewMenu = newMenu();
        add(myViewMenu, TOGGLE_SHOW_WAVEFORM, "Show waveform");
        add(myViewMenu, TOGGLE_SHOW_BEAT_LINES, "Show beat lines");
        add(myViewMenu, TOGGLE_SHOW_TEMPO_BOXES, "Show tempo boxes");
        add(myViewMenu, TOGGLE_SHOW_TEMPO_HELP, "Show tempo help");
        add(myViewMenu, TOGGLE_SHOW_NOTES, "Show notes");
        add(myViewMenu, TOGGLE_CHART_PREVIEW, "Use SM-style preview");
        sep(myViewMenu);
        add(myViewMenu, TOGGLE_REVERSE_SCROLL, "Reverse scroll");
        sep(myViewMenu);
        add(myViewMenu, USE_TIME_BASED_VIEW, "Time based (C-mod)");
        add(myViewMenu, USE_ROW_BASED_VIEW, "Row based (X-mod)");
        sep(myViewMenu);
        add(myViewMenu, OPEN_DIALOG_WAVEFORM_SETTINGS, "Waveform...");
        add(myViewMenu, 0 /*dummy*/, "Noteskins");
        sub(myViewMenu, myPreviewMenu, "Preview");
        sub(myViewMenu, myMinimapMenu, "Minimap");
        sub(myViewMenu, myBgStyleMenu, "Background");
        sub(myViewMenu, hViewZoom, "Zoom");
        sub(myViewMenu, hViewSnap, "Snap");
        sub(myViewMenu, hViewCursor, "Cursor");
        sub(myViewMenu, myStatusMenu, "Status");

        // Help menu.
        Item* hHelp = newMenu();
        add(hHelp, SHOW_SHORTCUTS, "Shortcuts...");
        add(hHelp, SHOW_MESSAGE_LOG, "Message Log...");
        add(hHelp, SHOW_DEBUG_LOG, "Debug Log...");
        sep(hHelp);
        add(hHelp, SHOW_ABOUT, "About...");

        // Top level menu.
        sub(menu, hFile, "File");
        sub(menu, hEdit, "Edit");
        sub(menu, hChart, "Chart");
        sub(menu, hNotes, "Notes");
        sub(menu, hTempo, "Tempo");
        sub(menu, hAudio, "Audio");
        sub(menu, myViewMenu, "View");
        sub(menu, hHelp, "Help");

        update(ALL_PROPERTIES);
    }

    // ================================================================================================
    // Menubar :: menu update functions.

    void registerUpdateFunctions() {
        using namespace Action;

        myUpdateFunctions[OPEN_FILE] = [] {
            MENU->myFileMenu->setEnabled(FILE_CLOSE, gSimfile->isOpen());
            MENU->myFileMenu->setEnabled(FILE_SAVE, gSimfile->isOpen());
            MENU->myFileMenu->setEnabled(FILE_SAVE_AS, gSimfile->isOpen());
        };
        myUpdateFunctions[RECENT_FILES] = [] {
            Item* recent = newMenu();
            int numFiles = min(gEditor->getNumRecentFiles(),
                               static_cast<int>(Action::MAX_RECENT_FILES));
            if (numFiles > 0) {
                recent->addItem(FILE_CLEAR_RECENT_FILES, "Clear list");
                recent->addSeperator();
                for (int i = 0; i < numFiles; ++i) {
                    recent->addItem(FILE_OPEN_RECENT_BEGIN + i,
                                    gEditor->getRecentFile(i));
                }
            }
            MENU->myFileMenu->replaceSubmenu(1, recent, "Recent files",
                                             (numFiles == 0));
        };
        myUpdateFunctions[SHOW_WAVEFORM] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_SHOW_WAVEFORM,
                                         gNotefield->hasShowWaveform());
        };
        myUpdateFunctions[SHOW_BEATLINES] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_SHOW_BEAT_LINES,
                                         gNotefield->hasShowBeatLines());
        };
        myUpdateFunctions[SHOW_NOTES] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_SHOW_NOTES,
                                         gNotefield->hasShowNotes());
        };
        myUpdateFunctions[SHOW_TEMPO_BOXES] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_SHOW_TEMPO_BOXES,
                                         gTempoBoxes->hasShowBoxes());
        };
        myUpdateFunctions[SHOW_TEMPO_HELP] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_SHOW_TEMPO_HELP,
                                         gTempoBoxes->hasShowHelp());
        };
        myUpdateFunctions[USE_JUMP_TO_NEXT_NOTE] = [] {
            MENU->myEditMenu->setChecked(TOGGLE_JUMP_TO_NEXT_NOTE,
                                         gEditing->hasJumpToNextNote());
        };
        myUpdateFunctions[USE_UNDO_REDO_JUMP] = [] {
            MENU->myEditMenu->setChecked(TOGGLE_UNDO_REDO_JUMP,
                                         gEditing->hasUndoRedoJump());
        };
        myUpdateFunctions[USE_TIME_BASED_COPY] = [] {
            MENU->myEditMenu->setChecked(TOGGLE_TIME_BASED_COPY,
                                         gEditing->hasTimeBasedCopy());
        };
        myUpdateFunctions[VISUAL_SYNC_ANCHOR] = [] {
            MENU->myVisualSyncMenu->setChecked(
                SET_VISUAL_SYNC_CURSOR_ANCHOR,
                gEditing->getVisualSyncMode() ==
                    Editing::VisualSyncAnchor::CURSOR);
            MENU->myVisualSyncMenu->setChecked(
                SET_VISUAL_SYNC_RECEPTOR_ANCHOR,
                gEditing->getVisualSyncMode() ==
                    Editing::VisualSyncAnchor::RECEPTORS);
        };
        myUpdateFunctions[USE_REVERSE_SCROLL] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_REVERSE_SCROLL,
                                         gView->hasReverseScroll());
        };
        myUpdateFunctions[USE_CHART_PREVIEW] = [] {
            MENU->myViewMenu->setChecked(TOGGLE_CHART_PREVIEW,
                                         gView->hasChartPreview());
        };
        myUpdateFunctions[VIEW_MODE] = [] {
            MENU->myViewMenu->setChecked(USE_ROW_BASED_VIEW,
                                         !gView->isTimeBased());
            MENU->myViewMenu->setChecked(USE_TIME_BASED_VIEW,
                                         gView->isTimeBased());
        };
        myUpdateFunctions[PREVIEW_ENABLED] = [] {
            MENU->myPreviewMenu->setChecked(PREVIEW_TOGGLE_ENABLED,
                                            gNotefieldPreview->hasEnabled());
        };
        myUpdateFunctions[PREVIEW_SHOW_BEATLINES] = [] {
            MENU->myPreviewMenu->setChecked(
                PREVIEW_TOGGLE_SHOW_BEAT_LINES,
                gNotefieldPreview->hasShowBeatLines());
        };
        myUpdateFunctions[PREVIEW_SHOW_REVERSE_SCROLL] = [] {
            MENU->myPreviewMenu->setChecked(
                PREVIEW_TOGGLE_REVERSE_SCROLL,
                gNotefieldPreview->hasReverseScroll());
        };
        myUpdateFunctions[PREVIEW_VIEW_MODE] = [] {
            auto mode = gNotefieldPreview->getMode();
            MENU->myPreviewMenu->setChecked(PREVIEW_VIEW_CMOD,
                                            mode == NotefieldPreview::CMOD);
            MENU->myPreviewMenu->setChecked(PREVIEW_VIEW_XMOD,
                                            mode == NotefieldPreview::XMOD);
            MENU->myPreviewMenu->setChecked(PREVIEW_VIEW_VARIABLE,
                                            mode == NotefieldPreview::VARIABLE);
        };
        myUpdateFunctions[VIEW_MINIMAP] = [] {
            auto mode = gMinimap->getMode();
            MENU->myMinimapMenu->setChecked(MINIMAP_SET_NOTES,
                                            mode == Minimap::NOTES);
            MENU->myMinimapMenu->setChecked(MINIMAP_SET_DENSITY,
                                            mode == Minimap::DENSITY);
        };
        myUpdateFunctions[VIEW_BACKGROUND] = [] {
            auto bg = gEditor->getBackgroundStyle();
            MENU->myBgStyleMenu->setChecked(BACKGROUND_SET_STRETCH,
                                            bg == BG_STYLE_STRETCH);
            MENU->myBgStyleMenu->setChecked(BACKGROUND_SET_LETTERBOX,
                                            bg == BG_STYLE_LETTERBOX);
            MENU->myBgStyleMenu->setChecked(BACKGROUND_SET_CROP,
                                            bg == BG_STYLE_CROP);
        };
        myUpdateFunctions[VIEW_NOTESKIN] = [] {
            Item* hSkins = System::MenuItem::create();
            int numValid = 0;
            int numTypes = min(gNoteskin->getNumTypes(),
                               static_cast<int>(Action::MAX_NOTESKINS));
            int activeType = gNoteskin->getType();
            for (int type = 0; type < numTypes; ++type) {
                if (gNoteskin->isSupported(type)) {
                    hSkins->addItem(SET_NOTESKIN_BEGIN + type,
                                    gNoteskin->getName(type));
                    if (type == activeType) {
                        hSkins->setChecked(static_cast<Action::Type>(
                                               SET_NOTESKIN_BEGIN + type),
                                           true);
                    }
                    ++numValid;
                }
            }
            // If the active type was zero, set it to the first skin in the list
            if (!activeType) {
                hSkins->setChecked((SET_NOTESKIN_BEGIN), true);
            }
            MENU->myViewMenu->replaceSubmenu(13, hSkins, "Noteskins",
                                             (numValid == 0));
        };
        myUpdateFunctions[STATUSBAR_CHART] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_CHART,
                                           gStatusbar->hasChart());
        };
        myUpdateFunctions[STATUSBAR_SNAP] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_SNAP,
                                           gStatusbar->hasSnap());
        };
        myUpdateFunctions[STATUSBAR_BPM] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_BPM,
                                           gStatusbar->hasBpm());
        };
        myUpdateFunctions[STATUSBAR_ROW] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_ROW,
                                           gStatusbar->hasRow());
        };
        myUpdateFunctions[STATUSBAR_BEAT] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_BEAT,
                                           gStatusbar->hasBeat());
        };
        myUpdateFunctions[STATUSBAR_MEASURE] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_MEASURE,
                                           gStatusbar->hasMeasure());
        };
        myUpdateFunctions[STATUSBAR_TIME] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_TIME,
                                           gStatusbar->hasTime());
        };
        myUpdateFunctions[STATUSBAR_TIMING_MODE] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_TIMING_MODE,
                                           gStatusbar->hasTimingMode());
        };
        myUpdateFunctions[STATUSBAR_SCROLL] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_SCROLL,
                                           gStatusbar->hasScroll());
        };
        myUpdateFunctions[STATUSBAR_SPEED] = [] {
            MENU->myStatusMenu->setChecked(TOGGLE_STATUS_SPEED,
                                           gStatusbar->hasSpeed());
        };
    }

    void update(Property prop) override {
        if (prop == ALL_PROPERTIES) {
            for (int i = 1; i < NUM_PROPERTIES; ++i) {
                myUpdateFunctions[i]();
            }
        } else {
            myUpdateFunctions[prop]();
        }
    }

};  // MenuBarImpl
};  // anonymous namespace.

// ================================================================================================
// Menubar API.

Menubar* gMenubar = nullptr;

void Menubar::create() { gMenubar = new MenuBarImpl; }

void Menubar::destroy() {
    delete static_cast<MenuBarImpl*>(gMenubar);
    gMenubar = nullptr;
}

};  // namespace Vortex
