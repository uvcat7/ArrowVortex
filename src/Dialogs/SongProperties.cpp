#include <Dialogs/SongProperties.h>

#include <Core/StringUtils.h>
#include <Core/Utils.h>

#include <System/File.h>

#include <Core/Draw.h>

#include <Editor/Notefield.h>
#include <Editor/View.h>
#include <Editor/Selection.h>
#include <Editor/History.h>

#include <Managers/TempoMan.h>
#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Music.h>

namespace Vortex {

enum BannerSize
{
	BANNER_W = 418,
	BANNER_H = 164
};

struct DialogSongProperties::BannerWidget : public GuiWidget
{
	BannerWidget(GuiContext* gui) : GuiWidget(gui)
	{
		width_ = BANNER_W;
		height_ = BANNER_H;
	}
	void onDraw() override
	{
		recti r = rect_;
		if(tex.handle())
		{
			Draw::fill(rect_, Colors::white, tex.handle());
		}
		else
		{
			Draw::fill(rect_, Color32(26));
		}
	}
	Texture tex;
};

DialogSongProperties::~DialogSongProperties()
{
	gNotefield->toggleShowSongPreview();
}

DialogSongProperties::DialogSongProperties()
{
	gNotefield->toggleShowSongPreview();

	setTitle("SIMFILE PROPERTIES");

	myCreateWidgets();

	myUpdateProperties();
	myUpdateBanner();
	myUpdateWidgets();
}

// ================================================================================================
// GuiWidget functions.

template <typename T>
static WgLineEdit* CreateField(RowLayout& layout, StringRef label, String& str, T func)
{
	WgLineEdit* edit = layout.add<WgLineEdit>(label);
	edit->text.bind(&str);
	edit->onChange.bind<void, String&>(func, str);
	return edit;
}

void DialogSongProperties::myCreateWidgets()
{
	myLayout.row().col(418);
	myBannerWidget = myLayout.add<BannerWidget>();
	myLayout.add<WgSeperator>();

	myLayout.row().col(72).col(342);

	GuiWidget* w;

	w = CreateField(myLayout, "Title", myTitle, [](String& s){ gMetadata->setTitle(s); });
	w->setTooltip("Title of the song");

	w = CreateField(myLayout, "Subtitle", mySubtitle, [](String& s){ gMetadata->setSubtitle(s); });
	w->setTooltip("Subtitle of the song");

	w = CreateField(myLayout, "Artist", myArtist, [](String& s){ gMetadata->setArtist(s); });
	w->setTooltip("Artist of the song");

	w = CreateField(myLayout, "Credit",   myCredit,   [](String& s){ gMetadata->setCredit(s); });
	w->setTooltip("Author of the simfile");

	myLayout.row().col(418);
	myLayout.add<WgSeperator>();
	myLayout.row().col(72).col(314).col(24);

	w = CreateField(myLayout, "Music", myMusic, [](String& s){ gMetadata->setMusicPath(s); });
	w->setTooltip("Path of the music file");

	auto findMusic = myLayout.add<WgButton>();
	findMusic->onPress.bind(this, &DialogSongProperties::onFindMusic);
	findMusic->text.set("{g:search}");
	findMusic->setTooltip("Search the stepfile directory for audio files");

	w = CreateField(myLayout, "BG", myBackground, [](String& s){ gMetadata->setBackgroundPath(s); });
	w->setTooltip("Path of the background image\nRecommended size: 640x480 (DDR/ITG) or larger");

	auto findBG = myLayout.add<WgButton>();
	findBG->onPress.bind(this, &DialogSongProperties::onFindBG);
	findBG->text.set("{g:search}");
	findBG->setTooltip("Search the stepfile directory for background images");

	w = CreateField(myLayout, "Banner", myBanner, [](String& s){ gMetadata->setBannerPath(s); });
	w->setTooltip("Path of the banner image\nRecommended size: 256x80 / 512x160 (DDR), 418x164 (ITG)");

	auto findBanner = myLayout.add<WgButton>();
	findBanner->onPress.bind(this, &DialogSongProperties::onFindBanner);
	findBanner->text.set("{g:search}");
	findBanner->setTooltip("Search the stepfile directory for banner images");

	//myLayout.row().col(72).col(342);

	w = CreateField(myLayout, "CD Title", myCdTitle, [](String& s){ gMetadata->setCdTitlePath(s); });
	w->setTooltip("Path of the CD title image (logo of the simfile author)\nRecommended size: around 64x48 (DDR/ITG)");

	auto findCDTitle = myLayout.add<WgButton>();
	findCDTitle->onPress.bind(this, &DialogSongProperties::onFindCdTitle);
	findCDTitle->text.set("{g:search}");
	findCDTitle->setTooltip("Search the stepfile directory for CD title images");

	myLayout.row().col(418);
	myLayout.add<WgSeperator>();
	myLayout.row().col(72).col(96).col(16).col(96).col(94).col(24);

	auto previewStart = myLayout.add<WgLineEdit>("Preview");
	previewStart->text.bind(&myPreviewStart);
	previewStart->setEditable(false);
	previewStart->setTooltip("The start time of the music preview");

	auto previewEnd = myLayout.add<WgLineEdit>("to");
	previewEnd->text.bind(&myPreviewEnd);
	previewEnd->setEditable(false);
	previewEnd->setTooltip("The end time of the music preview");

	auto setPreview = myLayout.add<WgButton>();
	setPreview->onPress.bind(this, &DialogSongProperties::onSetPreview);
	setPreview->text.set("Set region");
	setPreview->setTooltip("Set the music preview to the selected region");

	auto playPreview = myLayout.add<WgButton>();
	playPreview->onPress.bind(this, &DialogSongProperties::onPlayPreview);
	playPreview->text.set("{g:play}");
	playPreview->setTooltip("Play the music preview");

	myLayout.row().col(72).col(96).col(16).col(96).col(122);

	mySpinMinBPM = myLayout.add<WgSpinner>("Disp. BPM");
	mySpinMinBPM->value.bind(&myDisplayBpmRange.min);
	mySpinMinBPM->onChange.bind(this, &DialogSongProperties::mySetDisplayBpm);
	mySpinMinBPM->setTooltip("The low value of the display BPM");

	mySpinMaxBPM = myLayout.add<WgSpinner>("to");
	mySpinMaxBPM->value.bind(&myDisplayBpmRange.max);
	mySpinMaxBPM->onChange.bind(this, &DialogSongProperties::mySetDisplayBpm);
	mySpinMaxBPM->setTooltip("The high value of the display BPM");

	myBpmTypeList = myLayout.add<WgCycleButton>();
	myBpmTypeList->addItem("Default");
	myBpmTypeList->addItem("Custom");
	myBpmTypeList->addItem("Random");
	myBpmTypeList->value.bind(&myDisplayBpmType);
	myBpmTypeList->onChange.bind(this, &DialogSongProperties::mySetDisplayBpm);
	myBpmTypeList->setTooltip("Determines how the BPM preview is displayed");
}

void DialogSongProperties::myUpdateWidgets()
{
	bool enableBpmType = gSimfile->isOpen();
	bool enableBpmRange = enableBpmType && (myDisplayBpmType == BPM_CUSTOM);

	myBpmTypeList->setEnabled(enableBpmType);
	mySpinMinBPM->setEnabled(enableBpmRange);
	mySpinMaxBPM->setEnabled(enableBpmRange);
}

// ================================================================================================
// Updating properties.

void DialogSongProperties::myUpdateProperties()
{
	if(gSimfile->isOpen())
	{
		for(auto w : myLayout) w->setEnabled(true);

		auto meta = gSimfile->get();

		// Update the property strings.
		myTitle = meta->title;
		mySubtitle = meta->subtitle;
		myArtist = meta->artist;
		myCredit = meta->credit;
		myMusic = meta->music;
		myBackground = meta->background;
		myBanner = meta->banner;
		myCdTitle = meta->cdTitle;

		// Update the preview start/end time.
		if(meta->previewStart == 0.0 && meta->previewLength == 0.0)
		{
			myPreviewStart.clear();
			myPreviewEnd.clear();
		}
		else
		{
			myPreviewStart = Str::formatTime(meta->previewStart);
			myPreviewEnd = Str::formatTime(meta->previewStart + meta->previewLength);
		}

		// Update the display BPM.
		myDisplayBpmType = gTempo->getDisplayBpmType();
		if(myDisplayBpmType == BPM_CUSTOM)
		{
			myDisplayBpmRange = gTempo->getDisplayBpmRange();
		}
		else if(myDisplayBpmType == BPM_RANDOM)
		{
			myDisplayBpmRange = {0, 0};
		}
		else if(myDisplayBpmType == BPM_ACTUAL)
		{
			myDisplayBpmRange = gTempo->getBpmRange();
		}
	}
	else
	{
		for(auto w : myLayout) w->setEnabled(false);

		myTitle.clear();
		mySubtitle.clear();
		myArtist.clear();
		myCredit.clear();
		myMusic.clear();
		myBackground.clear();
		myBanner.clear();
		myCdTitle.clear();

		myPreviewStart.clear();
		myPreviewEnd.clear();

		myDisplayBpmRange = {0, 0};
		myDisplayBpmType = BPM_ACTUAL;
	}
}

void DialogSongProperties::myUpdateBanner()
{
	myBannerWidget->tex = Texture();
	if(gSimfile->isOpen())
	{
		auto meta = gSimfile->get();
		String filename = meta->banner;
		if(filename.len())
		{
			String path = gSimfile->getDir() + filename;
			myBannerWidget->tex = Texture(path.str());
			if(myBannerWidget->tex.handle() == 0)
			{
				HudWarning("Could not open \"%s\".", filename.str());
			}
		}
	}
}

// ================================================================================================
// Other functions.

void DialogSongProperties::onChanges(int changes)
{
	if(changes & VCM_SONG_PROPERTIES_CHANGED)
	{
		myUpdateProperties();
		myUpdateWidgets();
	}
	if(changes & VCM_BANNER_PATH_CHANGED)
	{
		myUpdateBanner();
	}
}

void DialogSongProperties::onSetPreview()
{
	auto region = gSelection->getSelectedRegion();
	double start = gTempo->rowToTime(region.beginRow);
	double len = gTempo->rowToTime(region.endRow) - start;
	gMetadata->setMusicPreview(start, len);
}

void DialogSongProperties::onPlayPreview()
{
	if(gSimfile->isOpen())
	{
		auto meta = gSimfile->get();
		gView->setCursorTime(meta->previewStart);
		gMusic->play();
	}
}

void DialogSongProperties::onFindMusic()
{
	String path = gMetadata->findMusicFile();
	if(path.empty())
	{
		HudNote("Could not find any audio files...");
	}
	else
	{
		gMetadata->setMusicPath(path);
	}
}

void DialogSongProperties::onFindBanner()
{
	String path = gMetadata->findBannerFile();
	if(path.empty())
	{
		HudNote("Could not find any banner art...");
	}
	else
	{
		gMetadata->setBannerPath(path);
	}
}

void DialogSongProperties::onFindBG()
{
	String path = gMetadata->findBackgroundFile();
	if(path.empty())
	{
		HudNote("Could not find any background art...");
	}
	else
	{
		gMetadata->setBackgroundPath(path);
	}
}

void DialogSongProperties::onFindCdTitle()
{
	String path = gMetadata->findCdTitleFile();
	if (path.empty())
	{
		HudNote("Could not find any CD Title art...");
	}
	else
	{
		gMetadata->setCdTitlePath(path);
	}
}

void DialogSongProperties::mySetDisplayBpm()
{
	switch(myDisplayBpmType)
	{
	case BPM_ACTUAL:
		gTempo->setDefaultBpm(); break;
	case BPM_CUSTOM:
		gTempo->setCustomBpm(myDisplayBpmRange); break;
	case BPM_RANDOM:
		gTempo->setRandomBpm(); break;
	};
}

}; // namespace Vortex
