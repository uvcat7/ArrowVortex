#include <Precomp.h>

#include <Vortex/Editor.h>

#include <Core/Utils/Xmr.h>
#include <Core/Utils/Unicode.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Vector.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>
#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Shader.h>

#include <Simfile/Simfile.h>
#include <Simfile/NoteSet.h>
#include <Simfile/NoteIterator.h>
#include <Simfile/Tempo.h>
#include <Simfile/History.h>
#include <Simfile/GameMode.h>
#include <Simfile/Formats.h>

#include <Vortex/Application.h>
#include <Vortex/VortexUtils.h>

#include <Vortex/Lua/Global.h>
#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Noteskin/NoteskinMan.h>

#include <Vortex/Edit/RatingEstimator.h>

#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

namespace AV {

using namespace std;

// ====================================================================================================================
// Static data

namespace Editor
{
	struct State
	{
		int lastUsedLoadFilter = 0;
		int lastUsedSaveFilter = 0;
	
		vector<FilePath> recentFiles;
	
		shared_ptr<Simfile> currentSimfile;
		shared_ptr<Chart> currentChart;
		Tempo* currentTempo = nullptr;
	
		vector<unique_ptr<SimfileFormats::Importer>> importers;
		vector<unique_ptr<SimfileFormats::Exporter>> exporters;
	
