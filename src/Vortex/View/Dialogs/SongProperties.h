#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

#include <Simfile/Tempo.h>

#include <Vortex/View/Widgets/BannerImage.h>

namespace AV {

class DialogSongProperties : public EditorDialog
{
public:
	~DialogSongProperties();
	DialogSongProperties();

	void onSetPreview();
	void onPlayPreview();

	void onFindMusic();
	void onFindBanner();
	void onFindBG();

private:
	void myInitTabs();
	void myInitBannerTab();
	void myInitMetadataTab();
	void myInitTransliterationTab();
	void myInitFilesTab();
	void myInitPreviewTab();

	void myUpdateActiveChart();
	void myUpdateWidgets();

	void myUpdateProperties();
	void myUpdateBanner();

	void mySetMusicPreview();
	void myReadMusicPreview();

	void mySetDisplayBpm();
	void myReadDisplayBpm();

	struct Form;
	Form* myForm;
};

} // namespace AV
