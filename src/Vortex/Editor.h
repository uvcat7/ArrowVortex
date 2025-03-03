#pragma once

#include <Precomp.h>

#include <Core/System/File.h>
#include <Core/Common/Event.h>

#include <Simfile/Formats.h>

namespace AV {
namespace Editor {

void initialize(const XmrDoc& settings);
void deinitialize();

// Tries to close the simfile. If there are unsaved changes and the user selects cancel when
// prompted, then the simfile is not closed and false is returned. Otherwise, the simfile is
// closed and true is returned.
bool closeSimfile();

// Imports and returns the simfile from the given path, which can be either a directory or file.
SimfileFormats::LoadResult importSimfile(stringref path);

// Exports the given simfile.
SimfileFormats::SaveResult exportSimfile(
	const shared_ptr<Simfile>& sim,
	SimfileFormats::Exporter* exporter,
	bool backupOnSave);

// Prompts the user to select a file, and calls "openSimfile" with the resulting path.
bool openSimfile();

// Closes the current simfile and loads a new simfile from the given file. If closing is aborted
// or loading fails, false is returned. If loading succeeds, true is returned.
bool openSimfile(stringref path);

// Calls "openSimfile" with the path of the next (or previous) simfile in the current pack.
// If finding or loading the simfile failed, false is returned. Otherwise, true is returned.
bool openNextSimfile(bool iterateForward);

// Tries to save the simfile. Returns true if saving succeeds, false otherwise.
bool saveSimfile(bool showSaveAsDialog);

// Opens sync mode.
void openSyncMode();

// Opens one of the charts in the simfile for editing.
void openChart(int index);

// Opens one of the charts in the simfile for editing.
void openChart(const Chart* chart);

// Opens the next chart in the simfile for editing.
void nextChart();

// Opens the previous chart in the simfile for editing.
void previousChart();

// Returns the simfile that is currently open for editing.
Chart* currentChart();

// Returns the chart that is currently open for editing.
Simfile* currentSimfile();

// Returns a shared pointer to the simfile that is currently open for editing.
shared_ptr<Simfile> currentSimfileReference();

// Returns the tempo of the simfile or chart that is currently open for editing.
Tempo* currentTempo();

// Empties the recently opened files list.
void clearRecentFiles();

// Adds a path to the list of recent files.
void addToRecentfiles(const FilePath& path);

// Returns the path of a recently opened file.
std::span<const FilePath> getRecentFiles();

// Events:
struct ModeChanged : Event {};
struct RecentFilesChanged : Event {};
struct ActiveChartChanged : Event {};
struct ActiveTempoChanged : Event {};
struct ActiveSimfileChanged : Event {};

} // namespace Editor
} // namespace AV
