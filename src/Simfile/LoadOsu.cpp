#include <Core/Core.h>

#include <map>
#include <algorithm>
#include <cmath>

#include <Core/Vector.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/TimingData.h>

#include <Managers/StyleMan.h>

namespace Vortex {
namespace Osu {
namespace {

enum { OSU = 0, OSUMANIA = 3 };

struct TimingPoint
{
	double bpm, velocity;
};

struct Parser
{
	double bpm;
	const char* line;
	String prop, tag;
};

struct OsuFile
{
	struct HitObject { int x; double time, endtime; };

	int gameMode;
	int numCols;
	int overallDifficulty;
	String chartVersion;
	String musicPath;
	String songArtist;
	String songArtistUnicode;
	String songTitle;
	String songTitleUnicode;
	String stepArtist;
	String artwork;
	String filename;

	std::map<double, TimingPoint> timingPoints;
	Vector<HitObject> hitObjects;
};

// ================================================================================================
// Parsing utility functions.

static bool IsSpace(char c)
{
	return c == ' ' || c == '\n';
}

static String Unquote(StringRef s)
{
	if(s.len() >= 2 && s.back() == '"' && s.front() == '"')
	{
		return Str::create(s.begin() + 1, s.end() - 1);
	}
	return s;
}

static const char* NextLine(const char* p)
{
	while(*p && *p != '\n') ++p;
	while(IsSpace(*p)) ++p;
	return p;
}

static bool ReadProperty(Parser& parser)
{
	// Check if the line is a tag.
	const char* p = parser.line;
	if(*p == 0 || *p == '[') return false;

	// Find the begin and end of the line.
	const char* begin = p;
	while(*p && *p != '\n') ++p;
	const char* end = p;
	while(end > begin && IsSpace(end[-1])) --end;

	// Copy the property and go to the next line.
	Str::assign(parser.prop, begin, end - begin);
	parser.line = NextLine(p);
	return true;
}

static bool ReadTag(Parser& parser)
{
	// Find the opening bracket.
	const char* p = parser.line;
	while(*p && *p != '[') p = NextLine(p);
	if(*p == 0) return false;
	const char* begin = p + 1;

	// Find the closing bracket.
	while(*p && *p != ']' && *p != '\n') ++p;
	if(*p == 0) return false;
	const char* end = p;

	// Copy the tag and go to the next line.
	Str::assign(parser.tag, begin, end - begin);
	parser.line = NextLine(p);
	return true;
}

static bool IsProp(StringRef prop, const char* name)
{
	const char* p = prop.begin();
	while(*name && *p == *name) ++p, ++name;
	return (*name == 0 && (*p == ':' || *p == ' ' || *p == '\n'));
}

static String PropVal(StringRef prop)
{
	const char* p = prop.begin();
	while(*p != ':') ++p;
	while(*p == ':' || *p == ' ') ++p;
	return Str::create(p, prop.end());
}

static int NoteVal(const char*& p)
{
	char* end;
	long out = strtol(p, &end, 0);
	if(end) p = end;
	while(*p && *p != ',') ++p;
	if(*p == ',') ++p;
	return (int)out;
}

static double NoteValf(const char*& p)
{
	char* end;
	double out = strtod(p, &end);
	if(end) p = end;
	while(*p && *p != ',') ++p;
	if(*p == ',') ++p;
	return (int)out;
}

static void SkipNoteVal(const char*& p)
{
	while(*p && *p != ',') ++p;
	if(*p == ',') ++p;
}

// ================================================================================================
// Main osu parser.

static void ParseGeneral(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		StringRef prop = parser.prop;
		if(IsProp(prop, "AudioFilename"))
		{
			out.musicPath = PropVal(prop);
		}
		else if(IsProp(prop, "Mode"))
		{
			out.gameMode = Str::readInt(PropVal(prop));
		}
	}
}

static void ParseMetaData(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		StringRef prop = parser.prop;
		if(IsProp(prop, "Title"))
		{
			out.songTitle = PropVal(prop);
		}
		else if(IsProp(prop, "TitleUnicode"))
		{
			out.songTitleUnicode = PropVal(prop);
		}
		else if(IsProp(prop, "Artist"))
		{
			out.songArtist = PropVal(prop);
		}
		else if(IsProp(prop, "ArtistUnicode"))
		{
			out.songArtistUnicode = PropVal(prop);
		}
		else if(IsProp(prop, "Creator"))
		{
			out.stepArtist = PropVal(prop);
		}
		else if(IsProp(prop, "Version"))
		{
			out.chartVersion = PropVal(prop);
		}
	}
}

static void ParseDifficulty(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		StringRef prop = parser.prop;
		if(IsProp(prop, "OverallDifficulty"))
		{
			out.overallDifficulty = Str::readInt(PropVal(prop));
		}
		else if(IsProp(prop, "CircleSize"))
		{
			out.numCols = Str::readInt(PropVal(prop));
		}
	}
}

