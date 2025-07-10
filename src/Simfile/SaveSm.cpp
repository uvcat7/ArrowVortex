#include <Core/StringUtils.h>

#include <System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>
#include <Simfile/TimingData.h>

#include <Managers/StyleMan.h>

#include <cmath>
#include <list>

namespace Vortex {
namespace Sm {
namespace {

enum SeperatorPos { END_OF_LINE, START_OF_LINE };
enum ForceWrite { ALWAYS, SONG_ONLY, NEVER };

static const int MEASURE_SUBDIV[] = {4, 8, 12, 16, 24, 32, 48, 64, 96, 192};
static const int NUM_MEASURE_SUBDIV = 10;
static const int ROWS_PER_NOTE_SECTION = 192;
static const int MIN_SECTIONS_PER_MEASURE = 4;

static double ToBeat(int row)
{
	return (double)row / ROWS_PER_BEAT;
}

struct ExportData
{
	Vector<int> diffs;
	FileWriter file;
	const Simfile* sim;
	const Chart* chart;
	bool ssc;
};

// ================================================================================================
// Generic write functions.

static void GiveUnicodeWarning(StringRef path, StringRef name)
{
	if(Str::isUnicode(path))
	{
		HudWarning("The %s path contains unicode characters,\n"
			"which might not work on some versions of Stepmania/ITG.", name.str());
	}
}

static String Escape(const char* name, const char* str)
{
	String s = (String)str;
	//These characters must be escaped in the .sm/.ssc file
	Str::replace(s, "\\", "\\\\");
	Str::replace(s, ":", "\\:");
	Str::replace(s, ";", "\\;");
	Str::replace(s, ";", "\\#");
	return s;
}

static bool ShouldWrite(ExportData& data, ForceWrite when, bool hasValue, bool sscOnly)
{
	bool versionOk = (data.ssc || !sscOnly);
	bool writeOk = hasValue || (when == ALWAYS) || (when == SONG_ONLY && data.chart == nullptr);
	return versionOk && writeOk;
}

static void WriteTag(ExportData& data, const char* tag, StringRef value, ForceWrite when, bool sscOnly)
{
	if(ShouldWrite(data, when, value.len() != 0, sscOnly))
	{
		data.file.printf("#%s:%s;\n", tag, value.str());
	}
}

static void WriteTag(ExportData& data, const char* tag, double value, ForceWrite when, bool sscOnly)
{
	if(ShouldWrite(data, when, value != 0, sscOnly))
	{
		data.file.printf("#%s:%.3f;\n", tag, value);
	}
}

static void WriteTag(ExportData& data, const char* tag, int value, ForceWrite when, bool sscOnly)
{
	if(ShouldWrite(data, when, value != 0, sscOnly))
	{
		data.file.printf("#%s:%i;\n", tag, value);
	}
}

static void WriteTextTag(ExportData& data, const char* tag, StringRef value, ForceWrite when, bool sscOnly, const char* alt = nullptr)
{
	if(alt && value.empty())
	{
		WriteTag(data, tag, Escape(tag, alt), when, sscOnly);
	}
	else
	{

		WriteTag(data, tag, Escape(tag, value.str()), when, sscOnly);
	}
}

static void WritePathTag(ExportData& data, const char* tag, StringRef value, ForceWrite when, bool sscOnly, const char* alt = nullptr)
{
	GiveUnicodeWarning(value, tag);
	WriteTextTag(data, tag, value, when, sscOnly, alt);
}

template <typename T, typename F>
static void WriteTag(ExportData& data, const char* tag, const T& list, SeperatorPos pos, char seperator, ForceWrite when, bool sscOnly, F func)
{
	if(ShouldWrite(data, when, list.size() != 0, sscOnly))
	{
		auto& file = data.file;
		file.printf("#%s:", tag);
		for(auto it = list.begin(); it != list.end();)
		{
			if(it != list.begin() && pos == START_OF_LINE)
			{
				file.write(&seperator, 1, 1);
			}
			func(*it);
			if(++it != list.end() && pos == END_OF_LINE)
			{
				file.write(&seperator, 1, 1);
			}
			if(list.size() > 1)
			{
				file.write("\n", 1, 1);
			}
		}
		file.write(";\n", 2, 1);
	}
}

template <typename T, typename F>
static void WriteSegments(ExportData& data, const char* tag, const Tempo* tempo,
	SeperatorPos pos, char seperator, ForceWrite when, bool sscOnly, F func)
{
	auto begin = tempo->segments->begin<T>();
	auto end = tempo->segments->end<T>();
	if(ShouldWrite(data, when, begin != end, sscOnly))
	{
		auto& file = data.file;
		file.printf("#%s:", tag);
		for(auto it = begin; it != end; ++it)
		{
			if(it != begin && pos == START_OF_LINE)
			{
				file.write(&seperator, 1, 1);
			}
			func(*it);
			if(it + 1 != end && pos == END_OF_LINE)
			{
				file.write(&seperator, 1, 1);
			}
			if(begin + 1 < end)
			{
				file.write("\n", 1, 1);
			}
		}
		file.write(";\n", 2, 1);
	}
}

// ================================================================================================
// Tempo write functions.

static void WriteOffset(ExportData& data, const Tempo* tempo)
{
	WriteTag(data, "OFFSET", tempo->offset, SONG_ONLY, false);
}

static void WriteBpms(ExportData& data, const Tempo* tempo)
{
	WriteSegments<BpmChange>(data, "BPMS", tempo, START_OF_LINE, ',', SONG_ONLY, false,
	[&](const BpmChange& change)
	{
		data.file.printf("%.3f=%.6f",
			ToBeat(change.row), change.bpm);
	});
}

static void WriteStops(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Stop>(data, "STOPS", tempo, START_OF_LINE, ',', SONG_ONLY, false,
	[&](const Stop& stop)
	{
		data.file.printf("%.3f=%.3f",
			ToBeat(stop.row), stop.seconds);
	});
}

static void WriteDelays(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Delay>(data, "DELAYS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Delay& delay)
	{
		data.file.printf("%.3f=%.3f",
			ToBeat(delay.row), delay.seconds);
	});
}

static void WriteWarps(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Warp>(data, "WARPS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Warp& warp)
	{
		data.file.printf("%.3f=%.3f",
			ToBeat(warp.row), ToBeat(warp.numRows));
	});
}

static void WriteSpeeds(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Speed>(data, "SPEEDS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Speed& speed)
	{
		data.file.printf("%.3f=%.3f=%.3f=%i",
			ToBeat(speed.row), speed.ratio, speed.delay, speed.unit);
	});
}

static void WriteScrolls(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Scroll>(data, "SCROLLS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Scroll& scroll)
	{
		data.file.printf("%.3f=%.3f",
			ToBeat(scroll.row), scroll.ratio);
	});
}

static void WriteTickCounts(ExportData& data, const Tempo* tempo)
{
	WriteSegments<TickCount>(data, "TICKCOUNTS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const TickCount& tick)
	{
		data.file.printf("%.3f=%i",
			ToBeat(tick.row), tick.ticks);
	});
}

static void WriteTimeSignatures(ExportData& data, const Tempo* tempo)
{
	WriteSegments<TimeSignature>(data, "TIMESIGNATURES", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const TimeSignature& sig)
	{
		data.file.printf("%.3f=%i=%i",
			ToBeat(sig.row), (int)ToBeat(sig.rowsPerMeasure), sig.beatNote);
	});
}

static void WriteLabels(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Label>(data, "LABELS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Label& label)
	{
		data.file.printf("%.3f=%s",
			ToBeat(label.row), label.str.str());
	});
}

static void WriteAttacks(ExportData& data, const Tempo* tempo)
{
	WriteTag(data, "ATTACKS", tempo->attacks, START_OF_LINE, ':', NEVER, true,
	[&](const Attack& attack)
	{
		data.file.printf("TIME=%.3f:", attack.time);
		data.file.printf((attack.unit == ATTACK_END) ? "END=%.3f:" : "LEN=%.3f:", attack.duration);
		data.file.printf("MODS=%s", attack.mods.str());
	});
}

static void WriteKeySounds(ExportData& data, const Tempo* tempo)
{
	WriteTag(data, "KEYSOUNDS", tempo->keysounds, END_OF_LINE, ',', NEVER, true,
	[&](StringRef str)
	{
		data.file.printf("%s", str.str());
	});
}

static void WriteCombos(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Combo>(data, "COMBOS", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Combo& combo)
	{
		data.file.printf("%.3f=%i", ToBeat(combo.row), combo.hitCombo);
		if(combo.hitCombo != combo.missCombo)
		{
			data.file.printf("=%i", combo.missCombo);
		}
	});
}

static void WriteFakes(ExportData& data, const Tempo* tempo)
{
	WriteSegments<Fake>(data, "FAKES", tempo, END_OF_LINE, ',', NEVER, true,
	[&](const Fake& fake)
	{
		data.file.printf("%.3f=%.3f",
			ToBeat(fake.row), ToBeat(fake.numRows));
	});
}

static void WriteDisplayBpm(ExportData& data, const Tempo* tempo)
{
	if(tempo->displayBpmType == BPM_CUSTOM)
	{
		if(tempo->displayBpmRange.min == tempo->displayBpmRange.max)
		{
			WriteTag(data, "DISPLAYBPM", tempo->displayBpmRange.min, ALWAYS, false);
		}
		else
		{
			Str::fmt fmt = "%1:%2";
			fmt.arg(tempo->displayBpmRange.min, 6);
			fmt.arg(tempo->displayBpmRange.max, 6);
			WriteTag(data, "DISPLAYBPM", fmt, ALWAYS, false);
		}
	}
	else if(tempo->displayBpmType == BPM_RANDOM)
	{
		WriteTag(data, "DISPLAYBPM", "*", ALWAYS, false);
	}
}

static void WriteTempo(ExportData& data, const Tempo* tempo)
{
	WriteOffset(data, tempo);

	WriteBpms(data, tempo);
	WriteStops(data, tempo);
	WriteDelays(data, tempo);
	WriteWarps(data, tempo);
	WriteSpeeds(data, tempo);
	WriteScrolls(data, tempo);
	WriteTickCounts(data, tempo);
	WriteTimeSignatures(data, tempo);

	WriteLabels(data, tempo);
	WriteAttacks(data, tempo);
	WriteKeySounds(data, tempo);
	WriteCombos(data, tempo);
	WriteFakes(data, tempo);
	WriteDisplayBpm(data, tempo);

	for(auto& misc : tempo->misc)
	{
		WriteTag(data, misc.tag.str(), misc.val, ALWAYS, false);
	}
}

// ================================================================================================
// Misc write functions.

static void WriteBgChanges(ExportData& data, const char* tag, const Vector<BgChange>& bgs)
{
	WriteTag(data, tag, bgs, END_OF_LINE, ',', ALWAYS, false,
	[&](const BgChange& bg)
	{
		if(bg.effect.empty() && bg.file2.empty() && bg.transition.empty() && bg.color.empty() && bg.color2.empty())
		{
			data.file.printf("%.3f=%s=%.3f=0=0=1",
				bg.startBeat,
				bg.file.str(),
				bg.rate);
		}
		else
		{
			data.file.printf("%.3f=%s=%.3f=%d=%d=%d=%s=%s=%s=%s=%s",
				bg.startBeat,
				bg.file.str(),
				bg.rate,
				bg.transition == "CrossFade",
				bg.effect == "StretchRewind",
				bg.effect != "StretchNoLoop",
				bg.effect.str(),
				bg.file2.str(),
				bg.transition.str(),
				bg.color.str(),
				bg.color2.str());
		}
	});
}

// ================================================================================================
// Chart writing functions.

static char GetNoteChar(uint type)
{
	if(type == NOTE_STEP_OR_HOLD)
	{
		return '1';
	}
	else if(type == NOTE_MINE)
	{
		return 'M';
	}
	else if(type == NOTE_LIFT)
	{
		return 'L';
	}
	else if(type == NOTE_FAKE)
	{
		return 'F';
	}
	return '0';
}

static char GetHoldChar(uint type)
{
	return (type == NOTE_STEP_OR_HOLD) ? '2' : '4';
}

static String RadarToString(const Vector<double>& list)
{
	String out;
	if(list.empty())
	{
		out = "0,0,0,0,0";
	}
	else
	{
		for(auto it = list.begin(); it != list.end(); ++it)
		{
			if(out.len()) Str::append(out, ',');
			Str::appendVal(out, *it, 0, 6);
		}
	}
	return out;
}

static const char* GetDifficultyString(Difficulty difficulty)
{
	switch(difficulty)
	{
		case DIFF_BEGINNER:  return "Beginner";
		case DIFF_EASY:      return "Easy";
		case DIFF_MEDIUM:    return "Medium";
		case DIFF_HARD:      return "Hard";
		case DIFF_CHALLENGE: return "Challenge";
	};
	return "Edit";
}

static int gcd(int a, int b)
{
	if (a == 0)
	{
		return b;
	}
	if (b == 0)
	{
		return a;
	}
	if (a > b)
	{
		return gcd(a - b, b);
	}
	else
	{
		return gcd(a, b - a);
	}
}

static void GetSectionCompression(const char* section, int width, std::list<uint> quantVec, int& count, int& pitch)
{
	// Determines the best compression for the given section.
	int best = ROWS_PER_NOTE_SECTION;
	String zeroline(width, '0');
	std::list<uint>::iterator it;
	int lcm = 1;
	for (it = quantVec.begin(); it != quantVec.end(); it++)
	{
		if (*it <= 0)
		{
			HudError("Bug: zero or negative quantization recorded in chart.");
			continue;
		}
		lcm = lcm * *it / gcd(lcm, *it);
		if (lcm > ROWS_PER_NOTE_SECTION)
		{
			lcm = ROWS_PER_NOTE_SECTION;
			break;
		}
	}

	// Set whole and half step measures to be quarter notes by default
	if (lcm <= MIN_SECTIONS_PER_MEASURE)
	{
		count = MIN_SECTIONS_PER_MEASURE;
	}
	else
	{
	// Determines the best compression for the given section.
    // Maybe lcm is the best factor, so just keep that.
		count = lcm;
		String zeroline(width, '0');

		//The factor list is small, just check them all by hand
		for (int i = lcm / 2; i >= 2; i--)
		{
			// Skip anything that isn't a lcm factor
			if (lcm % i > 0) continue;

			bool valid = true;
			float mod = (float) ROWS_PER_NOTE_SECTION / i;
			for (int j = 0; valid && j < ROWS_PER_NOTE_SECTION; ++j)
			{
				float rem = std::round(std::fmod(j, mod));
				// Check all the compressed rows and make sure they are empty
				if (rem > 0 && rem < static_cast<int>(mod)
					&& memcmp(section + j * width, zeroline.str(), width))
				{ 
					valid = false;
					break;
				}
			}

			// The first (largest) match is always the best
			if (valid && i >= MIN_SECTIONS_PER_MEASURE)
			{
				count = i;
				break;
			}
		}
	}
	// Is our factor a standard snap? If so, use it.
	// If not, save the measure as 192nds for SM5 Editor compatibility.
	if (ROWS_PER_NOTE_SECTION % count != 0)
	{
		count = ROWS_PER_NOTE_SECTION;
	}
	pitch = (ROWS_PER_NOTE_SECTION * width) / count;
}

static void WriteSections(ExportData& data)
{
	const Chart* chart = data.chart;

	int numCols = chart->style->numCols;
	int numPlayers = chart->style->numPlayers;

	if(numPlayers == 0 || numCols == 0) return;

	// Allocate a buffer for one uncompressed section of notes.
	int sectionSize = ROWS_PER_NOTE_SECTION * numCols;
	Vector<char> sectionVec;
	sectionVec.resize(sectionSize);
	char* section = sectionVec.data();

	// Export note data for each player.
	for(int pn = 0; pn < numPlayers; ++pn)
	{
		int startRow = 0;
		int remainingHolds = 0;

		Vector<const Note*> holdVec(numCols, nullptr);
		const Note** holds = holdVec.begin();

		std::list<uint> quantVec;

		const Note* it = chart->notes.begin();
		const Note* end = chart->notes.end();

		// Write all notes for the current player in blocks of one section.
		for(; it != end || remainingHolds > 0; startRow += ROWS_PER_NOTE_SECTION)
		{
			memset(section, '0', sectionSize);
			int endRow = startRow + ROWS_PER_NOTE_SECTION;

			// Advance to the first note in the current section.
			for(; it != end && (int)it->row < startRow; ++it);

			// Write the notes of the current player to the section data.
			for(; it != end && (int)it->row < endRow; ++it)
			{
				if((int)it->player == pn)
				{
					int pos = (it->row - startRow) * numCols + it->col;
					if(it->row == it->endrow)
					{
						section[pos] = GetNoteChar(it->type);
						quantVec.push_front(it->quant);
					}
					else
					{
						section[pos] = GetHoldChar(it->type);
						quantVec.push_front(it->quant);
						auto hold = holds[it->col];
						if(hold)
						{
							if((int)hold->endrow >= startRow && (int)hold->endrow < endRow)
							{
								int pos = ((int)hold->endrow - startRow) * numCols + (int)hold->col;
								section[pos] = '3';
								quantVec.push_front(it->quant);
								--remainingHolds;
							}
						}
						holds[it->col] = it;
						++remainingHolds;
					}
				}
			}

			// Write the remaining hold ends to the section data.
			if(remainingHolds > 0)
			{
				for(int col = 0; col < numCols; ++col)
				{
					auto hold = holds[col];
					if(hold)
					{
						if((int)hold->endrow >= startRow && (int)hold->endrow < endRow)
						{
							int pos = (hold->endrow - startRow) * numCols + hold->col;
							section[pos] = '3';
							holds[col] = nullptr;
							quantVec.push_front(it->quant);
							--remainingHolds;
						}
					}
				}
			}

			// Write the current section to the file.
			int count, pitch;
			const char* m = section;
			quantVec.unique();
			GetSectionCompression(m, numCols, quantVec, count, pitch);
			quantVec.clear();
			if (ROWS_PER_NOTE_SECTION % count != 0)
			{
				HudError("Bug: trying to save a non-supported number of rows. Data loss expected.");
			}
			for (int k = 0; k < count; ++k, m += pitch)
			{
				data.file.write(m, numCols, 1);
				data.file.write("\n", 1, 1);
			}

			// Write a comma if this is not the last section.
			if(it != end || remainingHolds > 0) data.file.write(",\n", 2, 1);
		}

		// Write an ampersand if this is not the last player.
		if(pn != numPlayers - 1) data.file.write("&\n", 2, 1);
	}
	data.file.write(";\n", 2, 1);
}

static void WriteChart(ExportData& data)
{
	const Chart* chart = data.chart;

	// Make sure each difficulty type is only exported once.
	Difficulty diff = chart->difficulty;
	int sd = chart->style->index * NUM_DIFFICULTIES + chart->difficulty;
	if(data.diffs.find(sd) != data.diffs.size())
	{
		Difficulty oldDiff = diff;
		String oldDesc = chart->description();
		diff = DIFF_EDIT;
		String newDesc = chart->description();

		HudWarning("Duplicate difficulties, saving (%s) as (%s) instead",
			oldDesc.str(), newDesc.str());
	}
	else if(chart->difficulty != DIFF_EDIT)
	{
		data.diffs.push_back(sd);
	}

	// Write the output chart data.
	data.file.printf("//--------------- %s - %s ----------------\n",
		chart->style->id.str(), chart->artist.str());

	String chartStyle = Escape("chart style", chart->style->id.str());
	String chartArtist = Escape("chart artist", chart->artist.str());

	if(data.ssc)
	{
		WriteTag(data, "NOTEDATA", String(), ALWAYS, true);
		WriteTag(data, "STEPSTYPE", chartStyle.str(), ALWAYS, true);
		WriteTag(data, "DESCRIPTION", chartArtist.str(), ALWAYS, true);
		WriteTag(data, "DIFFICULTY", GetDifficultyString(diff), ALWAYS, true);
		WriteTag(data, "METER", chart->meter, ALWAYS, true);
		WriteTag(data, "RADARVALUES", RadarToString(chart->radar).str(), ALWAYS, true);

		if(chart->tempo) WriteTempo(data, chart->tempo);
		
		data.file.printf("#NOTES:\n");
	}
	else
	{
		data.file.printf("#NOTES:\n");
		data.file.printf("     %s:\n", chartStyle.str());
		data.file.printf("     %s:\n", chartArtist.str());
		data.file.printf("     %s:\n", GetDifficultyString(diff));
		data.file.printf("     %i:\n", chart->meter);
		data.file.printf("     %s:\n", RadarToString(chart->radar).str());
	}

	WriteSections(data);
}

// ================================================================================================
// Simfile saving.

bool SaveSimfile(const Simfile* sim, bool ssc, bool backup)
{
	ExportData data;
	data.ssc = ssc;
	data.chart = nullptr;
	data.sim = sim;

	Path path = sim->dir + sim->file + (ssc ? ".ssc" : ".sm");

	// If a backup file is requested, rename the existing sim before saving over it.
	if(backup && (path.attributes() & File::ATR_EXISTS))
	{
		if(!File::moveFile(path.str, path.str + ".old", true))
		{
			String name = path.filename();
			HudError("Could not backup \"%s\".", name.str());
		}
	}

	// Open the output file.
	if(!data.file.open(path)) return false;
	GiveUnicodeWarning(path, "sim");

	// Start with a version tag for SSC files.
	if(ssc) WriteTag(data, "VERSION", "0.83", ALWAYS, true);

	WriteTextTag(data, "TITLE", sim->title, ALWAYS, false, "unknown");
	WriteTextTag(data, "SUBTITLE", sim->subtitle, ALWAYS, false);
	WriteTextTag(data, "ARTIST", sim->artist, ALWAYS, false);
	WriteTextTag(data, "TITLETRANSLIT", sim->titleTr, ALWAYS, false);
	WriteTextTag(data, "SUBTITLETRANSLIT", sim->subtitleTr, ALWAYS, false);
	WriteTextTag(data, "ARTISTTRANSLIT", sim->artistTr, ALWAYS, false);
	WriteTextTag(data, "GENRE", sim->genre, ALWAYS, false);
	WriteTextTag(data, "CREDIT", sim->credit, ALWAYS, false);

	WritePathTag(data, "MUSIC", sim->music, ALWAYS, false);
	WritePathTag(data, "BANNER", sim->banner, ALWAYS, false);
	WritePathTag(data, "BACKGROUND", sim->background, ALWAYS, false);
	WritePathTag(data, "CDTITLE", sim->cdTitle, ALWAYS, false);

	WriteTag(data, "SAMPLESTART", sim->previewStart, ALWAYS, false);
	WriteTag(data, "SAMPLELENGTH", sim->previewLength, ALWAYS, false);
	WriteTag(data, "SELECTABLE", sim->isSelectable ? "YES" : "NO", ALWAYS, false);

	if(sim->previewLength > 0 && sim->previewLength < 3)
	{
		HudWarning("The music preview is shorter than 3 seconds, which will default to 12 seconds in ITG.");
	}
	else if(sim->previewLength > 30)
	{
		HudWarning("The music preview is longer than 30 seconds, which will default to 12 seconds in ITG.");
	}

	WriteTempo(data, sim->tempo);

	WriteBgChanges(data, "BGCHANGES", sim->bgChanges[0]);
	if(sim->bgChanges[1].size())
	{
		WriteBgChanges(data, "BGCHANGES2", sim->bgChanges[1]);
	}
	WriteBgChanges(data, "FGCHANGES", sim->fgChanges);

	for(auto& chart : sim->charts)
	{
		data.chart = chart;
		WriteChart(data);
		data.chart = nullptr;
	}

	HudInfo("Saved: %s", path.filename().str());

	return true;
}

}; // anonymous namespace.

bool SaveSm(const Simfile* sim, bool backup)
{
	return SaveSimfile(sim, false, backup);
}

bool SaveSsc(const Simfile* sim, bool backup)
{
	return SaveSimfile(sim, true, backup);
}

}; // namespace Sm
}; // namespace Vortex
