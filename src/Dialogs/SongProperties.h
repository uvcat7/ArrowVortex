#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>
#include <Core/Draw.h>

#include <Simfile/Tempo.h>

namespace Vortex {

class DialogSongProperties : public EditorDialog {
   public:
    ~DialogSongProperties();
    DialogSongProperties();

    void onChanges(int changes) override;

    void onSetPreview();
    void onPlayPreview();

    void onFindMusic(bool open);
    void onFindBanner(bool open);
    void onFindBG(bool open);
    void onFindCdTitle(bool open);

   private:
    struct BannerWidget;
    struct CdTitleWidget;

    void myCreateWidgets();
    void myUpdateWidgets();

    void myUpdateProperties();
    void myUpdateBanner();
    void myUpdateCdTitle();

    void mySetProperty(int p);
    void mySetDisplayBpm();

    std::vector<Texture> extractSpriteSheet(const std::string& path,
                                            const std::string& filename);

    std::string fileDlgPath(const std::string& title);

    std::string myTitle;
    std::string mySubtitle;
    std::string myArtist;
    std::string myCredit;
    std::string myMusic;
    std::string myBackground;
    std::string myBanner;
    std::string myCdTitle;

    int myDisplayBpmType;
    BpmRange myDisplayBpmRange;

    BannerWidget* myBannerWidget;
    CdTitleWidget* myCdTitleWidget;
    WgCycleButton* myBpmTypeList;
    WgSpinner *mySpinMinBPM, *mySpinMaxBPM;
    std::string myPreviewStart, myPreviewEnd;
};

};  // namespace Vortex
