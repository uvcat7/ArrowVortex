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
//
// DWI represents notes using a numpad layout:
//   7 8 9
//   4 5 6
//   1 2 3
// Single-direction notes use their numpad digit (4=Left, 2=Down, 8=Up, 6=Right).
// Two simultaneous notes use the corner digit between them (e.g. Up+Left = 7).
// Six-panel solo adds diagonal directions: C=UpLeft, D=UpRight, and letter combos E-M.

enum DwiNote
{
	NO_NOTE = 0,
	NOTE_L, NOTE_UL, NOTE_D, NOTE_U, NOTE_UR, NOTE_R,
	NOTE_COUNT
};

// Converts an ArrowVortex column index to a DWI note enum for the given pad.
// For double (8-col) charts, each pad owns 4 columns starting at pad*4.
static DwiNote ColToDwiNote(int col, int numCols, int pad)
{
	int localCol  = col - (numCols == 8 ? pad * 4 : 0);
	int padColCount = (numCols == 8) ? 4 : numCols;
	if(localCol < 0 || localCol >= padColCount) return NO_NOTE;
	switch(padColCount)
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

// Returns the single DWI character for one note direction.
static char DwiNoteToChar(DwiNote note)
{
	switch(note)
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

// Returns a single DWI character encoding both notes simultaneously.
// Two notes that share a corner on the numpad map to that corner digit
// (e.g. Up+Left -> '7', Down+Right -> '3'). Returns '0' for unknown pairs.
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

// One note event at a specific row — either a tap, a hold head, or a hold tail.
// Hold tails (isHoldHead=false at endrow) are emitted as plain note chars,
// which the DWI loader uses to close the in-progress hold on that column.
struct NoteEvent
{
	int  row;
	int  col;
	bool isHoldHead; // true = hold starts here; false = tap or hold tail
};

// Emits 1-or-2 note characters for the given array of DWI notes (no surrounding brackets).
// 1 note  -> single-direction character (e.g. '8' for Up)
// 2 notes -> corner pair character      (e.g. '7' for Up+Left)
// 3+ notes -> individual characters in sequence; caller must wrap in <> if needed
static void EmitNoteChars(FileWriter& file, const DwiNote* notes, int count)
{
	if(count == 1)
		file.printf("%c", DwiNoteToChar(notes[0]));
	else if(count == 2)
		file.printf("%c", DwiPairToChar(notes[0], notes[1]));
	else
		for(int i = 0; i < count; ++i)
			file.printf("%c", DwiNoteToChar(notes[i]));
}

// Emits one DWI note slot (one position on the quantization grid).
//
// taps:      notes firing as plain taps or hold tails at this row
// holdHeads: notes starting a hold at this row (marked with ! in DWI)
//
// Output format by case:
//   empty                       -> "0"
//   1-2 taps, no holds          -> single/pair char,  e.g. "7"
//   3+ taps, no holds           -> <ABC>
//   1-2 hold heads, no taps     -> X!X or PairChar!PairChar
//   3+ hold heads, no taps      -> <ABC!ABC>
//   1 tap + 1 hold head         -> CornerChar!HoldChar, e.g. "7!4" (Up tap, Left hold)
//   3+ total (mixed)            -> <(taps)(holdHeads)!(holdHeads)>
static void EmitNoteSlot(FileWriter& file,
	DwiNote* taps,      int nTaps,
	DwiNote* holdHeads, int nHoldHeads)
{
	if(nTaps == 0 && nHoldHeads == 0)
	{
		file.printf("0");
	}
	else if(nHoldHeads == 0)
	{
		// Taps only: 1-2 as a single/pair char, 3+ wrapped in <>.
		if(nTaps <= 2)
			EmitNoteChars(file, taps, nTaps);
		else
		{
			file.printf("<");
			EmitNoteChars(file, taps, nTaps);
			file.printf(">");
		}
	}
	else if(nTaps == 0)
	{
		// Hold heads only: X!X or PairChar!PairChar for 1-2, <ABC!ABC> for 3+.
		if(nHoldHeads <= 2)
		{
			EmitNoteChars(file, holdHeads, nHoldHeads);
			file.printf("!");
			EmitNoteChars(file, holdHeads, nHoldHeads);
		}
		else
		{
			file.printf("<");
			EmitNoteChars(file, holdHeads, nHoldHeads);
			file.printf("!");
			EmitNoteChars(file, holdHeads, nHoldHeads);
			file.printf(">");
		}
	}
	else if(nTaps == 1 && nHoldHeads == 1)
	{
		// One tap + one hold head: corner pair char encodes both, ! marks the hold.
		// e.g. Up tap + Left hold head -> "7!4"
		file.printf("%c!%c", DwiPairToChar(taps[0], holdHeads[0]), DwiNoteToChar(holdHeads[0]));
	}
	else
	{
		// 3+ total notes (mixed taps and hold heads): use <> brackets.
		// Before !: taps followed by hold heads. After !: hold heads repeated.
		file.printf("<");
		EmitNoteChars(file, taps, nTaps);
		EmitNoteChars(file, holdHeads, nHoldHeads);
		file.printf("!");
		EmitNoteChars(file, holdHeads, nHoldHeads);
		file.printf(">");
	}
}

// Writes all note data for one pad of a chart.
//
// ArrowVortex uses 192 rows per measure (48 rows/beat * 4 beats). Each measure
// is written as one line to stay within the 4096-byte line buffers in DMX loaders.
//
// Three quantization modes, chosen per-measure based on where the notes fall:
//   mode 0 (default)  -> bare chars,     8th-note grid,   24 rows/slot, 8 slots/measure
//   mode 1 ( )        -> 16th-note grid, 12 rows/slot,   16 slots/measure
//   mode 2 ` '        -> 192nd-note grid, 1 row/slot,   192 slots/measure
// The { } 64th-note bracket is intentionally omitted: its beat division
// differs between ArrowVortex and DMX-family loaders.
static void WriteNoteDataForPad(FileWriter& file, const NoteList& notes, int numCols, int pad)
{
	// const int ROWS_PER_BEAT    = 48;
	const int ROWS_PER_MEASURE = ROWS_PER_BEAT * 4; // 192 rows; assumes 4/4 time

	// Column range owned by this pad.
	int padStart    = (numCols == 8) ? pad * 4 : 0;
	int padColCount = (numCols == 8) ? 4 : numCols;

	// Flatten hold notes into (row, col, isHoldHead) events so we can process
	// the chart one row at a time. Each hold produces two events: one at the
	// head row (isHoldHead=true) and one at the tail row (isHoldHead=false).
	// Taps produce a single event with isHoldHead=false.
	Vector<NoteEvent> noteEvents;
	for(const Note& note : notes)
	{
		int col = (int)note.col;
		if(col < padStart || col >= padStart + padColCount) continue;
		if(note.type != NOTE_STEP_OR_HOLD) continue;
		if(note.endrow > note.row)
		{
			noteEvents.push_back({note.row,    col, true});
			noteEvents.push_back({note.endrow, col, false});
		}
		else
		{
			noteEvents.push_back({note.row, col, false});
		}
	}
	if(noteEvents.empty())
	{
		file.printf("0");
		return;
	}

	std::sort(noteEvents.begin(), noteEvents.end(), [](const NoteEvent& a, const NoteEvent& b)
	{
		return (a.row != b.row) ? (a.row < b.row) : (a.col < b.col);
	});

	int lastRow     = noteEvents.back().row;
	int lastMeasure = lastRow / ROWS_PER_MEASURE;
	int eventCount  = (int)noteEvents.size();
	int evIdx       = 0; // advances forward as we consume events measure by measure

	for(int measure = 0; measure <= lastMeasure; measure++)
	{
		int measureStart = measure * ROWS_PER_MEASURE;
		int measureEnd   = measureStart + ROWS_PER_MEASURE;

		// Scan ahead (without consuming) to pick the quantization mode for this measure.
		int mode = 0;
		for(int scanIdx = evIdx; scanIdx < eventCount && noteEvents[scanIdx].row < measureEnd; scanIdx++)
		{
			if(noteEvents[scanIdx].row % 12 != 0) { mode = 2; break; }
			if(noteEvents[scanIdx].row % 24 != 0)   mode = 1;
		}

		int rowStep = (mode == 0) ? 24 : (mode == 1) ? 12 : 1;

		if(mode == 1)      file.printf("(");
		else if(mode == 2) file.printf("`");

		for(int row = measureStart; row < measureEnd; row += rowStep)
		{
			// Collect all events at this row, split into taps/tails and hold heads.
			DwiNote taps[8];      int nTaps      = 0;
			DwiNote holdHeads[8]; int nHoldHeads = 0;

			while(evIdx < eventCount && noteEvents[evIdx].row == row)
			{
				const NoteEvent& event = noteEvents[evIdx++];
				DwiNote dwiNote = ColToDwiNote(event.col, numCols, pad);
				if(dwiNote == NO_NOTE) continue;
				if(event.isHoldHead)
					holdHeads[nHoldHeads++] = dwiNote;
				else
					taps[nTaps++] = dwiNote;
			}

			EmitNoteSlot(file, taps, nTaps, holdHeads, nHoldHeads);
		}

		if(mode == 1)      file.printf(")\n");
		else if(mode == 2) file.printf("'\n");
		else               file.printf("\n");
	}
}

// ========================================================================================
// Style resolution.

// Maps an ArrowVortex chart style to the corresponding DWI tag name and pad count.
// Returns false if the style has no DWI equivalent.
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

	// Timing offset: GAP is the offset negated, in milliseconds.
	file.printf("#GAP:%.6f;\n", -tempo->offset * 1000.0);

	// Music preview.
	if(sim->previewStart > 0.0 || sim->previewLength > 0.0)
	{
		file.printf("#SAMPLESTART:%.3f;\n", sim->previewStart);
		file.printf("#SAMPLELENGTH:%.3f;\n", sim->previewLength);
	}

	// Initial BPM comes from the first BPM change segment (always at row 0).
	double initialBpm = SIM_DEFAULT_BPM;
	auto bpmBegin = tempo->segments->begin<BpmChange>();
	auto bpmEnd   = tempo->segments->end<BpmChange>();
	if(bpmBegin != bpmEnd)
		initialBpm = bpmBegin->bpm;
	file.printf("#BPM:%.6f;\n", initialBpm);

	// BPM changes after row 0 go into CHANGEBPM as a comma-separated beat=bpm list.
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

	// Stops go into FREEZE as a comma-separated beat=milliseconds list.
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
