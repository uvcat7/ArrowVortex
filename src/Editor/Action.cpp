#include <Editor/Action.h>

#include <Core/Input.h>

#include <Dialogs/Dialog.h>

#include <Editor/Common.h>
#include <Editor/ConvertAudio.h>
#include <Editor/Editing.h>
#include <Editor/Editor.h>
#include <Editor/Menubar.h>
#include <Editor/Minimap.h>
#include <Editor/Music.h>
#include <Editor/Notefield.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/Selection.h>
#include <Editor/Statusbar.h>
#include <Editor/TempoBoxes.h>
#include <Editor/TextOverlay.h>
#include <Editor/View.h>

#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/SimfileMan.h>

#include <Simfile/Common.h>
#include <Simfile/Notes.h>
#include <Simfile/Segments.h>

#include <System/System.h>

namespace Vortex {

bool Action::perform(Type action) {
    gMenubar->closeMenus();

    bool handled = true;

    // Range Actions
    if (action >= FILE_OPEN_RECENT_BEGIN && action < FILE_OPEN_RECENT_END) {
        gEditor->openSimfile(action - FILE_OPEN_RECENT_BEGIN);
        return handled;
    } else if (action >= SELECT_DENSITY_BEGIN && action < SELECT_DENSITY_END) {
        gSelection->selectNotes(SelectModifier::SELECT_SET,
                                action - SELECT_DENSITY_BEGIN + 1);
        return handled;
    } else if (action >= SET_NOTESKIN_BEGIN && action < SET_NOTESKIN_END) {
        gNoteskin->setType(action - SET_NOTESKIN_BEGIN);
        return handled;
    }

    // Standard Actions
    switch (action) {
        case EXIT_PROGRAM: {
            gEditor->onExitProgram();
            break;
        }

        case FILE_OPEN: {
            gEditor->openSimfile();
            break;
        }
        case FILE_SAVE: {
            gEditor->saveSimfile(false);
            break;
        }
        case FILE_SAVE_AS: {
            gEditor->saveSimfile(true);
            break;
        }
        case FILE_CLOSE: {
            gEditor->closeSimfile();
            break;
        }

        case FILE_CLEAR_RECENT_FILES: {
            gEditor->clearRecentFiles();
            break;
        }

        case OPEN_DIALOG_SONG_PROPERTIES: {
            gEditor->openDialog(DIALOG_SONG_PROPERTIES);
            break;
        }
        case OPEN_DIALOG_CHART_PROPERTIES: {
            gEditor->openDialog(DIALOG_CHART_PROPERTIES);
            break;
        }
        case OPEN_DIALOG_CHART_LIST: {
            gEditor->openDialog(DIALOG_CHART_LIST);
            break;
        }
        case OPEN_DIALOG_NEW_CHART: {
            gEditor->openDialog(DIALOG_NEW_CHART);
            break;
        }
        case OPEN_DIALOG_ADJUST_SYNC: {
            gEditor->openDialog(DIALOG_ADJUST_SYNC);
            break;
        }
        case OPEN_DIALOG_ADJUST_TEMPO: {
            gEditor->openDialog(DIALOG_ADJUST_TEMPO);
            break;
        }
        case OPEN_DIALOG_ADJUST_TEMPO_SM5: {
            gEditor->openDialog(DIALOG_ADJUST_TEMPO_SM5);
            break;
        }
        case OPEN_DIALOG_DANCING_BOT: {
            gEditor->openDialog(DIALOG_DANCING_BOT);
            break;
        }
        case OPEN_DIALOG_GENERATE_NOTES: {
            gEditor->openDialog(DIALOG_GENERATE_NOTES);
            break;
        }
        case OPEN_DIALOG_TEMPO_BREAKDOWN: {
            gEditor->openDialog(DIALOG_TEMPO_BREAKDOWN);
            break;
        }
        case OPEN_DIALOG_LABEL_BREAKDOWN: {
            gEditor->openDialog(DIALOG_LABEL_BREAKDOWN);
            break;
        }
        case OPEN_DIALOG_WAVEFORM_SETTINGS: {
            gEditor->openDialog(DIALOG_WAVEFORM_SETTINGS);
            break;
        }
        case OPEN_DIALOG_ZOOM: {
            gEditor->openDialog(DIALOG_ZOOM);
            break;
        }
        case OPEN_DIALOG_CUSTOM_SNAP: {
            gEditor->openDialog(DIALOG_CUSTOM_SNAP);
            break;
        }
        case OPEN_DIALOG_PREVIEW_SETTINGS: {
            gEditor->openDialog(DIALOG_PREVIEW_SETTINGS);
            break;
        }

        case EDIT_UNDO: {
            gSystem->getEvents().addKeyPress(Key::Z, Keyflag::CTRL, false);
            break;
        }
        case EDIT_REDO: {
            gSystem->getEvents().addKeyPress(Key::Y, Keyflag::CTRL, false);
            break;
        }
        case EDIT_CUT: {
            gSystem->getEvents().addKeyPress(Key::X, Keyflag::CTRL, false);
            break;
        }
        case EDIT_COPY: {
            gSystem->getEvents().addKeyPress(Key::C, Keyflag::CTRL, false);
            break;
        }
        case EDIT_PASTE: {
            gSystem->getEvents().addKeyPress(Key::V, Keyflag::CTRL, false);
            break;
        }
        case EDIT_PASTE_INSERT: {
            gSystem->getEvents().addKeyPress(
                Key::V, Keyflag::CTRL | Keyflag::SHIFT, false);
            break;
        }
        case EDIT_DELETE: {
            gSystem->getEvents().addKeyPress(Key::DELETE, 0, false);
            break;
        }
        case SELECT_ALL: {
            gSystem->getEvents().addKeyPress(Key::A, Keyflag::CTRL, false);
            break;
        }

        case TOGGLE_JUMP_TO_NEXT_NOTE: {
            gEditing->toggleJumpToNextNote();
            break;
        }
        case TOGGLE_UNDO_REDO_JUMP: {
            gEditing->toggleUndoRedoJump();
            break;
        }
        case TOGGLE_TIME_BASED_COPY: {
            gEditing->toggleTimeBasedCopy();
            break;
        }

        case SET_VISUAL_SYNC_CURSOR_ANCHOR: {
            gEditing->setVisualSyncAnchor(Editing::EditingAnchor::CURSOR);
            break;
        }
        case SET_VISUAL_SYNC_RECEPTOR_ANCHOR: {
            gEditing->setVisualSyncAnchor(Editing::EditingAnchor::RECEPTORS);
            break;
        }
        case INJECT_BOUNDING_BPM_CHANGE: {
            gEditing->injectBoundingBpmChange();
            break;
        }

        case REQUANTIZE_NOTES: {
            gEditing->requantizeNotes();
            break;
        }

        case SELECT_REGION: {
            gSelection->selectRegion();
            break;
        }
        case SELECT_ALL_STEPS: {
            gSelection->selectNotes(NotesMan::SELECT_STEPS);
            break;
        }
        case SELECT_ALL_MINES: {
            gSelection->selectNotes(NotesMan::SELECT_MINES);
            break;
        }
        case SELECT_ALL_HOLDS: {
            gSelection->selectNotes(NotesMan::SELECT_HOLDS);
            break;
        }
        case SELECT_ALL_ROLLS: {
            gSelection->selectNotes(NotesMan::SELECT_ROLLS);
            break;
        }
        case SELECT_ALL_FAKES: {
            gSelection->selectNotes(NotesMan::SELECT_FAKES);
            break;
        }
        case SELECT_ALL_LIFTS: {
            gSelection->selectNotes(NotesMan::SELECT_LIFTS);
            break;
        }
        case SELECT_REGION_BEFORE_CURSOR: {
            gSelection->selectRegion(0, gView->getCursorRow());
            break;
        }
        case SELECT_REGION_AFTER_CURSOR: {
            gSelection->selectRegion(gView->getCursorRow(),
                                     gSimfile->getEndRow());
            break;
        }

        case SELECT_QUANT_4: {
            gSelection->selectNotes(RT_4TH);
            break;
        }
        case SELECT_QUANT_8: {
            gSelection->selectNotes(RT_8TH);
            break;
        }
        case SELECT_QUANT_12: {
            gSelection->selectNotes(RT_12TH);
            break;
        }
        case SELECT_QUANT_16: {
            gSelection->selectNotes(RT_16TH);
            break;
        }
        case SELECT_QUANT_24: {
            gSelection->selectNotes(RT_24TH);
            break;
        }
        case SELECT_QUANT_32: {
            gSelection->selectNotes(RT_32ND);
            break;
        }
        case SELECT_QUANT_48: {
            gSelection->selectNotes(RT_48TH);
            break;
        }
        case SELECT_QUANT_64: {
            gSelection->selectNotes(RT_64TH);
            break;
        }
        case SELECT_QUANT_192: {
            gSelection->selectNotes(RT_192TH);
            break;
        }

        case SELECT_TEMPO_BPM: {
            gTempoBoxes->selectType(Segment::BPM);
            break;
        }
        case SELECT_TEMPO_STOP: {
            gTempoBoxes->selectType(Segment::STOP);
            break;
        }
        case SELECT_TEMPO_DELAY: {
            gTempoBoxes->selectType(Segment::DELAY);
            break;
        }
        case SELECT_TEMPO_WARP: {
            gTempoBoxes->selectType(Segment::WARP);
            break;
        }
        case SELECT_TEMPO_TIME_SIG: {
            gTempoBoxes->selectType(Segment::TIME_SIG);
            break;
        }
        case SELECT_TEMPO_TICK_COUNT: {
            gTempoBoxes->selectType(Segment::TICK_COUNT);
            break;
        }
        case SELECT_TEMPO_COMBO: {
            gTempoBoxes->selectType(Segment::COMBO);
            break;
        }
        case SELECT_TEMPO_SPEED: {
            gTempoBoxes->selectType(Segment::SPEED);
            break;
        }
        case SELECT_TEMPO_SCROLL: {
            gTempoBoxes->selectType(Segment::SCROLL);
            break;
        }
        case SELECT_TEMPO_FAKE: {
            gTempoBoxes->selectType(Segment::FAKE);
            break;
        }
        case SELECT_TEMPO_LABEL: {
            gTempoBoxes->selectType(Segment::LABEL);
            break;
        }

        case SET_TEMPO_EDIT_CURSOR_ANCHOR: {
            gEditing->setTempoEditAnchor(Editing::EditingAnchor::CURSOR);
            break;
        }
        case SET_TEMPO_EDIT_RECEPTOR_ANCHOR: {
            gEditing->setTempoEditAnchor(Editing::EditingAnchor::RECEPTORS);
            break;
        }
        case EDIT_TEMPO_BPM: {
            gEditing->openTempoEdit(Segment::BPM);
            break;
        }
        case EDIT_TEMPO_STOP: {
            gEditing->openTempoEdit(Segment::STOP);
            break;
        }
        case EDIT_TEMPO_DELAY: {
            gEditing->openTempoEdit(Segment::DELAY);
            break;
        }
        case EDIT_TEMPO_WARP: {
            gEditing->openTempoEdit(Segment::WARP);
            break;
        }
        case EDIT_TEMPO_TIME_SIG: {
            gEditing->openTempoEdit(Segment::TIME_SIG);
            break;
        }
        case EDIT_TEMPO_TICK_COUNT: {
            gEditing->openTempoEdit(Segment::TICK_COUNT);
            break;
        }
        case EDIT_TEMPO_COMBO: {
            gEditing->openTempoEdit(Segment::COMBO);
            break;
        }
        case EDIT_TEMPO_SPEED: {
            gEditing->openTempoEdit(Segment::SPEED);
            break;
        }
        case EDIT_TEMPO_SCROLL: {
            gEditing->openTempoEdit(Segment::SCROLL);
            break;
        }
        case EDIT_TEMPO_FAKE: {
            gEditing->openTempoEdit(Segment::FAKE);
            break;
        }
        case EDIT_TEMPO_LABEL: {
            gEditing->openTempoEdit(Segment::LABEL);
            break;
        }
        case SELECTION_TOGGLE_TEMPO_EDITOR: {
            gSelection->toggleTempoEditor();
            break;
        }

        case CHART_PREVIOUS: {
            gSimfile->previousChart();
            break;
        }
        case CHART_NEXT: {
            gSimfile->nextChart();
            break;
        }
        case CHART_DELETE: {
            gSimfile->removeChart(gChart->get());
            break;
        }

        case SIMFILE_PREVIOUS: {
            gEditor->openNextSimfile(false);
            break;
        }
        case SIMFILE_NEXT: {
            gEditor->openNextSimfile(true);
            break;
        }

        case CHART_CONVERT_COUPLES_TO_ROUTINE: {
            gEditing->convertCouplesToRoutine();
            break;
        }
        case CHART_CONVERT_ROUTINE_TO_COUPLES: {
            gEditing->convertRoutineToCouples();
            break;
        }

        case CHANGE_NOTES_TO_MINES: {
            gEditing->changeNotesToType(NoteType::NOTE_MINE);
            break;
        }
        case CHANGE_NOTES_TO_FAKES: {
            gEditing->changeNotesToType(NoteType::NOTE_FAKE);
            break;
        }
        case CHANGE_NOTES_TO_LIFTS: {
            gEditing->changeNotesToType(NoteType::NOTE_LIFT);
            break;
        }
        case CHANGE_MINES_TO_NOTES: {
            gEditing->changeMinesToType(NoteType::NOTE_STEP_OR_HOLD);
            break;
        }
        case CHANGE_MINES_TO_FAKES: {
            gEditing->changeMinesToType(NoteType::NOTE_FAKE);
            break;
        }
        case CHANGE_MINES_TO_LIFTS: {
            gEditing->changeMinesToType(NoteType::NOTE_LIFT);
            break;
        }
        case CHANGE_FAKES_TO_NOTES: {
            gEditing->changeFakesToType(NoteType::NOTE_STEP_OR_HOLD);
            break;
        }
        case CHANGE_LIFTS_TO_NOTES: {
            gEditing->changeLiftsToType(NoteType::NOTE_STEP_OR_HOLD);
            break;
        }
        case CHANGE_HOLDS_TO_STEPS: {
            gEditing->changeHoldsToType(NoteType::NOTE_STEP_OR_HOLD);
            break;
        }
        case CHANGE_HOLDS_TO_MINES: {
            gEditing->changeHoldsToType(NoteType::NOTE_MINE);
            break;
        }
        case CHANGE_BETWEEN_HOLDS_AND_ROLLS: {
            gEditing->changeHoldsToRolls();
            break;
        }
        case CHANGE_BETWEEN_PLAYER_NUMBERS: {
            gEditing->changePlayerNumber();
            break;
        }
        case CHANGE_NOTE_SIDE: {
            gEditing->changeNoteSide();
            break;
        }

        case MIRROR_NOTES_VERTICALLY: {
            gEditing->mirrorNotes(Editing::MIRROR_V);
            break;
        }
        case MIRROR_NOTES_HORIZONTALLY: {
            gEditing->mirrorNotes(Editing::MIRROR_H);
            break;
        }
        case MIRROR_NOTES_FULL: {
            gEditing->mirrorNotes(Editing::MIRROR_HV);
            break;
        }

        case EXPORT_NOTES_AS_LUA_TABLE: {
            gEditing->exportNotesAsLuaTable();
            break;
        }

        case SCALE_NOTES_2_TO_1: {
            gEditing->scaleNotes(2, 1);
            break;
        }
        case SCALE_NOTES_3_TO_2: {
            gEditing->scaleNotes(3, 2);
            break;
        }
        case SCALE_NOTES_4_TO_3: {
            gEditing->scaleNotes(4, 3);
            break;
        }
        case SCALE_NOTES_1_TO_2: {
            gEditing->scaleNotes(1, 2);
            break;
        }
        case SCALE_NOTES_2_TO_3: {
            gEditing->scaleNotes(2, 3);
            break;
        }
        case SCALE_NOTES_3_TO_4: {
            gEditing->scaleNotes(3, 4);
            break;
        }

        case SWITCH_TO_SYNC_MODE: {
            gSimfile->openChart(-1);
            break;
        }

        case VOLUME_RESET: {
            gMusic->setVolume(100);
            break;
        }
        case VOLUME_INCREASE: {
            gMusic->setVolume(gMusic->getVolume() + 10);
            break;
        }
        case VOLUME_DECREASE: {
            gMusic->setVolume(gMusic->getVolume() - 10);
            break;
        }
        case VOLUME_MUTE: {
            gMusic->setMuted(!gMusic->isMuted());
            break;
        }

        case CONVERT_MUSIC: {
            gMusic->startAudioConversion();
            break;
        }
        case CONVERT_MUSIC_TO_OGG: {
            gMusic->startAudioConversion(AudioFormat::OGG);
            break;
        }
        case CONVERT_MUSIC_TO_MP3: {
            gMusic->startAudioConversion(AudioFormat::MP3);
            break;
        }
        case CONVERT_MUSIC_TO_WAV: {
            gMusic->startAudioConversion(AudioFormat::WAV);
            break;
        }

        case SPEED_RESET: {
            gMusic->setSpeed(100);
            break;
        }
        case SPEED_INCREASE: {
            gMusic->setSpeed(gMusic->getSpeed() + 10);
            break;
        }
        case SPEED_DECREASE: {
            gMusic->setSpeed(gMusic->getSpeed() - 10);
            break;
        }

        case TOGGLE_BEAT_TICK: {
            gMusic->toggleBeatTick();
            break;
        }
        case TOGGLE_NOTE_TICK: {
            gMusic->toggleNoteTick();
            break;
        }

        case TOGGLE_SHOW_WAVEFORM: {
            gNotefield->toggleShowWaveform();
            break;
        }
        case BEATLINE_TOGGLE_ENABLED: {
            gNotefield->toggleShowBeatLines();
            break;
        }
        case BEATLINE_TOGGLE_SNAP: {
            gNotefield->toggleShowBeatLinesSnap();
            break;
        }
        case BEATLINE_TOGGLE_COLOR: {
            gNotefield->toggleShowBeatLinesColor();
            break;
        }
        case BEATLINE_TOGGLE_HOVER: {
            gNotefield->toggleShowBeatLinesHover();
            break;
        }
        case TOGGLE_SHOW_NOTES: {
            gNotefield->toggleShowNotes();
            break;
        }
        case TOGGLE_SHOW_TEMPO_BOXES: {
            gTempoBoxes->toggleShowBoxes();
            break;
        }
        case TOGGLE_SHOW_TEMPO_HELP: {
            gTempoBoxes->toggleShowHelp();
            break;
        }
        case TOGGLE_REVERSE_SCROLL: {
            gView->toggleReverseScroll();
            break;
        }
        case TOGGLE_CHART_PREVIEW: {
            gView->toggleChartPreview();
            break;
        }

        case PREVIEW_TOGGLE_ENABLED: {
            gNotefieldPreview->toggleEnabled();
            break;
        }
        case PREVIEW_TOGGLE_SHOW_BEAT_LINES: {
            gNotefieldPreview->toggleShowBeatLines();
            break;
        }
        case PREVIEW_TOGGLE_REVERSE_SCROLL: {
            gNotefieldPreview->toggleReverseScroll();
            break;
        }
        case PREVIEW_VIEW_CMOD: {
            gNotefieldPreview->setMode(NotefieldPreview::CMOD);
            break;
        }
        case PREVIEW_VIEW_XMOD: {
            gNotefieldPreview->setMode(NotefieldPreview::XMOD);
            break;
        }
        case PREVIEW_VIEW_VARIABLE: {
            gNotefieldPreview->setMode(NotefieldPreview::VARIABLE);
            break;
        }

        case MINIMAP_SET_NOTES: {
            gMinimap->setMode(Minimap::NOTES);
            break;
        }
        case MINIMAP_SET_DENSITY: {
            gMinimap->setMode(Minimap::DENSITY);
            break;
        }

        case BACKGROUND_HIDE: {
            gNotefield->setBgAlpha(0);
            break;
        }
        case BACKGROUND_INCREASE_ALPHA: {
            gNotefield->setBgAlpha(gNotefield->getBgAlpha() + 10);
            break;
        }
        case BACKGROUND_DECREASE_ALPHA: {
            gNotefield->setBgAlpha(gNotefield->getBgAlpha() - 10);
            break;
        }
        case BACKGROUND_SET_STRETCH: {
            gEditor->setBackgroundStyle(BG_STYLE_STRETCH);
            break;
        }
        case BACKGROUND_SET_LETTERBOX: {
            gEditor->setBackgroundStyle(BG_STYLE_LETTERBOX);
            break;
        }
        case BACKGROUND_SET_CROP: {
            gEditor->setBackgroundStyle(BG_STYLE_CROP);
            break;
        }

        case USE_TIME_BASED_VIEW: {
            gView->setTimeBased(true);
            break;
        }
        case USE_ROW_BASED_VIEW: {
            gView->setTimeBased(false);
            break;
        }

        case ZOOM_RESET: {
            gView->setZoomLevel(8);
            gView->setScaleLevel(4);
            break;
        }
        case ZOOM_IN: {
            gView->setZoomLevel(gView->getZoomLevel() + 0.25);
            break;
        }
        case ZOOM_OUT: {
            gView->setZoomLevel(gView->getZoomLevel() - 0.25);
            break;
        }
        case SCALE_INCREASE: {
            gView->setScaleLevel(gView->getScaleLevel() + 0.25);
            break;
        }
        case SCALE_DECREASE: {
            gView->setScaleLevel(gView->getScaleLevel() - 0.25);
            break;
        }

        case SNAP_NEXT: {
            gView->setSnapType(gView->getSnapType() + 1);
            break;
        }
        case SNAP_PREVIOUS: {
            gView->setSnapType(gView->getSnapType() - 1);
            break;
        }
        case SNAP_RESET: {
            gView->setSnapType(0);
            break;
        }

        case CURSOR_UP: {
            gView->setCursorRow(
                gView->snapRow(gView->getCursorRow(), View::SNAP_UP));
            break;
        }
        case CURSOR_DOWN: {
            gView->setCursorRow(
                gView->snapRow(gView->getCursorRow(), View::SNAP_DOWN));
            break;
        }
        case CURSOR_PREVIOUS_BEAT: {
            gView->setCursorToNextInterval(-48);
            break;
        }
        case CURSOR_NEXT_BEAT: {
            gView->setCursorToNextInterval(48);
            break;
        }
        case CURSOR_PREVIOUS_MEASURE: {
            gView->setCursorToNextInterval(-192);
            break;
        }
        case CURSOR_NEXT_MEASURE: {
            gView->setCursorToNextInterval(192);
            break;
        }
        case CURSOR_STREAM_START: {
            gView->setCursorToStream(true);
            break;
        }
        case CURSOR_STREAM_END: {
            gView->setCursorToStream(false);
            break;
        }
        case CURSOR_SELECTION_START: {
            gView->setCursorToSelection(true);
            break;
        }
        case CURSOR_SELECTION_END: {
            gView->setCursorToSelection(false);
            break;
        }
        case CURSOR_CHART_START: {
            gView->setCursorRow(0);
            break;
        }
        case CURSOR_CHART_END: {
            gView->setCursorRow(gSimfile->getEndRow());
            break;
        }

        case TOGGLE_STATUS_CHART: {
            gStatusbar->toggleChart();
            break;
        }
        case TOGGLE_STATUS_SNAP: {
            gStatusbar->toggleSnap();
            break;
        }
        case TOGGLE_STATUS_BPM: {
            gStatusbar->toggleBpm();
            break;
        }
        case TOGGLE_STATUS_ROW: {
            gStatusbar->toggleRow();
            break;
        }
        case TOGGLE_STATUS_BEAT: {
            gStatusbar->toggleBeat();
            break;
        }
        case TOGGLE_STATUS_MEASURE: {
            gStatusbar->toggleMeasure();
            break;
        }
        case TOGGLE_STATUS_HOVER: {
            gStatusbar->toggleHover();
            break;
        }
        case TOGGLE_STATUS_TIME: {
            gStatusbar->toggleTime();
            break;
        }
        case TOGGLE_STATUS_TIMING_MODE: {
            gStatusbar->toggleTimingMode();
            break;
        }
        case TOGGLE_STATUS_SCROLL: {
            gStatusbar->toggleScroll();
            break;
        }
        case TOGGLE_STATUS_SPEED: {
            gStatusbar->toggleSpeed();
            break;
        }

        case SHOW_SHORTCUTS: {
            gTextOverlay->show(TextOverlay::SHORTCUTS);
            break;
        }
        case SHOW_MESSAGE_LOG: {
            gTextOverlay->show(TextOverlay::MESSAGE_LOG);
            break;
        }
        case SHOW_DEBUG_LOG: {
            gTextOverlay->show(TextOverlay::DEBUG_LOG);
            break;
        }
        case SHOW_ABOUT: {
            gTextOverlay->show(TextOverlay::ABOUT);
            break;
        }

        default: {
            handled = false;
            break;
        }
    }
    return handled;
};

};  // namespace Vortex
