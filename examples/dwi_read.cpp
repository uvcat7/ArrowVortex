// dwi_read.cpp reads a .DWI file, for older DMX simfiles
// source file created 1/14/12 by Catastrophe
// 10-17-12 the fix for tempo changes ended up being a major rewrite

#include <algorithm>
#include <vector>
#include <stdio.h>

#include "../headers/common.h"
#include "../headers/dwi_read.h"

// these are only written to by readNextTag(). This is cleaner than 5 reference parameters
char tagName[32];
char tagValue[16384];
char tagLevel[16];
char tagLeftSide[16384];
char tagRightSide[16384];

// first the chart is read in as a series of these, then sorted, then the beats are turned into milliseconds (and a real chart)
struct BEAT_NOTE
{
	float          beat;
	int            type;       // one of the enumerated note types
	float          param;      // the length of the stop, or the new bpm, or the beat the hold ends on
	char           column;     // which column this note appears in

	BEAT_NOTE()
	{
		reset();
	}
	void reset()
	{
		beat = type = param = column = 0;
	}
};

// this is a special BEAT_NOTE type only used within this source file
#define HOLD_START 3

void readNextTag(FILE* fp);
// precondition: the globals noted above are only written to by this function, and fp is not null
// postcondition: starts reading at '#', ends reading at ';', and saves whatever it finds to these strings

int getColumn(char ch, char which);
// precondition: ch is valid DWI code, which is 0 or 1 for which side is needed
// postcondition: returns a column index for that DWI code, or -1 for no column

bool sortNoteFunction(struct BEAT_NOTE a, struct BEAT_NOTE b);
// precondition: only to be used by readDWI()
// postcondition: returns true is a is earlier than b

void processChartString(std::vector<struct BEAT_NOTE> *chart, char* string, int side);
// precondition: the songID and chartType must exist on disk
// postcondition: appends a bunch of BEAT_NOTE objects to the chart (singles or doubles)

UTIME getMillisecondsAtBeat(float targetBeat, std::vector<struct BEAT_NOTE> *chart, int startIndex, UTIME startTime, float currentTimePerBeat);
// precondition: the chart is sorted and startIndex is a valid index
// postcondition: loops through the chart until the number of beats have passed, noting any tempo changes

void readNextTag(FILE* fp)
{
	char* parts[5] = { tagName, tagValue, tagLevel, tagLeftSide, tagRightSide };
	int ch = 0;
	int currentPart = -1;
	int currentIndex = 0;

	tagName[0] = tagValue[0] = tagLevel[0] = tagLeftSide[0] = tagRightSide[0] = 0;

	while ( (ch = getc(fp)) != EOF )
	{
		// start with '#' and ignore anything (like extra \r\n) before it
		if ( ch == '#' )
		{
			currentPart = currentIndex = 0;
			continue;
		}
		else if ( currentPart == -1 )
		{
			continue;
		}
		// begin the next section?
		else if ( ch == ':' )
		{
			parts[currentPart][currentIndex] = 0;
			currentPart++;
			currentIndex = 0;
			ASSERT(currentPart < 5);
		}
		// done with all parts?
		else if ( ch == ';' )
		{
			parts[currentPart][currentIndex] = 0;
			return;
		}
		else
		{
			parts[currentPart][currentIndex] = ch;
			currentIndex++;
		}
	}
}

int getColumn(char ch, char which)
{
	if ( which == 0 )
	{
		switch (ch)
		{
		case '4': return 0;
		case '2': return 1;
		case '8': return 2;
		case '6': return 3;
		case '1': return 0;
		case '7': return 0;
		case '9': return 2;
		case '3': return 1;
		case 'B': return 0;
		case 'A': return 1;
		// Extended characters: UL/UR have no 4-panel equivalent; only the valid panel note is returned.
		case 'C': return -1; // UL solo
		case 'D': return -1; // UR solo
		case 'E': return 0;  // L+UL -> L
		case 'F': return 1;  // UL+D -> D
		case 'G': return 2;  // UL+U -> U
		case 'H': return 3;  // UL+R -> R
		case 'I': return 0;  // L+UR -> L
		case 'J': return 1;  // D+UR -> D
		case 'K': return 2;  // U+UR -> U
		case 'L': return 3;  // UR+R -> R
		case 'M': return -1; // UL+UR -> neither
		}
	}
	else
	{
		switch (ch)
		{
		case '1': return 1;
		case '7': return 2;
		case '9': return 3;
		case '3': return 3;
		case 'B': return 3;
		case 'A': return 2;
		// Extended characters C-M all map their second note to a diagonal; no 4-panel equivalent.
		}
	}
	return -1;
}

