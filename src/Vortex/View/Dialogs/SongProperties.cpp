#include <Precomp.h>

#include <Vortex/View/Dialogs/SongProperties.h>

#include <Core/Utils/String.h>
#include <Core/Utils/Util.h>

#include <Core/System/File.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Image.h>

#include <Core/Interface/Widgets/Grid.h>
#include <Core/Interface/Widgets/LineEdit.h>
#include <Core/Interface/Widgets/Spinner.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/CycleButton.h>
#include <Core/Interface/Widgets/Label.h>

#include <Simfile/Simfile.h>
#include <Simfile/History.h>

#include <Vortex/Editor.h>
#include <Vortex/VortexUtils.h>

#include <Vortex/Edit/TempoTweaker.h>
#include <Vortex/Managers/MusicMan.h>

#include <Vortex/Notefield/Notefield.h>
#include <Vortex/View/View.h>
#include <Vortex/View/Hud.h>

#include <Vortex/Edit/Selection.h>

#define Dlg DialogSongProperties

namespace AV {

using namespace std;

// =====================================================================================================================
// Helper controls.

class SimfileMetadataLineEdit : public WLineEdit
{
public:
	typedef Observable<string> Simfile::* Property;

	SimfileMetadataLineEdit(stringref tooltip, Property prop, EventId propertyChanged)
		: myProperty(prop)
	{
		setTooltip(tooltip);
		mySubscriptions.add<Editor::ActiveSimfileChanged>(
			this, &SimfileMetadataLineEdit::myReadProperty);
		mySubscriptions.add(
			propertyChanged, this, &SimfileMetadataLineEdit::myReadProperty);
		onChange = bind(&SimfileMetadataLineEdit::mySetProperty, this, placeholders::_1);
	}

private:
	void mySetProperty(stringref text)
	{
		if (auto s = Editor::currentSimfile())
			(s->*myProperty).set(text);
	}

	void myReadProperty()
	{
		if (auto s = Editor::currentSimfile())
			setText((s->*myProperty).get());
		else
			setText({});
	}

	EventSubscriptions mySubscriptions;
	Property myProperty;
};

struct Dlg::Form
{
	shared_ptr<WVerticalTabs> tabs;

	shared_ptr<WBannerImage> bannerImage;

	shared_ptr<WCycleButton> displayBpmType;
	shared_ptr<WSpinner> displayBpmMin;
	shared_ptr<WSpinner> displayBpmMax;

	shared_ptr<WLineEdit> previewStart;
	shared_ptr<WLineEdit> previewEnd;

	EventSubscriptions subscriptions;
};

// =====================================================================================================================
// Dialog.

Dlg::~Dlg()
{
	delete myForm;
}

Dlg::Dlg()
{
	setTitle("SIMFILE PROPERTIES");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();

	myUpdateProperties();
	myUpdateBanner();
	myUpdateWidgets();

	myForm->subscriptions.add<Editor::ActiveChartChanged>(this, &Dlg::myUpdateActiveChart);
	myForm->subscriptions.add<Simfile::MusicPreviewChanged>(this, &Dlg::myReadMusicPreview);
	myForm->subscriptions.add<Tempo::DisplayBpmChanged>(this, &Dlg::myReadDisplayBpm);
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitBannerTab();
	myInitMetadataTab();
	myInitTransliterationTab();
	myInitFilesTab();
	myInitPreviewTab();
}

void Dlg::myInitBannerTab()
{
	myForm->bannerImage = make_shared<WBannerImage>();

	myForm->tabs->addTab(myForm->bannerImage, "Banner Artwork");
}

static void InitializeRow(
	const shared_ptr<WGrid>& grid,
	int row,
	SimfileMetadataLineEdit::Property prop,
	EventId propChanged,
	stringref labelText,
	stringref tooltipText)
{
	grid->addWidget(make_shared<WLabel>(labelText), row, 0);
	grid->addWidget(make_shared<SimfileMetadataLineEdit>(tooltipText, prop, propChanged), row, 1);
}

void Dlg::myInitMetadataTab()
{
	auto grid = make_shared<WGrid>();
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 0);
	grid->addColumn(72, 4);
	grid->addColumn(80, 0, 1);