		bool backupOnSave = false;
	};
	static State* state = nullptr;
}
using Editor::state;

// =====================================================================================================================
// Recent files.

static constexpr int MaxRecentFiles = 10;

static void TruncateRecentFiles()
{
	if (state->recentFiles.size() > MaxRecentFiles)
		state->recentFiles.resize(MaxRecentFiles);
}

static void LoadRecentFiles()
{
	vector<string> lines;
	FileSystem::readLines("settings/recent files.txt", lines);
	for (auto& line : lines)
		state->recentFiles.emplace_back(line);
	TruncateRecentFiles();
}

static void SaveRecentFiles()
{
	FileWriter out;
	if (out.open("settings/recent files.txt"))
	{
		for (int i = 0; i < (int)state->recentFiles.size(); ++i)
		{
			auto& path = state->recentFiles[i];
			out.write(path.str.data(), 1, path.str.length());
			if (i != (int)state->recentFiles.size() - 1)
			{
				out.write("\n", 1, 1);
			}
		}
	}
}

void Editor::addToRecentfiles(const FilePath& path)
{
	Vector::eraseValues(state->recentFiles, path);
	state->recentFiles.insert(state->recentFiles.begin(), path);
	TruncateRecentFiles();
	EventSystem::publish<RecentFilesChanged>();
}

void Editor::clearRecentFiles()
{
	state->recentFiles.clear();
	EventSystem::publish<RecentFilesChanged>();
}

span<const FilePath> Editor::getRecentFiles()
{
	return { state->recentFiles.begin(), state->recentFiles.end() };
}

// ====================================================================================================================
// Initialization

void Editor::initialize(const XmrDoc& settings)
{
	state = new State();

	state->importers.emplace_back(make_unique<SimfileFormats::ImporterSsc>());
	state->importers.emplace_back(make_unique<SimfileFormats::ImporterSm>());
	state->importers.emplace_back(make_unique<SimfileFormats::ImporterOgg>());
	state->importers.emplace_back(make_unique<SimfileFormats::ImporterMp3>());
	state->importers.emplace_back(make_unique<SimfileFormats::ImporterWav>());

	state->exporters.emplace_back(make_unique<SimfileFormats::ExporterSm>());
	state->exporters.emplace_back(make_unique<SimfileFormats::ExporterSsc>());

	Lua::loadImportersExporters(state->importers, state->exporters);

	LoadRecentFiles();

	//openSimfile("D:\\Development\\ArrowVortex\\test\\sm\\Alchemist (Double)\\alchemist.sm");
	//openSimfile("D:\\Installed Games\\OpenITG\\Songs\\Really Long Stuff\\90,000 Miles\\90000 Miles.sm");
	//openDialog(DialogId::NEW_CHART);
}

void Editor::deinitialize()
{
	SaveRecentFiles();
	Util::reset(state);
}

// ====================================================================================================================
// Close simfile.

bool Editor::closeSimfile()
{
	auto sim = state->currentSimfile;
	if (!sim) return true;

	// Check if the user wants to discard unsaved changes.
	if (sim->hasUnsavedChanges())
	{
		auto title = sim->title.get();
		if (title.empty()) title = "the current file";
		auto msg = format("Do you want to save changes to {}?", title);

		auto res = Window::showMessageDlg("ArrowVortex", msg, Window::Button::YesNoCancel, Window::Icon::None);
		if (res == Window::Result::Cancel)
		{
			return false;
		}
		else if (res == Window::Result::Yes)
		{
			Editor::saveSimfile(false);
		}
	}

	bool hadChart = state->currentChart != nullptr;
	bool hadTempo = state->currentTempo != nullptr;

	state->currentSimfile = nullptr;
	state->currentChart = nullptr;
	state->currentTempo = nullptr;

	state->backupOnSave = false;

	MusicMan::unload();

	if (hadChart)
		EventSystem::publish<ActiveChartChanged>();

	if (hadTempo)
		EventSystem::publish<ActiveTempoChanged>();

	EventSystem::publish<ActiveSimfileChanged>();

	return true;
}

// ====================================================================================================================
// Open/save functionality.

static SimfileFormats::LoadResult LoadSimfileCore(const SimfileFormats::Importer& importer, const FilePath& path)
{
	auto result = importer.load(path);
	if (holds_alternative<shared_ptr<Simfile>>(result))
	{
		auto sim = get<shared_ptr<Simfile>>(result);
		sim->directory = path.directory();
		sim->filename = path.stem();
		sim->format = importer.fmt;
	}
	return result;
}

static string ConstructFilter(string format, const vector<string>& extensions)
{
	if (extensions.empty())
		return format.append("\0*.*\0", 5);

	auto last = extensions.end() - 1;
	if (extensions.size() < 6)
	{
		format.append(" (*.");
		for (auto it = extensions.begin(); it != extensions.end(); ++it)
		{
			format.append(*it);
			if (it != last)
				format.append("; *.");
			else
				format.push_back(')');
		}
	}

	format.append("\0*.", 3);
	for (auto it = extensions.begin(); it != extensions.end(); ++it)
	{
		format.append(*it);
		if (it != last)
			format.append(";*.");
		else
			format.push_back('\0');
	}

	return format;
}

static string ConstructImportFilterText()
{
	string result;
	set<string> extensionSet;
	vector<string> extensionList;

	for (auto& importer : state->importers)
	{
		for (auto& ext : importer->extensions)
			if (extensionSet.insert(ext).second)
				extensionList.emplace_back(ext);
		result.append(ConstructFilter(importer->fmt, importer->extensions));
	}

	result.insert(0, ConstructFilter("All Supported Files", extensionList));
	result.append(ConstructFilter("All Files", { "*" }));
	return result;
}

static string ConstructExportFilterText()
{
	string result;

	for (auto& exporter : state->exporters)
		result.append(ConstructFilter(exporter->fmt, exporter->extensions));

	return result;
}

static SimfileFormats::LoadResult LoadSimfile(const FilePath& path, int filterIndex)
{
	// Filter index is 1-based and the first filter is "Supported Files", so importers start at 2.
	filterIndex -= 2;
	if (filterIndex >= 0 && filterIndex < state->importers.size())
		return LoadSimfileCore(*state->importers[filterIndex].get(), path);

	// If no specific importer was chosen, use file extension(s) to decide.
	return Editor::importSimfile(path.str);
}

static bool OpenSimfile(const SimfileFormats::LoadResult& loadResult)
{
	// Check if user agrees to close the current simfile, if applicable.
	if (!Editor::closeSimfile())
		return false;

	// Try to load the simfile.
	if (holds_alternative<string>(loadResult))
	{
		Hud::error("Invalid simfile: %s", get<string>(loadResult).data());
		return false;
	}

	auto sim = get<shared_ptr<Simfile>>(loadResult);
	Hud::info("Opening simfile: %s", sim->filename.data());
	MusicMan::load(sim.get());
	state->backupOnSave = true;

	// Clean up any potential invalid data in the simfile after loading.
	sim->sanitize();
	auto tempo = sim->tempo;

	// Open the loaded simfile.
	state->currentSimfile = sim;
	state->currentTempo = sim->tempo.get();

	// Open the last non-edit chart of the style with the highest priority.
	uint lowestIndex = 0;
	state->currentChart = nullptr;
	for (auto chart : sim->charts)
	{
		if (!state->currentChart)
		{
			state->currentChart = chart;
			lowestIndex = chart->gameMode->index;
		}
		else if (chart->gameMode->index <= lowestIndex && chart->difficulty.get() != Difficulty::Edit)
		{
			state->currentChart = chart;
			lowestIndex = chart->gameMode->index;
		}
	}

	if (state->currentChart)
	{
		double rating = RatingEstimator::estimateRating(state->currentChart.get());
		Hud::note("Estimate rating: %f", rating);
	}

	View::setCursorTime(0.0);

	EventSystem::publish<Editor::ActiveChartChanged>();

	return true;
}

SimfileFormats::LoadResult Editor::importSimfile(stringref path)
{
	map<string, FilePath> extToFile;
	if (FileSystem::isDirectory(path))
	{
		auto dir = DirectoryPath(path);
		for (auto& file : FileSystem::listFiles(dir))
		{
			auto filePath = FilePath(dir, file);
			extToFile.insert(pair(filePath.extension(), filePath));
		}
	}
	else
	{
		auto filePath = FilePath(path);
		extToFile.insert(pair(filePath.extension(), filePath));
	}

	for (auto& importer : state->importers)
	{
		for (auto& ext : importer->extensions)
		{
			auto it = extToFile.find(ext);
			if (it != extToFile.end())
				return LoadSimfileCore(*importer, it->second);
		}
	}

	// If a file was given and no importer was found, default to the first loader (SM).
	if (extToFile.size() == 1)
		return LoadSimfileCore(*state->importers[0], extToFile.begin()->second);

	return "Folder does not contains any known simfile format.";
}

SimfileFormats::SaveResult Editor::exportSimfile(
	const shared_ptr<Simfile>& sim,
	SimfileFormats::Exporter* exporter,
	bool backupOnSave)
{
	if (!exporter)
	{
		auto targetFormat = sim->format;

		if (targetFormat.empty())
			targetFormat = AV::Settings::editor().defaultSaveFormat;

		for (auto& it : state->exporters)
		{
			if (it->fmt == targetFormat)
			{
				exporter = it.get();
				break;
			}
		}
	}

	// If no exporter was found, default to first exporter (SM).
	if (!exporter)
		exporter = state->exporters[0].get();

	return exporter->save(*sim, backupOnSave);
}

bool Editor::openSimfile()
{
	int filterIndex = state->lastUsedLoadFilter;
	string filters = ConstructImportFilterText();
	auto path = Window::openFileDlg("Open file", filterIndex, string(), filters);
	if (path.str.empty())
		return false;

	state->lastUsedLoadFilter = filterIndex;
	auto result = LoadSimfile(path, filterIndex);
	if (!OpenSimfile(result))
		return false;

	Editor::addToRecentfiles(path);
	return true;
}

bool Editor::openNextSimfile(bool iterateForward)
{
	// Check if a simfile is open.
	auto sim = Editor::currentSimfile();
	if (!sim) return false;

	// Find the index of the current simfile directory in the pack directory.
	auto songDir = sim->directory;
	auto packDir = songDir.parent();
	auto songDirName = songDir.name();
	auto songList = FileSystem::listDirectories(packDir, false);
	int index = -1;
	for (int i = 0; i < (int)songList.size(); ++i)
	{
		if (songList[i] == songDirName)
			index = i;
	}
	if (index == -1)
	{
		Hud::error("Could not find the current simfile directory.");
		return false;
	}

	// Iterate over the following simfile directories until one of them loads.
	if (iterateForward)
	{
		if (index < (int)songList.size() - 1)
		{
			if (!closeSimfile()) return false;
			while (++index < (int)songList.size())
			{
				auto path = DirectoryPath(packDir, songList[index]);
				auto result = Editor::importSimfile(path.str);
				if (OpenSimfile(result))
					return true;
			}
		}
		Hud::info("This is the last simfile.");
	}
	else
	{
		if (index > 0)
		{
			if (!closeSimfile()) return false;
			while (--index >= 0)
			{
				auto path = DirectoryPath(packDir, songList[index]);
				auto result = Editor::importSimfile(path.str);
				if (OpenSimfile(result))
					return true;
			}
		}
		Hud::info("This is the first simfile.");
	}

	return false;
}

bool Editor::openSimfile(stringref path)
{
	auto result = Editor::importSimfile(path);
	return OpenSimfile(result);
}

// ====================================================================================================================
// Save simfile.

bool Editor::saveSimfile(bool showSaveAsDialog)
{
	auto sim = state->currentSimfile;
	if (!sim) return false;

	SimfileFormats::Exporter* exporter = nullptr;

	if (sim->filename.empty() || showSaveAsDialog)
	{
		// Show the save file dialog.
		int filterIndex = state->lastUsedSaveFilter;
		string filters = ConstructExportFilterText();
		auto path = Window::saveFileDlg("save file", filterIndex, FilePath(), filters);
		if (path.str.empty())
			return false;

		// Update the song directory and filename.
		sim->directory = path.directory();
		sim->filename = path.stem();

		// Translate the selected filter to a format.
		state->lastUsedSaveFilter = filterIndex;

		filterIndex -= 1; // Filter index is 1-based, so exporters start at 1.
		if (filterIndex >= 0 && filterIndex < state->exporters.size())
		{
			exporter = state->exporters[filterIndex].get();
			sim->format = exporter->fmt;
		}
	}

	// Save the simfile.
	auto result = Editor::exportSimfile(sim, exporter, state->backupOnSave);
	if (holds_alternative<string>(result))
	{
		auto& err = get<string>(result);
		Log::error(format("Could not save simfile '{}' due to error: {}", sim->filename, err));
		return false;
	}
	state->backupOnSave = false;

	// Signal to the edit history that the current state is the saved state.
	sim->onFileSaved();
	for (auto chart : sim->charts)
	{
		chart->history->onFileSaved();
	}

	return true;
}

// ====================================================================================================================
// Chart functions.

static int FindChartIndex(const Chart* chart)
{
	auto sim = state->currentSimfile;
	if (sim)
	{
		for (int i = 0; i < (int)sim->charts.size(); ++i)
		{
			if (sim->charts[i].get() == chart)
			{
				return i;
			}
		}
	}
	return -1;
}

void Editor::openSyncMode()
{
	openChart(-1);
}

void Editor::openChart(int index)
{
	auto sim = state->currentSimfile;
	if (sim && index >= 0 && index < int(sim->charts.size()))
	{
		auto chart = (index >= 0) ? sim->charts[index] : nullptr;
		if (state->currentChart != chart)
		{
			state->currentChart = chart;
			if (chart)
			{
				string desc = chart->getFullDifficulty();
				string mode = chart->gameMode->mode;
				Hud::note("Switched to %s :: %s", mode.data(), desc.data());
			}
			else
			{
				Hud::note("Switched to sync mode");
			}
			EventSystem::publish<Editor::ActiveChartChanged>();
		}
	}
}

void Editor::openChart(const Chart* chart)
{
	int index = FindChartIndex(chart);
	if (index >= 0)
	{
		openChart(index);
	}
	else
	{
		Hud::error("Trying to open a chart that is not in the chart list.");
	}
}

void Editor::nextChart()
{
	auto index = FindChartIndex(state->currentChart.get());
	openChart(index + 1);
}

void Editor::previousChart()
{
	auto index = FindChartIndex(state->currentChart.get());
	openChart(index - 1);
}

// ====================================================================================================================
// Get functions.

Chart* Editor::currentChart()
{
	return state->currentChart.get();
}

Simfile* Editor::currentSimfile()
{
	return state->currentSimfile.get();
}

shared_ptr<Simfile> Editor::currentSimfileReference()
{
	return state->currentSimfile;
}

Tempo* Editor::currentTempo()
{
	return state->currentTempo;
}

} // namespace AV