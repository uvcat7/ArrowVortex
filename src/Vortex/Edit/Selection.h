#pragma once

#include <Core/Common/Event.h>

#include <Vortex/Common.h>

#include <Simfile/Tempo/SegmentTypes.h>

namespace AV {

struct SelectionRegion { int beginRow, endRow; };

// Determines how a selection is modified.
enum class SelectionModifier
{
	Set, // Replace the current selection.
	Add, // Add to the current selection.
	Sub, // Subtract from the current selection.
};

typedef std::map<Row, SegmentType> SelectedSegments;

namespace Selection
{
	void initialize(const XmrDoc& settings);
	void deinitialize();

	void drawRegionSelection();
	void drawSelectionBox();

	// Returns true if a region is currently selected.
	bool hasRegionSelection();

	// Returns true if one or more notes of the given chart are currently selected.
	bool hasNoteSelection(const Chart* chart);

	// Returns true if one or more segments of the given tempo are currently selected.
	bool hasSegmentSelection(const Tempo* tempo);

	// Selects all notes.
	int selectAllNotes(SelectionModifier mod);

	// Select all notes of a certain type.
	int selectNotes(SelectionModifier mod, NoteType type);

	// Select all notes of a certain quantization.
	int selectNotes(SelectionModifier mod, Quantization quantization);

	// Select all notes within the given row/column region.
	int selectNotes(SelectionModifier mod, RowCol begin, RowCol end);

	// Select all notes that are listed in the given note list.
	int selectNotes(SelectionModifier mod, const NoteSet& set);

	// Adds all currently selected notes to the given set and returns the number of selected notes.
	int getSelectedNotes(NoteSet& out);

	// Returns true if the given note is selected, false otherwise.
	bool hasSelectedNote(int col, const Note& note);

	// Selects all segments.
	int selectAllSegments(SelectionModifier mod);

	// Select all segments that are listed in the given segments list.
	int selectSegments(SelectionModifier mod, const SelectedSegments& segments);

	// Returns a list of all currently selected segments.
	const SelectedSegments& getSelectedSegments();

	// Starts or finishes a region selection at the cursor position.
	void selectRegion();

	// Selects the region defined by the given begin and end row.
	void selectRegion(int beginRow, Row endRow);

	// Returns the currently selected region.
	SelectionRegion getRegion();

	struct SelectionChanged : Event {};
};

} // namespace AV