	InitializeRow(grid, 0, &Simfile::title, EventSystem::getId<Simfile::TitleChanged>(),
		"Title", "Title of the song");

	InitializeRow(grid, 1, &Simfile::subtitle, EventSystem::getId<Simfile::SubtitleChanged>(),
		"Subtitle", "Subtitle of the song");

	InitializeRow(grid, 2, &Simfile::artist, EventSystem::getId<Simfile::ArtistChanged>(),
		"Artist", "Artist of the song");

	InitializeRow(grid, 3, &Simfile::credit, EventSystem::getId<Simfile::CreditChanged>(),
		"Credit", "Author of the simfile");

	InitializeRow(grid, 4, &Simfile::genre, EventSystem::getId<Simfile::GenreChanged>(),
		"Genre", "Genre of the music");

	myForm->tabs->addTab(grid, "Main Properties");
}

void Dlg::myInitTransliterationTab()
{
	auto grid = make_shared<WGrid>();
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 0);
	grid->addColumn(72, 4);
	grid->addColumn(80, 0, 1);

	InitializeRow(grid, 0, &Simfile::titleTranslit, EventSystem::getId<Simfile::TitleTranslitChanged>(),
		"Title", "Transliteration of the title to latin script");

	InitializeRow(grid, 1, &Simfile::subtitleTranslit, EventSystem::getId<Simfile::SubtitleTranslitChanged>(),
		"Subtitle", "Transliteration of the subtitle to latin script");

	InitializeRow(grid, 2, &Simfile::artistTranslit, EventSystem::getId<Simfile::ArtistTranslitChanged>(),
		"Artist", "Transliteration of the artist name to latin script");

	auto tab = myForm->tabs->addTab(grid, "Transliterations", false);
}

void Dlg::myInitFilesTab()
{
	auto grid = make_shared<WGrid>();
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 4);
	grid->addRow(22, 0);
	grid->addColumn(72, 4, 0);
	grid->addColumn(80, 4, 1);
	grid->addColumn(22, 0, 0);

	InitializeRow(grid, 0, &Simfile::music, EventSystem::getId<Simfile::MusicChanged>(),
		"Music", "Path of the music file");

	auto findMusic = make_shared<WButton>("{g:search}");
	findMusic->onPress = bind(&Dlg::onFindMusic, this);
	findMusic->setTooltip("Search the stepfile directory for audio files");
	grid->addWidget(findMusic, 0, 2);

	InitializeRow(grid, 1, &Simfile::background, EventSystem::getId<Simfile::BackgroundChanged>(),
		"BG", "Path of the background image\nRecommended size: 640x480 (DDR/ITG) or larger");

	auto findBg = make_shared<WButton>("{g:search}");
	findBg->onPress = bind(&Dlg::onFindBG, this);
	findBg->setTooltip("Search the stepfile directory for background images");
	grid->addWidget(findBg, 1, 2);

	InitializeRow(grid, 2, &Simfile::banner, EventSystem::getId<Simfile::BannerChanged>(),
		"BG", "Path of the banner image\nRecommended size: 256x80 / 512x160 (DDR), 418x164 (ITG)");

	auto findBanner = make_shared<WButton>("{g:search}");
	findBanner->onPress = bind(&Dlg::onFindBanner, this);
	findBanner->setTooltip("Search the stepfile directory for banner images");
	grid->addWidget(findBanner, 2, 2);

	InitializeRow(grid, 3, &Simfile::cdTitle, EventSystem::getId<Simfile::CdTitleChanged>(),
		"CD Title", "Path of the CD title image/simfile author logo\nRecommended size: around 64x48 (DDR/ITG)");

	myForm->tabs->addTab(grid, "Files");
}