bool sortNoteFunction(struct BEAT_NOTE a, struct BEAT_NOTE b)
{
	if ( a.beat == b.beat )
	{
		return a.type < b.type; // puts taps ahead of tempo changes, pretty much
	}
	return a.beat < b.beat;
}

void processChartString(std::vector<struct BEAT_NOTE> *chart, char* string, int side)
{
	int currentIndex = 0;
	int beatDivision = 2; // 1/8 beats are the default in DWI Files
	float totalBeatsProcessed = 0.0;
	float prevBeatProcessed = 0.0; // crtical for hold notes
	float beatOfHold[4] = { -1, -1, -1, -1 };

	// when side == 1 then add 4 to every column for the right half of a doubles chart
	int doublesFactor = side == 1 ? 4 : 0;
	
	while ( string[currentIndex] != 0 )
	{
		unsigned char ch = string[currentIndex];
		int columns[4] = { -1, -1, -1, -1 };

		// punctuation has various meanings
		if ( ch == '0' )
		{
			prevBeatProcessed = totalBeatsProcessed;
			totalBeatsProcessed += 1.0/beatDivision;
			currentIndex++;
			continue;
		}
		else if ( ch == '\r' || ch == '\n' )
		{
			currentIndex++;
			continue; // not needed, but nice for debugging below
		}
		else if ( ch == '(' )
		{
			beatDivision = 4; // parens mean 1/16th notes
			currentIndex++;
			continue;
		}
		else if ( ch == '[' )
		{
			beatDivision = 6; // brackets mean 1/24th notes
			currentIndex++;
			continue;
		}
		else if ( ch == '{' )
		{
			beatDivision = 16; // curly braces mean 1/64th notes
			currentIndex++;
			continue;
		}
		else if ( ch == '`' ) // == 96
		{
			beatDivision = 48; // this `00000' nonsense means 192nd notes
			currentIndex++;
			continue;
		}
		else if ( ch == ')' || ch == ']' || ch == '}' || ch == 146 || ch == 39 )
		{
			beatDivision = 2; // back to 1/8th notes
			currentIndex++;
			continue;
		}
		else if ( ch == '!' )
		{
			// what is to the right of the bang?
			currentIndex++; // skip over the bang
			columns[0] = getColumn(string[currentIndex], 0);
			columns[1] = getColumn(string[currentIndex], 1);

			if ( columns[0] != -1 && beatOfHold[columns[0]] == -1 )
			{
				beatOfHold[columns[0]] = prevBeatProcessed; // store when the hold starts
			}
			if ( columns[1] != -1 && beatOfHold[columns[1]] == -1 )
			{
				beatOfHold[columns[1]] = prevBeatProcessed; // store when the hold starts
			}
		}

		// look for regular notes
		else if ( (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'M') )
		{
			columns[0] = getColumn(ch, 0);
			columns[1] = getColumn(ch, 1);

			// X!X notation: this slot is a hold start, not a tap
			if ( string[currentIndex+1] == '!' && string[currentIndex+2] )
			{
				if ( columns[0] != -1 && beatOfHold[columns[0]] == -1 )
					beatOfHold[columns[0]] = totalBeatsProcessed;
				if ( columns[1] != -1 && beatOfHold[columns[1]] == -1 )
					beatOfHold[columns[1]] = totalBeatsProcessed;
				prevBeatProcessed = totalBeatsProcessed;
				totalBeatsProcessed += 1.0/beatDivision;
				currentIndex += 3; // consume X!X
				continue;
			}
		}
		else if ( ch == '<' ) // this special character means "tie columns together" to make triples and quads
		{
			int currentColumn = 0;
			currentIndex++;
			bool startedHolds = false;

			while ( string[currentIndex] != '>' )
			{
				ch = string[currentIndex];
				if ( startedHolds == false )
				{
					if ( ch == '!' )
					{
						startedHolds = true; // a triple is starting a hold
						currentIndex++;
						continue;
					}
					if ( getColumn(ch, 0) != -1 )
					{
						columns[currentColumn] = getColumn(ch, 0);
						currentColumn = currentColumn == 3 ? 3 : currentColumn + 1;
					}
					if ( getColumn(ch, 1) != -1 )
					{
						columns[currentColumn] = getColumn(ch, 1);
						currentColumn = currentColumn == 3 ? 3 : currentColumn + 1;
					}
				}
				else
				{
					// Hold starts from inside a <> combination tag.
					// Remove the column from the tap list so it becomes a pure hold head.
					int col0 = getColumn(ch, 0);
					int col1 = getColumn(ch, 1);

					if ( col0 != -1 && beatOfHold[col0] == -1 )
					{
						beatOfHold[col0] = totalBeatsProcessed;
						for ( int d = 0; d < 4; d++ )
							if ( columns[d] == col0 ) { columns[d] = -1; break; }
					}
					if ( col1 != -1 && beatOfHold[col1] == -1 )
					{
						beatOfHold[col1] = totalBeatsProcessed;
						for ( int d = 0; d < 4; d++ )
							if ( columns[d] == col1 ) { columns[d] = -1; break; }
					}
				}
				currentIndex++;
			}
		}
		else
		{
			al_trace("Unknown symbol: %c", ch);
		}

		// create a freeze note? the end of a hold is signaled by a tap note in a column that was marked as held on a previous pass
		if ( ch != '!' )
		{
			for ( int d = 0; d < 4; d++ )
			{
				if ( columns[d] != -1 && beatOfHold[columns[d]] != -1 )
				{
					struct BEAT_NOTE a;
					a.beat = beatOfHold[columns[d]];
					a.column = columns[d] + doublesFactor;
					a.type = HOLD_START;
					a.param = totalBeatsProcessed;

					beatOfHold[columns[d]] = -1; // clear the hold
					columns[d] = -1; // clear the tap which ended this hold
					chart->push_back(a); 
				}
			}
		}

		// this can happen if a hold ends - sort all the -1 columns to the end of the list
		// for example {-1, 2, -1, -1} becomes {2, -1, -1, -1}
		if ( columns[0] == -1 && (columns[1] != -1 || columns[2] != -1 || columns[3] != -1) )
		{
			columns[0] = columns[1];
			columns[1] = columns[2];
			columns[2] = columns[3];
			columns[3] = -1;
		}
		if ( columns[1] == -1 && (columns[2] != -1 || columns[3] != -1) )
		{
			columns[1] = columns[2];
			columns[2] = columns[3];
			columns[3] = -1;
		}
		if ( columns[2] == -1 && columns[3] != -1 )
		{
			columns[2] = columns[3];
			columns[3] = -1;
		}

		// create any notes that need to be made
		if ( ch != '!' ) // this doesn't count as a note, nor should it increment time
		{
			for ( int d = 0; d < 4; d++ )
			{
				if ( columns[d] != -1 )
				{
					struct BEAT_NOTE a;
					a.beat = totalBeatsProcessed;
					a.column = columns[d] + doublesFactor;
					a.type = TAP;
					chart->push_back(a);
				}
			}

			prevBeatProcessed = totalBeatsProcessed;
			totalBeatsProcessed += 1.0/beatDivision;
		}
		currentIndex++;
	}

	// finally, put an end-of-song marker here
	struct BEAT_NOTE endmarker;
	endmarker.beat = totalBeatsProcessed;
	endmarker.type = END_SONG;
	chart->push_back(endmarker);
}

