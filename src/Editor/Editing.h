#pragma once

#include <Core/Input.h>

#include <Managers/ChartMan.h>

namespace Vortex {

struct Editing : public InputHandler
{
	enum MirrorType
	{
		MIRROR_H,
		MIRROR_V,
		MIRROR_HV,
	};

	static void create(XmrNode& settings);
	static void destroy();

	virtual void saveSettings(XmrNode& settings) = 0;

	/// Called by the editor when changes were made to the simfile.
	virtual void onChanges(int changes) = 0;

	virtual void drawGhostNotes() = 0;

	virtual void deleteSelection() = 0;
	
	virtual void changeHoldsToSteps() = 0;
	virtual void changeNotesToMines() = 0;
	virtual void changeHoldsToRolls() = 0;
	virtual void changePlayerNumber() = 0;

	virtual void mirrorNotes(MirrorType type) = 0;
	virtual void scaleNotes(int numerator, int denominator) = 0;

	virtual void insertRows(int row, int numRows, bool curChartOnly) = 0;

	virtual void convertCouplesToRoutine() = 0;
	virtual void convertRoutineToCouples() = 0;

	virtual void exportNotesAsLuaTable() = 0;

	virtual void toggleJumpToNextNote() = 0;
	virtual bool hasJumpToNextNote() = 0;

	virtual void toggleUndoRedoJump() = 0;
	virtual bool hasUndoRedoJump() = 0;

	virtual void toggleTimeBasedCopy() = 0;
	virtual bool hasTimeBasedCopy() = 0;
};

extern Editing* gEditing;

}; // namespace Vortex