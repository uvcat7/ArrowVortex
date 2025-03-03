#pragma once

#include <Core/Vector.h>
#include <Core/NonCopyable.h>

#include <Simfile/Common.h>

namespace Vortex {

/// Formats from which the simfile can be loaded/saved.
enum SimFormat
{
	SIM_NONE,

	SIM_SM,
	SIM_SSC,
	SIM_OSU,
	SIM_OSZ,
	SIM_DWI,

	NUM_SIMFILE_FORMATS
};

/// Hold data for a foreground/background change.
struct BgChange
{
	String effect;
	String file, file2;
	String color, color2;
	String transition;
	double startBeat;
	double rate;
};

/// Holds the simfile metadata.
struct Simfile : NonCopyable
{
	Simfile();
	~Simfile();

	void sanitize();

	Vector<Chart*> charts;
	Tempo* tempo;

	String dir;
	String file;
	SimFormat format;

	String title, titleTr;
	String subtitle, subtitleTr;
	String artist, artistTr;
	String genre;
	String credit;

	String music;
	String banner;
	String background;
	String cdTitle;
	String lyricsPath;

	Vector<BgChange> fgChanges;
	Vector<BgChange> bgChanges[2];

	double previewStart;
	double previewLength;

	bool isSelectable;
};

// Returns the abbreviation of the given simfile format.
const char* GetFormatAbbreviation(SimFormat format);

}; // namespace Vortex