// NOTE: currently only used for calculating the end time of hold notes
UTIME getMillisecondsAtBeat(float targetBeat, std::vector<struct BEAT_NOTE> *chart, int startIndex, UTIME startTime, float currentTimePerBeat)
{
	UTIME retval = startTime;
	float msPerBeat = currentTimePerBeat;
	float lastBeatProcessed = chart->at(startIndex).beat;
	float overshoot = 0;
	unsigned int i = 0;

	if ( lastBeatProcessed > targetBeat )
	{
		al_trace("Searching for a past beat. Set breakpoint.\r\n");
	}

	for ( i = startIndex; i < chart->size(); i++ )
	{
		// check for finishing
		if ( targetBeat == chart->at(i).beat )
		{
			break;
		}
		if ( targetBeat < chart->at(i).beat )
		{
			overshoot = chart->at(i).beat - targetBeat; // fix a bug where not taking this into account was causing holds to run until the next note!
			break;
		}

		if ( chart->at(i).type == BPM_CHANGE )
		{
			retval += ((chart->at(i).beat - lastBeatProcessed) * msPerBeat);
			msPerBeat = BPM_TO_MSEC(chart->at(i).param);
			lastBeatProcessed = chart->at(i).beat;
		}
		if ( chart->at(i).type == SCROLL_STOP )
		{
			retval += chart->at(i).param;
		}
	}

	if ( i == chart->size() )
	{
		i--;
	}

	retval += ((chart->at(i).beat - lastBeatProcessed - overshoot) * msPerBeat);
	return retval;
}