static void ParseEvents(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		auto bg = Str::split(parser.prop, ",");
		if(bg.size() >= 3 && bg[0] == "0")
		{
			out.artwork = Unquote(bg[2]);
			break;
		}
	}
}

static void ParseTimingPoints(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		auto tp = Str::split(parser.prop, ",");
		if(tp.size() >= 2)
		{
			double time = Str::readDouble(tp[0]) * 0.001;
			double spb = Str::readDouble(tp[1]) * 0.001;
			if(spb > 0)
			{
				double bpm = 60.0 / spb;
				double roundBPM = round(bpm);
				if(abs(bpm - roundBPM) < 0.001) bpm = roundBPM;
				out.timingPoints[time] = {bpm, 1.0};
				parser.bpm = bpm;
			}
			else
			{
				double velocity = 0.1 / abs(spb);
				out.timingPoints[time] = {parser.bpm, velocity};
			}
		}
	}
}

static TimingPoint* GetTimingPoint(OsuFile& osu, double time)
{
	auto it = osu.timingPoints.lower_bound(time);
	if(it == osu.timingPoints.end()) return nullptr;
	while(it != osu.timingPoints.begin() && it->first > time) --it;
	return &(it->second);
}

static void ParseHitObjects(OsuFile& out, Parser& parser)
{
	while(ReadProperty(parser))
	{
		const char* p = parser.prop.str();
		int x = max(0, NoteVal(p)), y = NoteVal(p);
		double time = NoteVal(p) * 0.001;
		int type = NoteVal(p);
		switch(type)
		{
		case 1: // Regular step.
			out.hitObjects.push_back({x, time, time});
			break;
		case 128: // Hold note.
			int hitSound = NoteVal(p);
			double endTime = max(time, NoteVal(p) * 0.001);
			out.hitObjects.push_back({x, time, endTime});
			break;
		};
	}
}

static void ParseTag(OsuFile& osu, Parser& parser)
{
	if(parser.tag == "General")
	{
		ParseGeneral(osu, parser);
	}
	else if(parser.tag == "Metadata")
	{
		ParseMetaData(osu, parser);
	}
	else if(parser.tag == "Difficulty")
	{
		ParseDifficulty(osu, parser);
	}
	else if(parser.tag == "Events")
	{
		ParseEvents(osu, parser);
	}
	else if(parser.tag == "TimingPoints")
	{
		ParseTimingPoints(osu, parser);
	}
	else if(parser.tag == "HitObjects" && osu.gameMode == OSUMANIA)
	{
		ParseHitObjects(osu, parser);
	}
}

static bool ParseFile(OsuFile& out, String str)
{
	// Convert all comments and whitespace to space characters.
	for(char* p = str.begin(), *e = str.end(); p < e; ++p)
	{
		if(p[0] == '/' && p[1] == '/')
			for(; *p && *p != '\n'; ++p) *p = ' ';
		if(*p == '\r') *p = '\n';
		if(*p == '\t') *p = ' ';
	}

	// Initialize the parser.
	Parser parser;
	parser.bpm = SIM_DEFAULT_BPM;
	parser.line = str.begin();
	while(IsSpace(*parser.line)) ++parser.line;

	// Read the file data.
	out.numCols = 4;
	out.gameMode = 0;
	out.overallDifficulty = 1;
	while(ReadTag(parser)) ParseTag(out, parser);

	return true;
}

// ================================================================================================
// Difficulty selection.

static const char* difficultyStrings[][5] = {

	{"novice", "easy", "medium", "hard", "expert"}, // In The Groove.
	{"easy", "normal", "hard", "insane", "extra"}, // Basic Osu A. 
	{"easy", "normal", "hard", "insane", "expert"}, // Basic Osu B.
	{"easy", "normal", "hard", "lunatic", "extra"}, // Touhou.
	{"beginner", "normal", "hyper", "another", "black another"}, // IIDX.
	{"ez", "nm", "hd", "mx", "sc"}, // Osu!mania.

};