void Dlg::myInitPreviewTab()
{
	auto startLabel = make_shared<WLabel>("Music");

	auto start = make_shared<WLineEdit>();
	start->setEditable(false);
	start->setTooltip("The start startTime of the music preview");
	myForm->previewStart = start;

	auto endLabel = make_shared<WLabel>("to");

	auto end = make_shared<WLineEdit>();
	end->setEditable(false);
	end->setTooltip("The end startTime of the music preview");
	myForm->previewEnd = end;

	auto set = make_shared<WButton>("Set");
	set->onPress = bind(&Dlg::onSetPreview, this);
	set->setTooltip("Set the music preview to the selected region");

	auto play = make_shared<WButton>("{g:play}");
	play->onPress = bind(&Dlg::onPlayPreview, this);
	play->setTooltip("Play the music preview");

	auto bpm = make_shared<WLabel>("BPM");

	auto bpmMin = make_shared<WSpinner>();
	bpmMin->onChange = bind(&Dlg::mySetDisplayBpm, this);
	bpmMin->setTooltip("The low value of the display BPM");
	myForm->displayBpmMin = bpmMin;

	auto bpmTo = make_shared<WLabel>("to");
	
	auto bpmMax = make_shared<WSpinner>();
	bpmMax->onChange = bind(&Dlg::mySetDisplayBpm, this);
	bpmMax->setTooltip("The high value of the display BPM");
	myForm->displayBpmMax = bpmMax;

	auto bpmType = make_shared<WCycleButton>();
	bpmType->addItem("Default");
	bpmType->addItem("Custom");
	bpmType->addItem("Random");
	bpmType->onChange = bind(&Dlg::mySetDisplayBpm, this);
	bpmType->setTooltip("Determines how the BPM preview is displayed");
	myForm->displayBpmType = bpmType;

	auto grid = make_shared<WGrid>();

	grid->addRow(22, 4);
	grid->addRow(22, 0);

	grid->addColumn(74, 4, 0);
	grid->addColumn(48, 4, 1);
	grid->addColumn(18, 4, 0);
	grid->addColumn(48, 4, 1);
	grid->addColumn(64, 4, 0);
	grid->addColumn(22, 0, 0);

	grid->addWidget(startLabel, 0, 0);
	grid->addWidget(start, 0, 1);
	grid->addWidget(endLabel, 0, 2);
	grid->addWidget(end, 0, 3);
	grid->addWidget(set, 0, 4);
	grid->addWidget(play, 0, 5);
	grid->addWidget(bpm, 1, 0);
	grid->addWidget(bpmMin, 1, 1);
	grid->addWidget(bpmTo, 1, 2);
	grid->addWidget(bpmMax, 1, 3);
	grid->addWidget(bpmType, 1, 4, 1, 2);

	myForm->tabs->addTab(grid, "Preview");
}

// =====================================================================================================================
// Widget functions.

static void EnableWidget(Widget& w)
{
	w.setEnabled(true);
}

static void DisableWidget(Widget& w)
{
	w.setEnabled(false);
}

void Dlg::myUpdateWidgets()
{
	auto bpmType = myForm->displayBpmType->selectedItem();

	bool enableBpmType = (Editor::currentSimfile() != nullptr);
	bool enableBpmRange = enableBpmType && (bpmType == (int)DisplayBpmType::Custom);

	myForm->displayBpmType->setEnabled(enableBpmType);
	myForm->displayBpmMin->setEnabled(enableBpmRange);
	myForm->displayBpmMax->setEnabled(enableBpmRange);
}

void Dlg::myUpdateProperties()
{
	string blank;
	auto tempo = Editor::currentTempo();
	if (tempo)
	{
		myForm->tabs->setEnabled(true);
	}
	else
	{
		myForm->tabs->setEnabled(false);

		myForm->previewStart->setText(blank);
		myForm->previewEnd->setText(blank);

		myForm->displayBpmMin->setValue(0);
		myForm->displayBpmMax->setValue(0);
		myForm->displayBpmType->setSelectedItem((int)DisplayBpmType::Actual);
	}
}