int readDWI(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	char filename[] = "DATA/dwis/101.dwi";
	FILE* fp = NULL;
	std::vector<struct BEAT_NOTE> beats;

	// hopefully open the target DWI file
	filename[10] = (songID/100 % 10) + '0';
	filename[11] = (songID/10 % 10) + '0';
	filename[12] = (songID % 10) + '0';

	if ( fopen_s(&fp, filename, "rt") != 0 )
	{
		// as a fall back, open a memory dump of a chart (legacy chart data)
		return readDMXSQ(chart, holds, songID, chartType);
	}

	// fix any potential bugs regarding data from one chart accidentally merging into the next chart
	chart->clear();
	holds->clear();
	int gap = 0;
	float timePerBeat = 400.0; // 150 BPM

	// start reading tags
	readNextTag(fp);
	while ( tagName[0] != 0 )
	{
		if ( _strcmpi("SINGLE", tagName) == 0 )
		{
			if ( _strcmpi("BASIC", tagValue) == 0 && chartType == SINGLE_MILD )
			{
				processChartString(&beats, tagLeftSide, 0);
			}
			else if ( _strcmpi("ANOTHER", tagValue) == 0 && chartType == SINGLE_WILD )
			{
				processChartString(&beats, tagLeftSide, 0);
			}
			else if ( _strcmpi("MANIAC", tagValue) == 0 && chartType == SINGLE_ANOTHER )
			{
				processChartString(&beats, tagLeftSide, 0);
			}
		}
		if ( _strcmpi("DOUBLE", tagName) == 0 )
		{
			if ( _strcmpi("BASIC", tagValue) == 0 && chartType == DOUBLE_MILD )
			{
				processChartString(&beats, tagLeftSide, 0);
				processChartString(&beats, tagRightSide, 1);
			}
			else if ( _strcmpi("ANOTHER", tagValue) == 0 && chartType == DOUBLE_WILD )
			{
				processChartString(&beats, tagLeftSide, 0);
				processChartString(&beats, tagRightSide, 1);
			}
			else if ( _strcmpi("MANIAC", tagValue) == 0 && chartType == DOUBLE_ANOTHER )
			{
				processChartString(&beats, tagLeftSide, 0);
				processChartString(&beats, tagRightSide, 1);
			}
		}
		if ( _strcmpi("BPM", tagName) == 0 )
		{
			float bpm = atof(tagValue);
			timePerBeat = BPM_TO_MSEC(bpm);

			// set the initial scroll rate // WHY DOES THIS MAKE SONG NOT END? affects end of song marker somehow?
			struct ARROW a;
			a.timing = 0;
			a.color = bpm;
			a.type = BPM_CHANGE;
			a.judgement = UNSET;
			chart->push_back(a);
		}
		if ( _strcmpi("GAP", tagName) == 0 )
		{
			gap = atoi(tagValue);
			al_trace("gap = %d\r\n", gap);
		}
		if ( _strcmpi("CHANGEBPM", tagName) == 0 ) // #CHANGEBPM:992.000=95.000,1016.000=190.000;
		{
			char* token, *next;
			token = strtok_s(tagValue, ",;", &next);

			while ( token != NULL )
			{
				char leftSide[32], rightSide[32];
				char* equalsPos = strchr(token, '=');
				if ( equalsPos == NULL )
				{
					continue;
				}
				strncpy_s(leftSide, token, equalsPos - token);
				strcpy_s(rightSide, equalsPos+1);

				struct BEAT_NOTE b;
				b.beat = atof(leftSide)/4.0;
				b.param = atof(rightSide);
				b.type = BPM_CHANGE;
				beats.push_back(b);

				token = strtok_s(NULL, ",;", &next);
			}
		}
		if ( _strcmpi("FREEZE", tagName) == 0 ) // #FREEZE:668.000=327.000,1292.000=967.000;
		{
			char* token, *next;
			token = strtok_s(tagValue, ",;", &next);

			while ( token != NULL )
			{
				char leftSide[32], rightSide[32];
				char* equalsPos = strchr(token, '=');
				if ( equalsPos == NULL )
				{
					continue;
				}
				strncpy_s(leftSide, token, equalsPos - token);
				strcpy_s(rightSide, equalsPos+1);

				struct BEAT_NOTE b;
				b.beat = atof(leftSide)/4.0;
				b.param = atof(rightSide);
				b.type = SCROLL_STOP;
				beats.push_back(b);
				
				token = strtok_s(NULL, ",;", &next);
			}
		}

		// next loop
		readNextTag(fp);
	}

	sort(beats.begin(), beats.end(), sortNoteFunction);

	// for each item in the beats vector, create a struct ARROW (real chart object) and translate 'DWI beats' to milliseconds
	int numNotes = beats.size();
	float currentTime = gap;
	float lastBeatProcessed = 0.0;

	for ( int i = 0; i < numNotes; i++ )
	{
		struct ARROW a;
		struct FREEZE f;
		float beatsDifference = beats[i].beat - lastBeatProcessed;
		lastBeatProcessed = beats[i].beat;

		currentTime += timePerBeat*beatsDifference;

		if (currentTime < 0)
		{
			continue; // uh-oh;
		}
		al_trace("%f\r\n", currentTime);

		switch ( beats[i].type )
		{
		case TAP:
			a.timing = currentTime;
			a.color = calculateArrowColor(a.timing, timePerBeat);
			a.type = TAP;
			a.columns[0] = beats[i].column;
			a.judgement = UNSET;
			chart->push_back(a);
			break;
		case HOLD_START:
			f.startTime = currentTime;
			f.columns[0] = beats[i].column;
			f.endTime1 = getMillisecondsAtBeat(beats[i].param, &beats, i, currentTime, timePerBeat); //it ends at beat beats[i].param
			holds->push_back(f); 
			break;
		case BPM_CHANGE:
			a.timing = currentTime;
			a.color = beats[i].param;
			a.type = BPM_CHANGE;
			a.judgement = UNSET;
			chart->push_back(a);

			timePerBeat = BPM_TO_MSEC(beats[i].param); // new tempo! the length of a beat has henceforth and immediately changed
			break;
		case SCROLL_STOP:
			a.timing = currentTime;
			a.color = beats[i].param;
			a.type = SCROLL_STOP;
			chart->push_back(a);

			currentTime += beats[i].param; // advance the time
			break;
		case END_SONG:
			a.timing = currentTime + 1000; // TODO: something better than this, maybe check the mp3?
			a.type = END_SONG;
			chart->push_back(a);
			break;
		default:
			al_trace("IMPOSSIBLE NOTE TYPE IN readDWI() %d\r\n", beats[i].type);
		}
	}

	fclose(fp);

	// count the maximum score that this chart is worth. it is needed while the chart is being played
	int maxScore = 0;
	for ( size_t i = 0; i < chart->size(); i++ )
	{
		if ( chart->at(i).type == TAP || chart->at(i).type == JUMP )
		{
			maxScore += 2;
		}
	}
	maxScore += holds->size()*2;
	return maxScore;
}

