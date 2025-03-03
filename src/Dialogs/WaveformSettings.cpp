#include <Dialogs/WaveformSettings.h>

#include <Core/WidgetsLayout.h>

#include <Editor/Waveform.h>

#define Dlg DialogWaveformSettings

namespace Vortex {

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	myPresetIndex = 0;

	myColors = gWaveform->getColors();
	myLuminance = gWaveform->getLuminance();
	myWaveShape = gWaveform->getWaveShape();
	myAntiAliasing = gWaveform->getAntiAliasing();

	myFilterType = Waveform::FT_HIGH_PASS;
	myFilterStrength = 0.75f;

	setTitle("WAVEFORM SETTINGS");

	myLayout.row().col(96).col(128);

	// Color Scheme.
	WgCycleButton* preset = myLayout.add<WgCycleButton>("Presets");
	preset->onChange.bind(this, &Dlg::myApplyPreset);
	preset->value.bind(&myPresetIndex);
	preset->addItem("Vortex");
	preset->addItem("DDReam");
	preset->setTooltip("Preset styles for the waveform appearance");

	// BG color.
	WgColorPicker* bgColor = myLayout.add<WgColorPicker>("BG color");
	bgColor->red.bind(&myColors.bg.r);
	bgColor->green.bind(&myColors.bg.g);
	bgColor->blue.bind(&myColors.bg.b);
	bgColor->alpha.bind(&myColors.bg.a);
	bgColor->onChange.bind(this, &Dlg::myUpdateSettings);
	bgColor->setTooltip("Color of the waveform background");

	// FG color.
	WgColorPicker* waveColor = myLayout.add<WgColorPicker>("Wave color");
	waveColor->red.bind(&myColors.wave.r);
	waveColor->green.bind(&myColors.wave.g);
	waveColor->blue.bind(&myColors.wave.b);
	waveColor->alpha.bind(&myColors.wave.a);
	waveColor->onChange.bind(this, &Dlg::myUpdateSettings);
	waveColor->setTooltip("Color of the waveform");

	// Filter color
	WgColorPicker* filterColor = myLayout.add<WgColorPicker>("Filter color");
	filterColor->red.bind(&myColors.filter.r);
	filterColor->green.bind(&myColors.filter.g);
	filterColor->blue.bind(&myColors.filter.b);
	filterColor->alpha.bind(&myColors.filter.a);
	filterColor->onChange.bind(this, &Dlg::myUpdateSettings);
	filterColor->setTooltip("Color of the filtered waveform");

	// Luminance.
	WgCycleButton* lum = myLayout.add<WgCycleButton>("Luminance");
	lum->value.bind(&myLuminance);
	lum->onChange.bind(this, &Dlg::myUpdateSettings);
	lum->addItem("Uniform");
	lum->addItem("Amplitude");
	lum->setTooltip("Determines the lightness of the waveform peaks");

	// Wave shape.
	WgCycleButton* shape = myLayout.add<WgCycleButton>("Wave shape");
	shape->value.bind(&myWaveShape);
	shape->onChange.bind(this, &Dlg::myUpdateSettings);
	shape->addItem("Rectified");
	shape->addItem("Signed");
	shape->setTooltip("Determines the shape of the waveform peaks");

	// Anti-aliasing.
	WgCycleButton* aa = myLayout.add<WgCycleButton>("Anti-aliasing");
	aa->value.bind(&myAntiAliasing);
	aa->onChange.bind(this, &Dlg::myUpdateSettings);
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
	filter->value.bind(&myFilterType);
	filter->addItem("High-pass");
	filter->addItem("Low-pass");
	filter->setTooltip("Determines the shape of the waveform filter");

	// Filter strength.
	WgSlider* strength = myLayout.add<WgSlider>("Strength");
	strength->value.bind(&myFilterStrength);
	strength->setTooltip("The strength of the waveform filter");

	// Show both waveforms.
	myLayout.row().col(228);
	WgCheckbox* bothWaves = myLayout.add<WgCheckbox>();
	bothWaves->text.set("Overlay filtered waveform");
	bothWaves->value.bind(&myOverlayFilter);
	bothWaves->onChange.bind(this, &Dlg::myToggleOverlayFilter);
	bothWaves->setTooltip("If enabled, the filtered waveform is shown on top of the original waveform");

	// Filtering.
	myLayout.row().col(112).col(112);

	WgButton* disable = myLayout.add<WgButton>();
	disable->text.set("Disable filter");
	disable->onPress.bind(this, &Dlg::myDisableFilter);
	disable->setTooltip("Hides the filtered waveform");

	WgButton* enable = myLayout.add<WgButton>();
	enable->text.set("Apply filter");
	enable->onPress.bind(this, &Dlg::myEnableFilter);
	enable->setTooltip("Shows the filtered waveform");
}

void Dlg::myApplyPreset()
{
	gWaveform->setPreset((Waveform::Preset)myPresetIndex);
	myColors = gWaveform->getColors();
	myLuminance = gWaveform->getLuminance();
	myWaveShape = gWaveform->getWaveShape();
	myAntiAliasing = gWaveform->getAntiAliasing();
}

void Dlg::myUpdateSettings()
{
	gWaveform->setColors(myColors);
	gWaveform->setAntiAliasing(myAntiAliasing);
	gWaveform->setLuminance((Waveform::Luminance)myLuminance);
	gWaveform->setWaveShape((Waveform::WaveShape)myWaveShape);
}

void Dlg::myToggleOverlayFilter()
{
	gWaveform->overlayFilter(myOverlayFilter);
}

void Dlg::myEnableFilter()
{
	gWaveform->enableFilter((Waveform::FilterType)myFilterType, myFilterStrength);
}

void Dlg::myDisableFilter()
{
	gWaveform->disableFilter();
}	

}; // namespace Vortex