static void AssignDifficulties(Simfile* sim, const Vector<String>& versions)
{
	if(sim->charts.empty()) return;

	auto& charts = sim->charts;
	Chart* bestMatches[5] = {0, 0, 0, 0, 0};
	int bestNumMatches = 0;

	// Find the set of difficulties that has the most matches with the charts.
	for(int set = 0; set < 6; ++set)
	{
		Chart* matches[5] = {0, 0, 0, 0, 0};
		for(int i = 0; i < charts.size(); ++i)
		{
			for(int j = 0; j < 5; ++j)
			{
				if(versions[i] == difficultyStrings[set][j])
				{
					matches[j] = charts[i];
				}
			}
		}
		int numMatches = 0;
		for(int i = 0; i < 5; ++i)
		{
			if(matches[i]) ++numMatches;
		}
		if(numMatches > bestNumMatches)
		{
			bestNumMatches = numMatches;
			for(int i = 0; i < 5; ++i)
			{
				bestMatches[i] = matches[i];
			}
		}
	}

	// Assign difficulties based on the best match.
	if(bestNumMatches > 0)
	{
		for(auto& chart : charts)
		{
			chart->difficulty = DIFF_EDIT;
		}
		for(int i = 0; i < 5; ++i)
		{
			if(bestMatches[i])
			{
				bestMatches[i]->difficulty = (Difficulty)i;
			}
		}
	}
	else // No matches, sort by number of notes and assign upwards.
	{
		std::sort(charts.begin(), charts.end(),
		[](const Chart* a, const Chart* b)
		{
			return a->notes.size() < b->notes.size();
		});
		int offset = max(0, min(charts[0]->meter / 2, 5 - charts.size()));
		for(int i = 0; i < charts.size(); ++i)
		{
			charts[i]->difficulty = (Difficulty)(offset + i);
		}
	}
}

// ================================================================================================
// Timing point conversion.

static void ConvertTimingPoints(Simfile* sim, OsuFile& osu)
{
	auto tempo = sim->tempo;

	// If the timing points list is empty, return the fallback BPM 120.
	if(osu.timingPoints.empty()) return;

	// Set the first BPM at row zero.
	auto it = osu.timingPoints.begin();
	auto end = osu.timingPoints.end();

	BpmChange initialBpm;
	initialBpm.bpm = it->second.bpm;
	tempo->segments->append(initialBpm);
	tempo->offset = -it->first;

	// Make sure the offset is positive, to prevent notes before the first row.
	if(tempo->offset < 0)
	{
		double spm = 4.0 * 60.0 / it->second.bpm;
		tempo->offset = spm - fmod(-tempo->offset, spm);
	}

	// Calculate the rows of the other BPM values relative to the previous BPM.
	double spb = 60.0 / it->second.bpm;
	double prevTime = -tempo->offset, prevRow = 0.0;
	for(++it; it != end; ++it)
	{
		// Determine the row of the BPM change.
		double deltaTime = it->first - prevTime;
		double deltaRows = 48.0 * deltaTime / spb;
		double curRow = prevRow + deltaRows;

		// Round the row.
		int row = (int)(curRow + 0.5);
		tempo->segments->append(BpmChange(row, it->second.bpm));

		// Advance to the next BPM change.
		spb = 60.0 / it->second.bpm;
		prevTime = it->first;
		prevRow = curRow;
	}
}

// ================================================================================================
// Note conversion.

static bool LessThan(const OsuFile::HitObject& a, const OsuFile::HitObject& b)
{
	if(a.time != b.time) return a.time < b.time;
	return a.x < b.x;
}

static void ConvertNotes(Simfile* sim, OsuFile& osu, Chart& chart)
{
	// Make sure the hit objects are sorted by time.
	if(!std::is_sorted(osu.hitObjects.begin(), osu.hitObjects.end(), LessThan))
	{
		std::sort(osu.hitObjects.begin(), osu.hitObjects.end(), LessThan);
	}

	// Then assign rows to notes based on the time stamps.
	TimingData timing;
	timing.update(sim->tempo);
	TempoRowTracker tracker(timing);
	for(auto& hitObject : osu.hitObjects)
	{
		// x ranges from 0 to 512 (inclusive), y ranges from 0 to 384 (inclusive).
		int colWidth = 512 / max(osu.numCols, 1);
		int col = min(max(0, hitObject.x / colWidth), osu.numCols);

		// Convert time and endtime to rows.
		int row = tracker.advance(hitObject.time);
		if(hitObject.endtime > hitObject.time)
		{
			int endrow = timing.timeToRow(hitObject.endtime);
			chart.notes.append({row, endrow, (uint)col, 0, NOTE_STEP_OR_HOLD});
		}
		else
		{
			chart.notes.append({row, row, (uint)col, 0, NOTE_STEP_OR_HOLD});
		}
	}
}

static void DestroyFiles(Vector<OsuFile*>& files)
{
	for(auto file : files)
	{
		delete file;
	}
}

}; // anonymous namespace.