void Dlg::myUpdateBanner()
{
	auto& banner = myForm->bannerImage->banner;
	banner.texture.reset();

	if (auto sim = Editor::currentSimfile())
	{
		string bnfile = sim->banner.get();
		if (bnfile.length())
		{
			auto path = FilePath(sim->directory, bnfile);
			banner.texture = make_shared<Texture>(path, TextureFormat::RGBA);
			if (!banner.texture->valid())
			{
				Hud::warning("Could not open banner: %s", bnfile.data());
			}
		}
	}
}

// =====================================================================================================================
// Other functions.

void Dlg::myUpdateActiveChart()
{
	auto sim = Editor::currentSimfile();
	myUpdateProperties();
	myUpdateWidgets();
}

void Dlg::onSetPreview()
{
	auto region = Selection::getRegion();
	auto& timing = Editor::currentTempo()->timing;
	double start = timing.rowToTime(region.beginRow);
	double len = timing.rowToTime(region.endRow) - start;
	Editor::currentSimfile()->musicPreview.set(MusicPreview(start, len));
}

void Dlg::onPlayPreview()
{
	if (auto sim = Editor::currentSimfile())
	{
		View::setCursorTime(sim->musicPreview.get().start);
		MusicMan::play();
	}
	// TextureManager::debugDump();
}

void Dlg::onFindMusic()
{
	auto sim = Editor::currentSimfile();
	auto path = Sound::find(sim->directory);
	if (path.empty())
	{
		Hud::note("Could not find any audio files...");
	}
	else
	{
		Editor::currentSimfile()->music.set(path);
	}
}

void Dlg::onFindBanner()
{
	auto sim = Editor::currentSimfile();
	if (!sim->autofillBanner())
		Hud::note("Could not find any banner art...");
}

void Dlg::onFindBG()
{
	auto sim = Editor::currentSimfile();
	if (!sim->autofillBackground())
		Hud::note("Could not find any background art...");
}

void Dlg::mySetMusicPreview()
{
	auto region = Selection::getRegion();
	auto& timing = Editor::currentTempo()->timing;
	double start = timing.rowToTime(region.beginRow);
	double len = timing.rowToTime(region.endRow) - start;
	Editor::currentSimfile()->musicPreview.set({ start, len });
}

void Dlg::myReadMusicPreview()
{
	if (auto sim = Editor::currentSimfile())
	{
		auto& preview = sim->musicPreview.get();
		if (preview.start > 0.0 && preview.length > 0.0)
		{
			myForm->previewStart->setText(String::formatTime(preview.start));
			myForm->previewEnd->setText(String::formatTime(preview.start + preview.length));
			return;
		}
	}
	myForm->previewStart->setText(string());
	myForm->previewEnd->setText(string());
}

void Dlg::mySetDisplayBpm()
{
	if (auto tempo = Editor::currentTempo())
	{
		switch ((DisplayBpmType)myForm->displayBpmType->selectedItem())
		{
		case DisplayBpmType::Actual:
			tempo->displayBpm.set(DisplayBpm::actual);
			break;
		case DisplayBpmType::Custom:
			tempo->displayBpm.set(DisplayBpm(
				myForm->displayBpmMin->value(),
				myForm->displayBpmMax->value(),
				DisplayBpmType::Custom));
			break;
		case DisplayBpmType::Random:
			tempo->displayBpm.set(DisplayBpm::random);
			break;
		};
	}
}

void Dlg::myReadDisplayBpm()
{
	auto dispBpm = DisplayBpm::actual;

	if (auto tempo = Editor::currentTempo())
		dispBpm = tempo->displayBpm.get();

	myForm->displayBpmMin->setValue(dispBpm.low);
	myForm->displayBpmMax->setValue(dispBpm.high);
	myForm->displayBpmType->setSelectedItem((int)dispBpm.type);
}

} // namespace AV
