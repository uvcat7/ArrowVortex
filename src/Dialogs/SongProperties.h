#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>
#include <Core/Draw.h>

#include <Simfile/Tempo.h>

namespace Vortex {

class DialogSongProperties : public EditorDialog
{
public:
	~DialogSongProperties();
	DialogSongProperties();

	void onChanges(int changes) override;

	void onSetPreview();
	void onPlayPreview();

	void onFindMusic();
	void onFindBanner();
	void onFindBG();
	void onFindCdTitle();

private:
	struct BannerWidget;

	void myCreateWidgets();
	void myUpdateWidgets();

	void myUpdateProperties();
	void myUpdateBanner();

	void mySetProperty(int p);
	void mySetDisplayBpm();

	String myTitle;
	String mySubtitle;
	String myArtist;
	String myCredit;
	String myMusic;
	String myBackground;
	String myBanner;
	String myCdTitle;
	
	int myDisplayBpmType;
	BpmRange myDisplayBpmRange;

	BannerWidget* myBannerWidget;
	WgCycleButton* myBpmTypeList;
	WgSpinner* mySpinMinBPM, *mySpinMaxBPM;
	String myPreviewStart, myPreviewEnd;
};

}; // namespace Vortex
