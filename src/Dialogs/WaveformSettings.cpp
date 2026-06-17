#include <Dialogs/WaveformSettings.h>

#include <Core/WidgetsLayout.h>

#include <Editor/Waveform.h>

namespace Vortex {

DialogWaveformSettings::~DialogWaveformSettings() = default;

DialogWaveformSettings::DialogWaveformSettings() {
    presetIndex_ = 0;

    settingsColorScheme_ = gWaveform->getColors();
    luminanceValue_ = gWaveform->getLuminance();
    waveShape_ = gWaveform->getWaveShape();
    antiAliasingMode_ = gWaveform->getAntiAliasing();
    isOverlayFilterActive_ = gWaveform->getOverlayFilter();

    filterType_ = Waveform::FT_HIGH_PASS;
    filterStrength_ = 0.75f;

    setTitle("WAVEFORM SETTINGS");

    myLayout.row().col(96).col(128);

    // Color Scheme.
    WgCycleButton* preset = myLayout.add<WgCycleButton>("Presets");
    preset->onChange.bind(this, &DialogWaveformSettings::myApplyPreset);
    preset->value.bind(&presetIndex_);
    preset->addItem("Vortex");
    preset->addItem("DDReam");
    preset->setTooltip("Preset styles for the waveform appearance");

    // BG color.
    WgColorPicker* bgColor = myLayout.add<WgColorPicker>("BG color");
    bgColor->red.bind(&settingsColorScheme_.bg.r);
    bgColor->green.bind(&settingsColorScheme_.bg.g);
    bgColor->blue.bind(&settingsColorScheme_.bg.b);
    bgColor->alpha.bind(&settingsColorScheme_.bg.a);
    bgColor->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    bgColor->setTooltip("Color of the waveform background");

    // FG color.
    WgColorPicker* waveColor = myLayout.add<WgColorPicker>("Wave color");
    waveColor->red.bind(&settingsColorScheme_.wave.r);
    waveColor->green.bind(&settingsColorScheme_.wave.g);
    waveColor->blue.bind(&settingsColorScheme_.wave.b);
    waveColor->alpha.bind(&settingsColorScheme_.wave.a);
    waveColor->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    waveColor->setTooltip("Color of the waveform");

    // Filter color
    WgColorPicker* filterColor = myLayout.add<WgColorPicker>("Filter color");
    filterColor->red.bind(&settingsColorScheme_.filter.r);
    filterColor->green.bind(&settingsColorScheme_.filter.g);
    filterColor->blue.bind(&settingsColorScheme_.filter.b);
    filterColor->alpha.bind(&settingsColorScheme_.filter.a);
    filterColor->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    filterColor->setTooltip("Color of the filtered waveform");

    // Luminance.
    WgCycleButton* lum = myLayout.add<WgCycleButton>("Luminance");
    lum->value.bind(&luminanceValue_);
    lum->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    lum->addItem("Uniform");
    lum->addItem("Amplitude");
    lum->setTooltip("Determines the lightness of the waveform peaks");

    // Wave shape.
    WgCycleButton* shape = myLayout.add<WgCycleButton>("Wave shape");
    shape->value.bind(&waveShape_);
    shape->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    shape->addItem("Rectified");
    shape->addItem("Signed");
    shape->setTooltip("Determines the shape of the waveform peaks");

    // Anti-aliasing.
    WgCycleButton* aa = myLayout.add<WgCycleButton>("Anti-aliasing");
    aa->value.bind(&antiAliasingMode_);
    aa->onChange.bind(this, &DialogWaveformSettings::myUpdateSettings);
    aa->addItem("None");
    aa->addItem("2x");
    aa->addItem("3x");
    aa->addItem("4x");
    aa->setTooltip("Determines the smoothness of the waveform shape");

    // Filter type.
    myLayout.row().col(228);
    myLayout.add<WgSeperator>();
    myLayout.row().col(96).col(128);

    WgCycleButton* filter = myLayout.add<WgCycleButton>("Filter type");
    filter->value.bind(&filterType_);
    filter->addItem("High-pass");
    filter->addItem("Low-pass");
    filter->setTooltip("Determines the shape of the waveform filter");

    // Filter strength.
    WgSlider* strength = myLayout.add<WgSlider>("Strength");
    strength->value.bind(&filterStrength_);
    strength->setTooltip("The strength of the waveform filter");

    // Show both waveforms.
    myLayout.row().col(228);
    WgCheckbox* bothWaves = myLayout.add<WgCheckbox>();
    bothWaves->text.set("Overlay filtered waveform");
    bothWaves->value.bind(&isOverlayFilterActive_);
    bothWaves->onChange.bind(this,
                             &DialogWaveformSettings::myToggleOverlayFilter);
    bothWaves->setTooltip(
        "If enabled, the filtered waveform is shown on top of the original "
        "waveform");

    // Filtering.
    myLayout.row().col(112).col(112);

    WgButton* disable = myLayout.add<WgButton>();
    disable->text.set("Disable filter");
    disable->onPress.bind(this, &DialogWaveformSettings::myDisableFilter);
    disable->setTooltip("Hides the filtered waveform");

    WgButton* enable = myLayout.add<WgButton>();
    enable->text.set("Apply filter");
    enable->onPress.bind(this, &DialogWaveformSettings::myEnableFilter);
    enable->setTooltip("Shows the filtered waveform");
}

void DialogWaveformSettings::myApplyPreset() {
    gWaveform->setPreset(static_cast<Waveform::Preset>(presetIndex_));
    settingsColorScheme_ = gWaveform->getColors();
    luminanceValue_ = gWaveform->getLuminance();
    waveShape_ = gWaveform->getWaveShape();
    antiAliasingMode_ = gWaveform->getAntiAliasing();
}

void DialogWaveformSettings::myUpdateSettings() {
    gWaveform->setColors(settingsColorScheme_);
    gWaveform->setAntiAliasing(antiAliasingMode_);
    gWaveform->setLuminance(static_cast<Waveform::Luminance>(luminanceValue_));
    gWaveform->setWaveShape(static_cast<Waveform::WaveShape>(waveShape_));
}

void DialogWaveformSettings::myToggleOverlayFilter() {
    gWaveform->setOverlayFilter(isOverlayFilterActive_);
}

void DialogWaveformSettings::myEnableFilter() {
    gWaveform->enableFilter(static_cast<Waveform::FilterType>(filterType_),
                            filterStrength_);
}

void DialogWaveformSettings::myDisableFilter() { gWaveform->disableFilter(); }

};  // namespace Vortex