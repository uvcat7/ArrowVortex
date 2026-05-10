#include <Core/Core.h>
#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <algorithm>

#include <System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/Notes.h>
#include <Simfile/SegmentGroup.h>

#include <Managers/StyleMan.h>

namespace Vortex {
namespace Dwi {

// ========================================================================================
// Note encoding.

enum DwiNote
{
	NO_NOTE = 0,
	NOTE_L, NOTE_UL, NOTE_D, NOTE_U, NOTE_UR, NOTE_R,
	NOTE_COUNT
};

static DwiNote ColToDwiNote(int col, int numCols, int pad)
{
	int localCol = col - (numCols == 8 ? pad * 4 : 0);
	int panelCols = (numCols == 8) ? 4 : numCols;
	if(localCol < 0 || localCol >= panelCols) return NO_NOTE;
	switch(panelCols)
	{
	case 4:
		switch(localCol)
		{
		case 0: return NOTE_L;
		case 1: return NOTE_D;
		case 2: return NOTE_U;
		case 3: return NOTE_R;
		}
		break;
	case 6:
		switch(localCol)
		{
		case 0: return NOTE_L;
		case 1: return NOTE_UL;
		case 2: return NOTE_D;
		case 3: return NOTE_U;
		case 4: return NOTE_UR;
		case 5: return NOTE_R;
		}
		break;
	}
	return NO_NOTE;
}

static char DwiNoteToChar(DwiNote a)
{
	switch(a)
	{
	case NOTE_L:  return '4';
	case NOTE_UL: return 'C';
	case NOTE_D:  return '2';
	case NOTE_U:  return '8';
	case NOTE_UR: return 'D';
	case NOTE_R:  return '6';
	default:      return '0';
	}
}

// Returns a single DWI character encoding both notes (or just one if b == NO_NOTE).
static char DwiPairToChar(DwiNote a, DwiNote b)
{
	if(a == NO_NOTE && b == NO_NOTE) return '0';
	if(b == NO_NOTE || b == a) return DwiNoteToChar(a);
	if(a == NO_NOTE) return DwiNoteToChar(b);
	if(a > b) { DwiNote t = a; a = b; b = t; }
	if(a == NOTE_L  && b == NOTE_D)  return '1';
	if(a == NOTE_L  && b == NOTE_UL) return 'E';
	if(a == NOTE_L  && b == NOTE_U)  return '7';
	if(a == NOTE_L  && b == NOTE_UR) return 'I';
	if(a == NOTE_L  && b == NOTE_R)  return 'B';
	if(a == NOTE_UL && b == NOTE_D)  return 'F';
	if(a == NOTE_UL && b == NOTE_U)  return 'G';
	if(a == NOTE_UL && b == NOTE_UR) return 'M';
	if(a == NOTE_UL && b == NOTE_R)  return 'H';
	if(a == NOTE_D  && b == NOTE_U)  return 'A';
	if(a == NOTE_D  && b == NOTE_UR) return 'J';
	if(a == NOTE_D  && b == NOTE_R)  return '3';
	if(a == NOTE_U  && b == NOTE_UR) return 'K';
	if(a == NOTE_U  && b == NOTE_R)  return '9';
	if(a == NOTE_UR && b == NOTE_R)  return 'L';
	return '0';
}

// ========================================================================================
// Difficulty mapping.

static const char* DifficultyToDwi(Difficulty diff)
{
	switch(diff)
	{
	case DIFF_BEGINNER:  return "BEGINNER";
	case DIFF_EASY:      return "BASIC";
	case DIFF_MEDIUM:    return "ANOTHER";
	case DIFF_HARD:      return "MANIAC";
	case DIFF_CHALLENGE: return "SMANIAC";
	default:             return "ANOTHER";
	}
}

// ========================================================================================
// Beat/row conversion.
// DWI CHANGEBPM/FREEZE stores beat positions as str where: AV_row = ParseBeat(str) / 4
// ParseBeat gives: row = str * 48, so after /4: AV_row = str * 12
// Inverse: DWI beat string = AV_row / 12.0

static double RowToDwiBeat(int row)
{
	return (double)row / 12.0;
}

// ========================================================================================
// Note data writing.

struct NoteEvent
{
	int row;
	int col;
	bool isHoldStart;
};

static void EmitNoteSlot(FileWriter& file, DwiNote* others, int nOthers, DwiNote* starts, int nStarts)
{
	if(nOthers == 0 && nStarts == 0)
	{
		file.printf("0");
	}
	else if(nStarts == 0)
	{
		if(nOthers == 1)
			file.printf("%c", DwiNoteToChar(others[0]));
		else if(nOthers == 2)
			file.printf("%c", DwiPairToChar(others[0], others[1]));
		else
		{
			file.printf("<");
			for(int i = 0; i < nOthers; ++i)
				file.printf("%c", DwiNoteToChar(others[i]));
			file.printf(">");
		}
	}
	else if(nOthers == 0)
	{
		if(nStarts == 1)
		{
			char c = DwiNoteToChar(starts[0]);
			file.printf("%c!%c", c, c);
		}
		else if(nStarts == 2)
		{
			char c = DwiPairToChar(starts[0], starts[1]);
			file.printf("%c!%c", c, c);
		}
		else
		{
			file.printf("<");
			for(int i = 0; i < nStarts; ++i)
				file.printf("%c", DwiNoteToChar(starts[i]));
			file.printf("!");
			for(int i = 0; i < nStarts; ++i)
				file.printf("%c", DwiNoteToChar(starts[i]));
			file.printf(">");
		}
	}
	else
	{
		// Mixed tap + hold-start on same row: use corner pair char when possible.
		file.printf("<");
		if(nOthers == 1 && nStarts == 1)
		{
			// Encode tap+hold-start as a single corner pair char (e.g., UP+LEFT -> 7).
			file.printf("%c", DwiPairToChar(others[0], starts[0]));
		}
		else
		{
			if(nOthers == 1)
				file.printf("%c", DwiNoteToChar(others[0]));
			else if(nOthers == 2)
				file.printf("%c", DwiPairToChar(others[0], others[1]));
			else
				for(int i = 0; i < nOthers; ++i)
					file.printf("%c", DwiNoteToChar(others[i]));
			if(nStarts == 1)
				file.printf("%c", DwiNoteToChar(starts[0]));
			else if(nStarts == 2)
				file.printf("%c", DwiPairToChar(starts[0], starts[1]));
			else
				for(int i = 0; i < nStarts; ++i)
					file.printf("%c", DwiNoteToChar(starts[i]));
		}
		file.printf("!");
		if(nStarts == 1)
			file.printf("%c", DwiNoteToChar(starts[0]));
		else if(nStarts == 2)
			file.printf("%c", DwiPairToChar(starts[0], starts[1]));
		else
			for(int i = 0; i < nStarts; ++i)
				file.printf("%c", DwiNoteToChar(starts[i]));
		file.printf(">");
	}
}

static void WriteNoteDataForPad(FileWriter& file, const NoteList& notes, int numCols, int pad)
{
	// Each measure is written as one line to stay well under the 4096-byte
	// tagLeftSide/tagRightSide buffers in DMX-family DWI loaders.
	// mode 0: notes on 24-row grid -> bare 8th-note chars, 8 slots/measure
	// mode 1: notes on 12-row grid -> ( ) 16th-note, 16 slots/measure
	// mode 2: notes off 12-row grid -> backtick/apostrophe 192nd-note, 192 slots/measure
	// The { } bracket is never used: its beatDivision differs between AV and DMX.
	const int ROWS_PER_BEAT2 = 48;
	const int ROWS_PER_MEASURE = ROWS_PER_BEAT2 * 4; // 192 rows; assumes 4/4

	int padStart = (numCols == 8) ? pad * 4 : 0;
	int padCols  = (numCols == 8) ? 4 : numCols;

	Vector<NoteEvent> events;
	for(const Note& note : notes)
	{
		int col = (int)note.col;
		if(col < padStart || col >= padStart + padCols) continue;
		if(note.type != NOTE_STEP_OR_HOLD) continue;
		if(note.endrow > note.row)
		{
			events.push_back({note.row, col, true});
			events.push_back({note.endrow, col, false});
		}
		else
		{
			events.push_back({note.row, col, false});
		}
	}
	if(events.empty())
	{
		file.printf("0");
		return;
	}

	std::sort(events.begin(), events.end(), [](const NoteEvent& a, const NoteEvent& b)
	{
		return (a.row != b.row) ? (a.row < b.row) : (a.col < b.col);
	});

	int lastRow     = events.back().row;
	int lastMeasure = lastRow / ROWS_PER_MEASURE;

	int ei        = 0;
	int numEvents = (int)events.size();

	for(int measure = 0; measure <= lastMeasure; measure++)
	{
		int mStart = measure * ROWS_PER_MEASURE;
		int mEnd   = mStart + ROWS_PER_MEASURE;

		// Determine mode for this measure.
		int mode = 0;
		for(int ti = ei; ti < numEvents && events[ti].row < mEnd; ti++)
		{
			if(events[ti].row % 12 != 0) { mode = 2; break; }
			if(events[ti].row % 24 != 0)   mode = 1;
		}

		int step = (mode == 0) ? 24 : (mode == 1) ? 12 : 1;

		if(mode == 1)      file.printf("(");
		else if(mode == 2) file.printf("`");

		for(int row = mStart; row < mEnd; row += step)
		{
			DwiNote starts[8]; int nStarts = 0;
			DwiNote others[8]; int nOthers = 0;

			while(ei < numEvents && events[ei].row == row)
			{
				const NoteEvent& ev = events[ei++];
				DwiNote dn = ColToDwiNote(ev.col, numCols, pad);
				if(dn == NO_NOTE) continue;
				if(ev.isHoldStart)
					starts[nStarts++] = dn;
				else
					others[nOthers++] = dn;
			}

			EmitNoteSlot(file, others, nOthers, starts, nStarts);
		}

		if(mode == 1)      file.printf(")\n");
		else if(mode == 2) file.printf("'\n");
		else               file.printf("\n");
	}
}

// ========================================================================================
// Style resolution.

static bool GetDwiStyle(const Chart* chart, const char*& outTag, int& outNumPads)
{
	StringRef id = chart->style->id;
	if(id == "dance-single") { outTag = "SINGLE"; outNumPads = 1; return true; }
	if(id == "dance-double") { outTag = "DOUBLE"; outNumPads = 2; return true; }
	if(id == "dance-couple") { outTag = "COUPLE"; outNumPads = 2; return true; }
	if(id == "dance-solo")   { outTag = "SOLO";   outNumPads = 1; return true; }
	return false;
}

// ========================================================================================
// Simfile saving.

bool SaveDwi(const Simfile* sim, bool backup)
{
	Path path = sim->dir + sim->file + ".dwi";

	if(backup && (path.attributes() & File::ATR_EXISTS))
	{
		if(!File::moveFile(path.str, path.str + ".old", true))
		{
			String name = path.filename();
			HudError("Could not backup \"%s\".", name.str());
		}
	}

	FileWriter file;
	if(!file.open(path)) return false;

	const Tempo* tempo = sim->tempo;

	// Metadata tags.
	file.printf("#TITLE:%s;\n", sim->title.str());
	file.printf("#ARTIST:%s;\n", sim->artist.str());
	if(sim->genre.len())
		file.printf("#GENRE:%s;\n", sim->genre.str());
	file.printf("#FILE:%s;\n", sim->music.str());

	// Timing offset: GAP is offset negated, in milliseconds.
	file.printf("#GAP:%.6f;\n", -tempo->offset * 1000.0);

	// Music preview.
	if(sim->previewStart > 0.0 || sim->previewLength > 0.0)
	{
		file.printf("#SAMPLESTART:%.3f;\n", sim->previewStart);
		file.printf("#SAMPLELENGTH:%.3f;\n", sim->previewLength);
	}

	// Initial BPM from the first BPM change (at row 0).
	double initialBpm = SIM_DEFAULT_BPM;
	auto bpmBegin = tempo->segments->begin<BpmChange>();
	auto bpmEnd   = tempo->segments->end<BpmChange>();
	if(bpmBegin != bpmEnd)
		initialBpm = bpmBegin->bpm;
	file.printf("#BPM:%.6f;\n", initialBpm);

	// BPM changes after row 0 go into CHANGEBPM.
	{
		bool first = true;
		for(auto it = bpmBegin; it != bpmEnd; ++it)
		{
			if(it->row == 0) continue;
			if(first) { file.printf("#CHANGEBPM:"); first = false; }
			else file.printf(",");
			file.printf("%.6f=%.6f", RowToDwiBeat(it->row), it->bpm);
		}
		if(!first) file.printf(";\n");
	}

	// Stops go into FREEZE (duration in milliseconds).
	{
		auto stopBegin = tempo->segments->begin<Stop>();
		auto stopEnd   = tempo->segments->end<Stop>();
		bool first = true;
		for(auto it = stopBegin; it != stopEnd; ++it)
		{
			if(first) { file.printf("#FREEZE:"); first = false; }
			else file.printf(",");
			file.printf("%.6f=%.6f", RowToDwiBeat(it->row), it->seconds * 1000.0);
		}
		if(!first) file.printf(";\n");
	}

	// Display BPM.
	if(tempo->displayBpmType == BPM_CUSTOM)
	{
		if(tempo->displayBpmRange.min == tempo->displayBpmRange.max)
			file.printf("#DISPLAYBPM:%.6f;\n", tempo->displayBpmRange.min);
		else
			file.printf("#DISPLAYBPM:%.6f..%.6f;\n",
				tempo->displayBpmRange.min, tempo->displayBpmRange.max);
	}
	else if(tempo->displayBpmType == BPM_RANDOM)
	{
		file.printf("#DISPLAYBPM:*;\n");
	}

	// Charts. DWI supports at most one chart per (style, difficulty) combination.
	Vector<int> writtenDiffs;
	for(const Chart* chart : sim->charts)
	{
		const char* dwiTag;
		int numPads;
		if(!GetDwiStyle(chart, dwiTag, numPads))
		{
			HudWarning("Chart style \"%s\" is not supported in DWI format, skipping.",
				chart->style->id.str());
			continue;
		}

		int key = chart->style->index * NUM_DIFFICULTIES + chart->difficulty;
		if(writtenDiffs.find(key) != writtenDiffs.size())
		{
			HudWarning("Duplicate DWI difficulty \"%s %s\", skipping.",
				dwiTag, DifficultyToDwi(chart->difficulty));
			continue;
		}
		if(chart->difficulty != DIFF_EDIT)
			writtenDiffs.push_back(key);

		int numCols = chart->style->numCols;

		file.printf("#%s:%s:%i:\n", dwiTag, DifficultyToDwi(chart->difficulty), chart->meter);
		for(int pad = 0; pad < numPads; ++pad)
		{
			if(pad > 0) file.printf(":");
			WriteNoteDataForPad(file, chart->notes, numCols, pad);
		}
		file.printf(";\n");
	}

	HudInfo("Saved: %s", path.filename().str());
	return true;
}

}; // namespace Dwi
}; // namespace Vortex
