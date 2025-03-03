#include <Precomp.h>

#include <Core/Common/Serialize.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#include <Simfile/History.h>
#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>
#include <Simfile/NoteSet.h>
#include <Simfile/History.h>
#include <Simfile/GameMode.h>

namespace AV {

using namespace std;

Simfile::Simfile()
	: isSelectable(true)
	, title            (EventSystem::getId<TitleChanged>())
	, titleTranslit    (EventSystem::getId<TitleTranslitChanged>())
	, subtitle         (EventSystem::getId<SubtitleChanged>())
	, subtitleTranslit (EventSystem::getId<SubtitleTranslitChanged>())
	, artist           (EventSystem::getId<ArtistChanged>())
	, artistTranslit   (EventSystem::getId<ArtistTranslitChanged>())
	, genre            (EventSystem::getId<GenreChanged>())
	, credit           (EventSystem::getId<CreditChanged>())
	, music            (EventSystem::getId<MusicChanged>())
	, banner           (EventSystem::getId<BannerChanged>())
	, background       (EventSystem::getId<BackgroundChanged>())
	, cdTitle          (EventSystem::getId<CdTitleChanged>())
	, lyricsPath       (EventSystem::getId<LyricsPathChanged>())
	, musicPreview     (EventSystem::getId<MusicPreviewChanged>(), MusicPreview::none)
{
	tempo = make_shared<Tempo>(this);

	mySubscriptions.add<Chart::DescriptionChangedEvent>(this, &Simfile::sortCharts);
	mySubscriptions.add<Chart::MeterChangedEvent>(this, &Simfile::sortCharts);
}

void Simfile::sanitize()
{
	for (int i = 0; i < (int)charts.size(); ++i)
	{
		auto chart = charts[i];
		if (chart->gameMode == nullptr)
		{
			string desc = chart->getFullDifficulty();
			Log::warning(std::format("{} is missing a style, ignoring chart.", desc));
			charts.erase(charts.begin() + i--);
		}
		else
		{
			chart->sanitize();
		}
	}
	tempo->sanitize();
}

void Simfile::sortCharts()
{
	stable_sort(charts.begin(), charts.end(),
	[](const shared_ptr<Chart>& a, const shared_ptr<Chart>& b)
	{
		if (a->gameMode != b->gameMode)
			return a->gameMode->index < b->gameMode->index;

		auto diffA = a->difficulty.get();
		auto diffB = b->difficulty.get();
		if (diffA != diffB)
			return diffA < diffB;

		return a->meter.get() < b->meter.get();
	});
}

static string FindArtworkFile(const DirectoryPath& dir, const char* abbrev, const char* full)
{
	for (auto& name : FileSystem::listFiles(dir, false))
	{
		String::toLower(name);
		string cmp[] = { " ", "-", "_" };
		for (auto& s : cmp)
		{
			string prefix = abbrev + s;
			string postfix = s + abbrev;
			if (String::startsWith(name, prefix.data()) || String::endsWith(name, postfix.data()))
				return name;
		}
		if (name == abbrev || String::find(name, full) != string::npos)
			return name;
	}
	return {};
}

bool Simfile::autofillBanner()
{
	auto file = FindArtworkFile(directory, "bn", "banner");
	if (file.empty()) return false;
	banner.set(file);
	return true;
}

bool Simfile::autofillBackground()
{
	auto file = FindArtworkFile(directory, "bg", "background");
	if (file.empty()) return false;
	background.set(file);
	return true;
}

int Simfile::findChart(const Chart* chart)
{
	for (size_t i = 0, size = charts.size(); i != size; ++i)
	{
		if (charts[i].get() == chart)
			return (int)i;
	}
	return -1;
}

void Simfile::updateTiming()
{
	tempo->updateTiming();
	for (auto chart : charts)
	{
		if (chart->tempo)
		{
			chart->tempo->updateTiming();
		}
	}
}

// =====================================================================================================================
// Simfile :: history functions.

bool Simfile::hasUnsavedChanges() const
{
	for (auto chart : charts)
	{
		if (chart->history->hasUnsavedChanges())
			return true;
	}
	return false;
}

void Simfile::onFileSaved()
{
	for (auto chart : charts)
		chart->history->onFileSaved();
}

void Simfile::addChart(const shared_ptr<Chart>& chart)
{
	charts.emplace_back(chart);
	sortCharts();
	EventSystem::publish<ChartsChanged>();
}

void Simfile::removeChart(const Chart* chart)
{
	for (auto it = charts.begin(); it != charts.end(); ++it)
	{
		if (it->get() == chart)
		{
			charts.erase(it);
			EventSystem::publish<ChartsChanged>();
			return;
		}
	}
	Log::error(std::format("Failed to remove {}, could not find it.", chart->getFullDifficulty()));
}

// =====================================================================================================================
// SimfileMetadata.

MusicPreview::MusicPreview(double start, double length)
	: start(start)
	, length(length)
{
}

const MusicPreview MusicPreview::none(0, 0);

} // namespace AV
