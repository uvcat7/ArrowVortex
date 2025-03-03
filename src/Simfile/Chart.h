#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/Common/Event.h>

#include <Simfile/Common.h>
#include <Simfile/NoteSet.h>
#include <Simfile/History.h>

namespace AV {

// Supported difficulties.
enum class Difficulty
{
	Beginner,
	Easy,
	Medium,
	Hard,
	Challenge,
	Edit,

	Count
};

// Returns the name of the given difficulty type.
const char* GetDifficultyName(Difficulty type);

// Returns the color representation of the given difficulty type.
Color GetDifficultyColor(Difficulty dt);

// An intended chart modification.
struct ChartModification
{
	ChartModification(int numCols);

	// Determines which notes are removed in addition to notesToRemove.
	enum class RemovePolicy
	{
		// Only removes notes that directly intersect the added notes.
		OverlappingNotes,

		// Removes all notes on rows that intersect the added notes.
		OverlappingRows,

		// Removes all notes within or intersecting the modified region.
		EntireRegion,
	};

	// Message shown on the HUD after the modification is undone/redone.
	struct Description
	{
		// Description used when a single note is modified (e.g. "mirrored %1 note").
		const char* singular;

		// Description used when multiple notes are modified (e.g. "mirrored %2 notes").
		const char* plural;
	};

	// Notes that are intended to be added.
	// After the modification, this contains the notes that were actually added.
	NoteSet notesToAdd;

	// Notes that are intended to be removed.
	// After the modification, this contains the notes that were actually removed.
	NoteSet notesToRemove;

	// Determines which notes are removed in addition to notesToRemove.
	// The default behaviour is REMOVE_OVERLAPPING_NOTES.
	RemovePolicy removePolicy;

	// An optional description of the edit that is attached to the history entry.
	const Description* description;
};

// Holds data for a chart.
class Chart : NonCopyable
{
public:
	~Chart();

	// Creates a blank chart and sets its simfile.
	Chart(Simfile* owner, const GameMode* mode);

	// Sanitizes the notes and tempo, and makes sure the chart parameters are valid.
	void sanitize();

	// Returns the difficulty and meter of the chart (e.g. "Challenge 12").
	std::string getFullDifficulty() const;

	// Returns the chart tempo if the chart has its own tempo, or the simfile tempo otherwise.
	Tempo& getTempo();

	// Returns the chart tempo if the chart has its own tempo, or the simfile tempo otherwise.
	const Tempo& getTempo() const;

	// Returns the total number of notes in the chart, excluding mines.
	int getStepCount() const;

	// Modifies the chart notes.
	// The resulting modifications are written back to the modification that was passed.
	void modify(ChartModification& mod);

// Events:

	struct NotesChangedEvent : Event {};
	struct NameChangedEvent : Event {};
	struct StyleChangedEvent : Event {};
	struct AuthorChangedEvent : Event {};
	struct DescriptionChangedEvent : Event {};
	struct DifficultyChangedEvent : Event {};
	struct MeterChangedEvent : Event {};
	struct TimingChanged : Event {}; // One or more properties that affect timing changed.
	struct SegmentsChanged : Event {}; // One or more segments have changed.

// Properties:

	// The note data of the chart.
	NoteSet notes;

	// The tempo of the chart if the chart has a split tempo.
	// If tempo is null, the chart uses the simfile tempo.
	shared_ptr<Tempo> tempo;

	// The game mode (e.g. dance-single, pump-double).
	const GameMode* const gameMode;

	// The simfile that the chart is part of.
	const Simfile* const simfile;

	// The editing history.
	ChartHistory* history;

	// The chart name.
	Observable<std::string> name;

	// The chart style (e.g. "Keyboard", "Pad").
	Observable<std::string> style;

	// The author of the steps.
	Observable<std::string> author;

	// The chart description
	Observable<std::string> description;

	// The chart difficulty (e.g. Easy, Challenge, Edit).
	Observable<Difficulty> difficulty;

	// The chart meter (i.e. block rating, difficulty rating).
	Observable<int> meter;

	// The radar values.
	vector<double> radar;

	// Properties read from the chart that are unknown.
	vector<std::pair<std::string, std::string>> misc;

};

} // namespace AV
