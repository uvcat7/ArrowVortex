#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/Waveform.h>

namespace Vortex {

class DialogWaveformSettings : public EditorDialog {
   public:
    ~DialogWaveformSettings();
    DialogWaveformSettings();

   private:
    void myApplyPreset();
    void myUpdateSettings();
    void myEnableFilter();
    void myDisableFilter();
    void myToggleOverlayFilter();

    Waveform::ColorScheme settingsColorScheme_;
    int presetIndex_;
    int luminanceValue_;
    int waveShape_;
    int antiAliasingMode_;

    int filterType_;
    float filterStrength_;
    bool isOverlayFilterActive_;
};

};  // namespace Vortex