/*
static bool ParseOsz(Vector<OsuFile*>& out, StringRef path, String& err)
{
	unzFile zip = unzOpen(path.str());

	// Get info about the zip file
	unz_global_info global_info;
	if(unzGetGlobalInfo(zip, &global_info) != UNZ_OK)
	{
		err = "could not read file global info";
		unzClose(zip);
		return false;
	}

	// Read the files.
	String str;
	char buffer[512];
	for(uint i = 0; i < global_info.number_entry; ++i)
	{
		unz_file_info file_info;
		if(unzGetCurrentFileInfo(zip, &file_info, buffer, 512, 0, 0, 0, 0) == UNZ_OK)
		{
			Path filename(buffer);
			if(filename.hasExt("osu"))
			{
				while(true)
				{
					int read = unzReadCurrentFile(zip, buffer, 512);
					if(read <= 0) break;
					Str::append(str, buffer, read);
				}
				if(str.len())
				{
					out.push_back(new OsuFile);
					ParseFile(*out.back(), str);
					out.back()->filename = filename.name();
				}
				unzCloseCurrentFile(zip);
			}			
		}
		if(i < global_info.number_entry - 1)
		{
			unzGoToNextFile(zip);
		}
	}
	unzClose(zip);

	return true;
}
*/

static bool ParseDir(Vector<OsuFile*>& out, StringRef dir, String& err)
{
	for(auto& file : File::findFiles(dir, false, "osu"))
	{
		bool success;
		String str = File::getText(file, &success);
		if(str.empty() || !success) continue;

		out.push_back(new OsuFile);
		ParseFile(*out.back(), str);
		out.back()->filename = file.name();
	}
	return true;
}

bool LoadOsu(StringRef path, Simfile* sim)
{
	bool result = true;
	bool isZip = Path(path).hasExt("osz");

	// Parse all osu files in the current directory.
	String err;
	Vector<OsuFile*> files;
	if(isZip)
	{
		// ParseOsz(files, path, err);
	}
	else
	{
		ParseDir(files, sim->dir, err);
	}

	// Check if there are any osu files.
	if(files.empty()) return false;

	// Check if there are any osu/osu!mania charts.
	OsuFile* firstOsuManiaFile = nullptr;
	OsuFile* firstOsuFile = nullptr;
	OsuFile* mainFile = nullptr;
	for(auto file : files)
	{
		if(file->gameMode == OSU && !firstOsuFile)
			firstOsuFile = file;

		if(file->gameMode == OSUMANIA && !firstOsuManiaFile)
			firstOsuManiaFile = file;

		if((file->gameMode == OSU || file->gameMode == OSUMANIA) && file->filename == sim->file)
			mainFile = file;
	}
	if(firstOsuFile == nullptr && firstOsuManiaFile == nullptr)
	{
		DestroyFiles(files);
		return false;
	}

	// Make sure we have a main file.
	if(!mainFile) mainFile = firstOsuFile;
	if(!mainFile) mainFile = firstOsuManiaFile;

	// Copy song properties.
	sim->title = mainFile->songTitle;
	sim->artist = mainFile->songArtist;
	sim->music = mainFile->musicPath;
	sim->banner = mainFile->artwork;
	sim->background = mainFile->artwork;

	// Convert the timing points to BPM changes.
	ConvertTimingPoints(sim, *mainFile);

	// Convert the hit objects to DDR/ITG charts.
	Vector<String> versions;
	for(auto file : files)
	{
		if(file->gameMode == OSUMANIA && file->hitObjects.size())
		{
			versions.push_back(file->chartVersion);

			Chart* chart = new Chart;

			chart->style = gStyle->findStyle(file->chartVersion, file->numCols, 1);
			chart->artist = file->stepArtist;
			chart->meter = file->overallDifficulty;
			ConvertNotes(sim, *file, *chart);

			sim->charts.push_back(chart);
		}
	}

	// After all charts are loaded, check which set of difficulties best matches the loaded set.
	AssignDifficulties(sim, versions);

	// Remove the difficulty tag from the filename.
	sim->file = mainFile->filename;
	int begin = Str::findLast(sim->file, '[');
	int end = Str::find(sim->file, ']', begin);
	if(begin >= 0 && end != String::npos)
	{
		Str::erase(sim->file, begin, end + 1 - begin);
		if(sim->file.len() && sim->file.back() == ' ')
		{
			Str::pop_back(sim->file);
		}
	}

	sim->format = isZip ? SIM_OSZ : SIM_OSU;

	DestroyFiles(files);
	return result;
}

}; // namespace Osu
}; // namespace Vortex
