#include <Editor/Action.h>

#include <System/System.h>

#include <Editor/Editor.h>
#include <Editor/Menubar.h>
#include <Editor/View.h>
#include <Editor/History.h>
#include <Editor/Statusbar.h>
#include <Editor/Editing.h>
#include <Editor/Notefield.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/Waveform.h>
#include <Editor/Selection.h>
#include <Editor/TextOverlay.h>
#include <Editor/Music.h>
#include <Editor/Minimap.h>
#include <Editor/TempoBoxes.h>

#include <Managers/MetadataMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/StyleMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/TempoMan.h>

#include <Dialogs/Dialog.h>

namespace Vortex {

void Action::perform(Type action)
{
	#define FIRST_CASE(x) case x: {
	#define CASE(x) } break; case x: {

	switch(action)
	{
	FIRST_CASE(EXIT_PROGRAM)
		gEditor->onExitProgram();

	CASE(FILE_OPEN)
		gEditor->openSimfile();
	CASE(FILE_SAVE)
		gEditor->saveSimfile(false);
	CASE(FILE_SAVE_AS)
		gEditor->saveSimfile(true);
	CASE(FILE_CLOSE)
		gEditor->closeSimfile();

	CASE(FILE_CLEAR_RECENT_FILES)
		gEditor->clearRecentFiles();

	CASE(OPEN_DIALOG_SONG_PROPERTIES)
		gEditor->openDialog(DIALOG_SONG_PROPERTIES);
	CASE(OPEN_DIALOG_CHART_PROPERTIES)
		gEditor->openDialog(DIALOG_CHART_PROPERTIES);
	CASE(OPEN_DIALOG_CHART_LIST)
		gEditor->openDialog(DIALOG_CHART_LIST);
	CASE(OPEN_DIALOG_NEW_CHART)
		gEditor->openDialog(DIALOG_NEW_CHART);
	CASE(OPEN_DIALOG_ADJUST_SYNC)
		gEditor->openDialog(DIALOG_ADJUST_SYNC);
	CASE(OPEN_DIALOG_ADJUST_TEMPO)
		gEditor->openDialog(DIALOG_ADJUST_TEMPO);
	CASE(OPEN_DIALOG_ADJUST_TEMPO_SM5)
		gEditor->openDialog(DIALOG_ADJUST_TEMPO_SM5);
	CASE(OPEN_DIALOG_DANCING_BOT)
		gEditor->openDialog(DIALOG_DANCING_BOT);
	CASE(OPEN_DIALOG_GENERATE_NOTES)
		gEditor->openDialog(DIALOG_GENERATE_NOTES);
	CASE(OPEN_DIALOG_TEMPO_BREAKDOWN)
		gEditor->openDialog(DIALOG_TEMPO_BREAKDOWN);
	CASE(OPEN_DIALOG_LABEL_BREAKDOWN)
		gEditor->openDialog(DIALOG_LABEL_BREAKDOWN);
	CASE(OPEN_DIALOG_WAVEFORM_SETTINGS)
		gEditor->openDialog(DIALOG_WAVEFORM_SETTINGS);
	CASE(OPEN_DIALOG_ZOOM)
		gEditor->openDialog(DIALOG_ZOOM);
	CASE(OPEN_DIALOG_CUSTOM_SNAP)
		gEditor->openDialog(DIALOG_CUSTOM_SNAP);

	CASE(EDIT_UNDO)
		gSystem->getEvents().addKeyPress(Key::Z, Keyflag::CTRL, false);
	CASE(EDIT_REDO)
		gSystem->getEvents().addKeyPress(Key::Y, Keyflag::CTRL, false);
	CASE(EDIT_CUT)
		gSystem->getEvents().addKeyPress(Key::X, Keyflag::CTRL, false);
	CASE(EDIT_COPY)
		gSystem->getEvents().addKeyPress(Key::C, Keyflag::CTRL, false);
	CASE(EDIT_PASTE)
		gSystem->getEvents().addKeyPress(Key::V, Keyflag::CTRL, false);
	CASE(EDIT_DELETE)
		gSystem->getEvents().addKeyPress(Key::DELETE, 0, false);
	CASE(SELECT_ALL)
		gSystem->getEvents().addKeyPress(Key::A, Keyflag::CTRL, false);

	CASE(TOGGLE_JUMP_TO_NEXT_NOTE)
		gEditing->toggleJumpToNextNote();
	CASE(TOGGLE_UNDO_REDO_JUMP)
		gEditing->toggleUndoRedoJump();
	CASE(TOGGLE_TIME_BASED_COPY)
		gEditing->toggleTimeBasedCopy();

	CASE(SET_VISUAL_SYNC_CURSOR_ANCHOR)
		gEditing->setVisualSyncAnchor(Editing::VisualSyncAnchor::CURSOR);
	CASE(SET_VISUAL_SYNC_RECEPTOR_ANCHOR)
		gEditing->setVisualSyncAnchor(Editing::VisualSyncAnchor::RECEPTORS);
	CASE(INJECT_BOUNDING_BPM_CHANGE)
		gEditing->injectBoundingBpmChange();
	CASE(SHIFT_ROW_NONDESTRUCTIVE)
		gEditing->shiftAnchorRowToMousePosition(false);
	CASE(SHIFT_ROW_DESTRUCTIVE)
		gEditing->shiftAnchorRowToMousePosition(true);

	CASE(SELECT_REGION)
		gSelection->selectRegion();
	CASE(SELECT_ALL_STEPS)
		gSelection->selectNotes(NotesMan::SELECT_STEPS);
	CASE(SELECT_ALL_MINES)
		gSelection->selectNotes(NotesMan::SELECT_MINES);
	CASE(SELECT_ALL_HOLDS)
		gSelection->selectNotes(NotesMan::SELECT_HOLDS);
	CASE(SELECT_ALL_ROLLS)
		gSelection->selectNotes(NotesMan::SELECT_ROLLS);
	CASE(SELECT_ALL_FAKES)
		gSelection->selectNotes(NotesMan::SELECT_FAKES);
	CASE(SELECT_ALL_LIFTS)
		gSelection->selectNotes(NotesMan::SELECT_LIFTS);
	CASE(SELECT_REGION_BEFORE_CURSOR)
		gSelection->selectRegion(0, gView->getCursorRow());
	CASE(SELECT_REGION_AFTER_CURSOR)
		gSelection->selectRegion(gView->getCursorRow(), gSimfile->getEndRow());

	CASE(SELECT_QUANT_4)
		gSelection->selectNotes(RT_4TH);
	CASE(SELECT_QUANT_8)
		gSelection->selectNotes(RT_8TH);
	CASE(SELECT_QUANT_12)
		gSelection->selectNotes(RT_12TH);
	CASE(SELECT_QUANT_16)
		gSelection->selectNotes(RT_16TH);
	CASE(SELECT_QUANT_24)
		gSelection->selectNotes(RT_24TH);
	CASE(SELECT_QUANT_32)
		gSelection->selectNotes(RT_32ND);
	CASE(SELECT_QUANT_48)
		gSelection->selectNotes(RT_48TH);
	CASE(SELECT_QUANT_64)
		gSelection->selectNotes(RT_64TH);
	CASE(SELECT_QUANT_192)
		gSelection->selectNotes(RT_192TH);

	CASE(SELECT_TEMPO_BPM)
		gTempoBoxes->selectType(Segment::BPM);
	CASE(SELECT_TEMPO_STOP)
		gTempoBoxes->selectType(Segment::STOP);
	CASE(SELECT_TEMPO_DELAY)
		gTempoBoxes->selectType(Segment::DELAY);
	CASE(SELECT_TEMPO_WARP)
		gTempoBoxes->selectType(Segment::WARP);
	CASE(SELECT_TEMPO_TIME_SIG)
		gTempoBoxes->selectType(Segment::TIME_SIG);
	CASE(SELECT_TEMPO_TICK_COUNT)
		gTempoBoxes->selectType(Segment::TICK_COUNT);
	CASE(SELECT_TEMPO_COMBO)
		gTempoBoxes->selectType(Segment::COMBO);
	CASE(SELECT_TEMPO_SPEED)
		gTempoBoxes->selectType(Segment::SPEED);
	CASE(SELECT_TEMPO_SCROLL)
		gTempoBoxes->selectType(Segment::SCROLL);
	CASE(SELECT_TEMPO_FAKE)
		gTempoBoxes->selectType(Segment::FAKE);
	CASE(SELECT_TEMPO_LABEL)
		gTempoBoxes->selectType(Segment::LABEL);

	CASE(CHART_PREVIOUS)
		gSimfile->previousChart();
	CASE(CHART_NEXT)
		gSimfile->nextChart();
	CASE(CHART_DELETE)
		gSimfile->removeChart(gChart->get());

	CASE(SIMFILE_PREVIOUS)
		gEditor->openNextSimfile(false);
	CASE(SIMFILE_NEXT)
		gEditor->openNextSimfile(true);

	CASE(CHART_CONVERT_COUPLES_TO_ROUTINE)
		gEditing->convertCouplesToRoutine();
	CASE(CHART_CONVERT_ROUTINE_TO_COUPLES)
		gEditing->convertRoutineToCouples();

	CASE(CHANGE_NOTES_TO_MINES)
		gEditing->changeNotesToType(NoteType::NOTE_MINE);
	CASE(CHANGE_NOTES_TO_FAKES)
		gEditing->changeNotesToType(NoteType::NOTE_FAKE);
	CASE(CHANGE_NOTES_TO_LIFTS)
		gEditing->changeNotesToType(NoteType::NOTE_LIFT);
	CASE(CHANGE_MINES_TO_NOTES)
		gEditing->changeMinesToType(NoteType::NOTE_STEP_OR_HOLD);
	CASE(CHANGE_MINES_TO_FAKES)
		gEditing->changeMinesToType(NoteType::NOTE_FAKE);
	CASE(CHANGE_MINES_TO_LIFTS)
		gEditing->changeMinesToType(NoteType::NOTE_LIFT);
	CASE(CHANGE_FAKES_TO_NOTES)
		gEditing->changeFakesToType(NoteType::NOTE_STEP_OR_HOLD);
	CASE(CHANGE_LIFTS_TO_NOTES)
		gEditing->changeLiftsToType(NoteType::NOTE_STEP_OR_HOLD);
	CASE(CHANGE_HOLDS_TO_STEPS)
		gEditing->changeHoldsToType(NoteType::NOTE_STEP_OR_HOLD);
	CASE(CHANGE_HOLDS_TO_MINES)
		gEditing->changeHoldsToType(NoteType::NOTE_MINE);
	CASE(CHANGE_BETWEEN_HOLDS_AND_ROLLS)
		gEditing->changeHoldsToRolls();
	CASE(CHANGE_BETWEEN_PLAYER_NUMBERS)
		gEditing->changePlayerNumber();

	CASE(MIRROR_NOTES_VERTICALLY)
		gEditing->mirrorNotes(Editing::MIRROR_V);
	CASE(MIRROR_NOTES_HORIZONTALLY)
		gEditing->mirrorNotes(Editing::MIRROR_H);
	CASE(MIRROR_NOTES_FULL)
		gEditing->mirrorNotes(Editing::MIRROR_HV);

	CASE(EXPORT_NOTES_AS_LUA_TABLE)
		gEditing->exportNotesAsLuaTable();

	CASE(SCALE_NOTES_2_TO_1)
		gEditing->scaleNotes(2, 1);
	CASE(SCALE_NOTES_3_TO_2)
		gEditing->scaleNotes(3, 2);
	CASE(SCALE_NOTES_4_TO_3)
		gEditing->scaleNotes(4, 3);
	CASE(SCALE_NOTES_1_TO_2)
		gEditing->scaleNotes(1, 2);
	CASE(SCALE_NOTES_2_TO_3)
		gEditing->scaleNotes(2, 3);
	CASE(SCALE_NOTES_3_TO_4)
		gEditing->scaleNotes(3, 4);

	CASE(SWITCH_TO_SYNC_MODE)
		gSimfile->openChart(-1);

	CASE(VOLUME_RESET)
		gMusic->setVolume(100);
	CASE(VOLUME_INCREASE)
		gMusic->setVolume(gMusic->getVolume() + 10);
	CASE(VOLUME_DECREASE)
		gMusic->setVolume(gMusic->getVolume() - 10);
	CASE(VOLUME_MUTE)
		gMusic->setMuted(!gMusic->isMuted());

	CASE(CONVERT_MUSIC_TO_OGG)
		gMusic->startOggConversion();

	CASE(SPEED_RESET)
		gMusic->setSpeed(100);
	CASE(SPEED_INCREASE)
		gMusic->setSpeed(gMusic->getSpeed() + 10);
	CASE(SPEED_DECREASE)
		gMusic->setSpeed(gMusic->getSpeed() - 10);

	CASE(TOGGLE_BEAT_TICK)
		gMusic->toggleBeatTick();
	CASE(TOGGLE_NOTE_TICK)
		gMusic->toggleNoteTick();

	CASE(TOGGLE_SHOW_WAVEFORM)
		gNotefield->toggleShowWaveform();
	CASE(TOGGLE_SHOW_BEAT_LINES)
		gNotefield->toggleShowBeatLines();
	CASE(TOGGLE_SHOW_NOTES)
		gNotefield->toggleShowNotes();
	CASE(TOGGLE_SHOW_TEMPO_BOXES)
		gTempoBoxes->toggleShowBoxes();
	CASE(TOGGLE_SHOW_TEMPO_HELP)
		gTempoBoxes->toggleShowHelp();
	CASE(TOGGLE_REVERSE_SCROLL)
		gView->toggleReverseScroll();
	CASE(TOGGLE_CHART_PREVIEW)
		gView->toggleChartPreview();

	CASE(PREVIEW_TOGGLE_ENABLED)
		gNotefieldPreview->toggleEnabled();
	CASE(PREVIEW_TOGGLE_SHOW_BEAT_LINES)
		gNotefieldPreview->toggleShowBeatLines();
	CASE(PREVIEW_TOGGLE_REVERSE_SCROLL)
		gNotefieldPreview->toggleReverseScroll();
	CASE(PREVIEW_VIEW_CMOD)
		gNotefieldPreview->setMode(NotefieldPreview::CMOD);
	CASE(PREVIEW_VIEW_XMOD)
		gNotefieldPreview->setMode(NotefieldPreview::XMOD);
	CASE(PREVIEW_VIEW_XMOD_ALL)
		gNotefieldPreview->setMode(NotefieldPreview::XMOD_ALL);

	CASE(MINIMAP_SET_NOTES)
		gMinimap->setMode(Minimap::NOTES);
	CASE(MINIMAP_SET_DENSITY)
		gMinimap->setMode(Minimap::DENSITY);

	CASE(BACKGROUND_HIDE)
		gNotefield->setBgAlpha(0);
	CASE(BACKGROUND_INCREASE_ALPHA)
		gNotefield->setBgAlpha(gNotefield->getBgAlpha() + 10);
	CASE(BACKGROUND_DECREASE_ALPHA)
		gNotefield->setBgAlpha(gNotefield->getBgAlpha() - 10);
	CASE(BACKGROUND_SET_STRETCH)
		gEditor->setBackgroundStyle(BG_STYLE_STRETCH);
	CASE(BACKGROUND_SET_LETTERBOX)
		gEditor->setBackgroundStyle(BG_STYLE_LETTERBOX);
	CASE(BACKGROUND_SET_CROP)
		gEditor->setBackgroundStyle(BG_STYLE_CROP);

	CASE(USE_TIME_BASED_VIEW)
		gView->setTimeBased(true);
	CASE(USE_ROW_BASED_VIEW)
		gView->setTimeBased(false);

	CASE(ZOOM_RESET)
		gView->setZoomLevel(8);
		gView->setScaleLevel(4);
	CASE(ZOOM_IN)
		gView->setZoomLevel(gView->getZoomLevel() + 0.25);
	CASE(ZOOM_OUT)
		gView->setZoomLevel(gView->getZoomLevel() - 0.25);
	CASE(SCALE_INCREASE)
		gView->setScaleLevel(gView->getScaleLevel() + 0.25);
	CASE(SCALE_DECREASE)
		gView->setScaleLevel(gView->getScaleLevel() - 0.25);

	CASE(SNAP_NEXT)
		gView->setSnapType(gView->getSnapType() + 1);
	CASE(SNAP_PREVIOUS)
		gView->setSnapType(gView->getSnapType() - 1);
	CASE(SNAP_RESET)
		gView->setSnapType(0);

	CASE(CURSOR_UP)
		gView->setCursorRow(gView->snapRow(gView->getCursorRow(), View::SNAP_UP));
	CASE(CURSOR_DOWN)
		gView->setCursorRow(gView->snapRow(gView->getCursorRow(), View::SNAP_DOWN));
	CASE(CURSOR_PREVIOUS_BEAT)
		gView->setCursorToNextInterval(-48);
	CASE(CURSOR_NEXT_BEAT)
		gView->setCursorToNextInterval(48);
	CASE(CURSOR_PREVIOUS_MEASURE)
		gView->setCursorToNextInterval(-192);
	CASE(CURSOR_NEXT_MEASURE)
		gView->setCursorToNextInterval(192);
	CASE(CURSOR_STREAM_START)
		gView->setCursorToStream(true);
	CASE(CURSOR_STREAM_END)
		gView->setCursorToStream(false);
	CASE(CURSOR_SELECTION_START)
		gView->setCursorToSelection(true);
	CASE(CURSOR_SELECTION_END)
		gView->setCursorToSelection(false);
	CASE(CURSOR_CHART_START)
		gView->setCursorRow(0);
	CASE(CURSOR_CHART_END)
		gView->setCursorRow(gSimfile->getEndRow());

	CASE(TOGGLE_STATUS_CHART)
		gStatusbar->toggleChart();
	CASE(TOGGLE_STATUS_SNAP)
		gStatusbar->toggleSnap();
	CASE(TOGGLE_STATUS_BPM)
		gStatusbar->toggleBpm();
	CASE(TOGGLE_STATUS_ROW)
		gStatusbar->toggleRow();
	CASE(TOGGLE_STATUS_BEAT)
		gStatusbar->toggleBeat();
	CASE(TOGGLE_STATUS_MEASURE)
		gStatusbar->toggleMeasure();
	CASE(TOGGLE_STATUS_TIME)
		gStatusbar->toggleTime();
	CASE(TOGGLE_STATUS_TIMING_MODE)
		gStatusbar->toggleTimingMode();
	CASE(TOGGLE_STATUS_SCROLL)
		gStatusbar->toggleScroll();
	CASE(TOGGLE_STATUS_SPEED)
		gStatusbar->toggleSpeed();

	CASE(SHOW_SHORTCUTS)
		gTextOverlay->show(TextOverlay::SHORTCUTS);
	CASE(SHOW_MESSAGE_LOG)
		gTextOverlay->show(TextOverlay::MESSAGE_LOG);
	CASE(SHOW_DEBUG_LOG)
		gTextOverlay->show(TextOverlay::DEBUG_LOG);
	CASE(SHOW_ABOUT)
		gTextOverlay->show(TextOverlay::ABOUT);
	}};

	if(action >= FILE_OPEN_RECENT_BEGIN && action < FILE_OPEN_RECENT_END)
	{
		gEditor->openSimfile(action - FILE_OPEN_RECENT_BEGIN);
	}
	if(action >= SET_NOTESKIN_BEGIN && action < SET_NOTESKIN_END)
	{
		gNoteskin->setType(action - SET_NOTESKIN_BEGIN);
	}
}

}; // namespace Vortex
