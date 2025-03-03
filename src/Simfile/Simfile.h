#pragma once

#include <Core/Common/NonCopyable.h>
#include <Core/Common/Event.h>
#include <Core/System/File.h>

#include <Simfile/Common.h>

namespace AV {

// Hold data for a foreground/background change.
struct BgChange
{
	std::string effect;
	std::string file;
	std::string file2;
	std::string color;
	std::string color2;
	std::string transition;
	double startBeat = 0;
	double rate = 0;
};

// Holds the music preview start and length.
struct MusicPreview
{
	MusicPreview(double start, double length);

	double start; // Start startTime in seconds.
	double length; // Length in seconds.

	static const MusicPreview none;

	auto operator <=> (const MusicPreview&) const = default;
};

// Holds data for the entire simfile.
class Simfile : NonCopyable
{
public:
	Simfile();

	void sanitize();
	void sortCharts();

	bool autofillBanner();
	bool autofillBackground();

	// Returns the index of the chart in the chart list, or -1 if the chart was not found.
	int findChart(const Chart* chart);

	// Updates the timing data of the simfile tempo and all charts with split tempo.
	void updateTiming();

	// Returns true if the simfile history or any of the chart histories have unsaved changes.
	bool hasUnsavedChanges() const;

	void onFileSaved();

// Editing functions:

	// Adds a chart to the simfile.
	void addChart(const shared_ptr<Chart>& chart);

	// Removes a chart from the simfile.
	// Does not actually delete the chart, since the action is reversible.
	void removeChart(const Chart* chart);

// Properties:

	// Song title, in the original language.
	Observable<std::string> title;

	// Song title, transliterated to latin script.
	Observable<std::string> titleTranslit;

	// Song subtitle, in the original language.
	Observable<std::string> subtitle;

	// Song subtitle, transliterated to latin script.
	Observable<std::string> subtitleTranslit;

	// Song artist, in the original language.
	Observable<std::string> artist;

	// Song artist, transliterated to latin script.
	Observable<std::string> artistTranslit;

	// Song genre.
	Observable<std::string> genre;

	// Author of the simfile.
	Observable<std::string> credit;

	// Path to the music file, relative to the simfile.
	Observable<std::string> music;

	// Path to the banner file, relative to the simfile.
	Observable<std::string> banner;

	// Path to the background file, relative to the simfile.
	Observable<std::string> background;

	// Path to the CD title file, relative to the simfile.
	Observable<std::string> cdTitle;

	// Path to the lyrics file, relative to the simfile.
	Observable<std::string> lyricsPath;

	// Start and length of the music preview clip.
	Observable<MusicPreview> musicPreview;

	// List of foreground changes.
	vector<BgChange> fgChanges;

	// List of background changes (2 layers).
	vector<BgChange> bgChanges[2];

	// List of paths to keysound files.
	vector<std::string> keysounds;

	// Indicates if the simfile is selectable during regular gameplay.
	bool isSelectable;

	// Properties read from the simfile that are unknown.
	vector<std::pair<std::string, std::string>> misc;

	// List of charts that are part of the simfile.
	vector<shared_ptr<Chart>> charts;

	// Global tempo for charts in the simfile that do not have their own tempo.
	shared_ptr<Tempo> tempo;

	// Directory from which the simfile was loaded.
	DirectoryPath directory;

	// Filename from which the simfile was loaded, without extension.
	std::string filename;

	// Format from which the simfile was loaded or to which it was last saved (e.g. "Stepmania 5").
	std::string format;

// Events:

	struct ChartsChanged : Event {};
	struct TitleChanged : Event {};
	struct TitleTranslitChanged : Event {};
	struct SubtitleChanged : Event {};
	struct SubtitleTranslitChanged : Event {};
	struct ArtistChanged : Event {};
	struct ArtistTranslitChanged : Event {};
	struct GenreChanged : Event {};
	struct CreditChanged : Event {};
	struct MusicChanged : Event {};
	struct BannerChanged : Event {};
	struct BackgroundChanged : Event {};
	struct CdTitleChanged : Event {};
	struct LyricsPathChanged : Event {};
	struct MusicPreviewChanged : Event {};

private:
	EventSubscriptions mySubscriptions;
};

} // namespace AV
