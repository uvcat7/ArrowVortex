#pragma once

#include <Core/GuiContext.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

enum DialogId {
    DIALOG_ADJUST_SYNC,
    DIALOG_ADJUST_TEMPO,
    DIALOG_ADJUST_TEMPO_SM5,
    DIALOG_CHART_LIST,
    DIALOG_CHART_PROPERTIES,
    DIALOG_DANCING_BOT,
    DIALOG_GENERATE_NOTES,
    DIALOG_NEW_CHART,
    DIALOG_SONG_PROPERTIES,
    DIALOG_TEMPO_BREAKDOWN,
    DIALOG_LABEL_BREAKDOWN,
    DIALOG_WAVEFORM_SETTINGS,
    DIALOG_ZOOM,
    DIALOG_CUSTOM_SNAP,
    DIALOG_PREVIEW_SETTINGS,
    NUM_DIALOG_IDS
};

class EditorDialog : public GuiDialog {
   public:
    ~EditorDialog();
    EditorDialog();

    void onUpdateSize() override;
    void onTick() override;
    void onDraw() override;

    void setId(DialogId id);

    virtual void onChanges(int changes) {}

    static DialogId getId(const char* name);
    static const char* getName(DialogId id);

   protected:
    DialogId myId;
    RowLayout myLayout;
};

};  // namespace Vortex