int readDWI2P(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readDWI(chart, holds, songID, chartType);

	for ( std::vector<struct ARROW>::iterator c = chart->begin(); c != chart->end(); c++ )
	{
		for ( int i = 0; i < 4; i++ )
		{
			if ( c->columns[i] >= 0 && c->columns[i] < 4 )
			{
				c->columns[i] += 4;
			}
		}
	}

	for ( std::vector<struct FREEZE>::iterator f = holds->begin(); f != holds->end(); f++ )
	{
		for ( int i = 0; i < 2; i++ )
		{
			if ( f->columns[i] >= 0 && f->columns[i] < 4 )
			{
				f->columns[i] += 4;
			}
		}
	}

	return retval;
}

int readDWICenter(std::vector<struct ARROW> *chart, std::vector<struct FREEZE> *holds, int songID, int chartType)
{
	int retval = readDWI(chart, holds, songID, chartType);

	for ( std::vector<struct ARROW>::iterator c = chart->begin(); c != chart->end(); c++ )
	{
		for ( int i = 0; i < 4; i++ )
		{
			switch ( c->columns[i] )
			{
			case 0: c->columns[i] = 3; break;
			case 1: c->columns[i] = 2; break;
			case 2: c->columns[i] = 5; break;
			case 3: c->columns[i] = 4; break;
			default:
				break;
			}
		}
	}

	for ( std::vector<struct FREEZE>::iterator f = holds->begin(); f != holds->end(); f++ )
	{
		for ( int i = 0; i < 2; i++ )
		{
			switch ( f->columns[i] )
			{
			case 0: f->columns[i] = 3; break;
			case 1: f->columns[i] = 2; break;
			case 2: f->columns[i] = 5; break;
			case 3: f->columns[i] = 4; break;
			default:
				break;
			}
		}
	}

	return retval;
}

