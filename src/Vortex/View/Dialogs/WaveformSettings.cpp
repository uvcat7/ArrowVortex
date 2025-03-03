#include <Precomp.h>

#include <Core/Interface/Widgets/Slider.h>
#include <Core/Interface/Widgets/Checkbox.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/ColorPicker.h>
#include <Core/Interface/Widgets/CycleButton.h>
#include <Core/Interface/Widgets/Label.h>

#include <Vortex/Settings.h>
#include <Vortex/Notefield/Waveform.h>
#include <Vortex/View/Dialogs/WaveformSettings.h>

#define Dlg DialogWaveformSettings

namespace AV {

using namespace std;

struct Dlg::Form
{
	shared_ptr<WVerticalTabs> tabs;

	shared_ptr<WColorPicker> bgColor;
	shared_ptr<WColorPicker> waveColor;
	shared_ptr<WColorPicker> filterColor;

	shared_ptr<WCycleButton> presetIndex;
	shared_ptr<WCycleButton> luminance;
	shared_ptr<WCycleButton> waveShape;
	shared_ptr<WCycleButton> antiAliasing;

	shared_ptr<WCycleButton> filterType;
	shared_ptr<WSlider> filterStrength;
	shared_ptr<WCheckbox> overlayFilter;
};

// =====================================================================================================================
// Constructor and destructor.

Dlg::~Dlg()
{
	delete myForm;
}

Dlg::Dlg()
{
	setTitle("WAVEFORM SETTINGS");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	myInitTabs();
	myUpdateFromWaveform();
	
	myForm->filterType->setSelectedItem((int)Waveform::FilterType::HighPass);
	myForm->filterStrength->setValue(0.75f);
	myForm->overlayFilter->setValue(0);
}

// =====================================================================================================================
// Tabs construction.

void Dlg::myInitTabs()
{
	myForm = new Form();
	myContent = myForm->tabs = make_shared<WVerticalTabs>();

	myInitMainTab();
	myInitFilterTab();
}

void Dlg::myInitMainTab()
{
	auto presetsLabel = make_shared<WLabel>("Presets");
	auto presets = make_shared<WCycleButton>();
	presets->onChange = bind(&Dlg::myApplyPreset, this);
	presets->addItem("Vortex");
	presets->addItem("DDReam");
	presets->setTooltip("Preset styles for the waveform appearance");
	myForm->presetIndex = presets;

	auto bgColorLabel = make_shared<WLabel>("BG color");
	auto bgColor = make_shared<WColorPicker>();
	bgColor->onChange = bind(&Dlg::myUpdateSettings, this);
	bgColor->setTooltip("Color of the waveform background");
	myForm->bgColor = bgColor;

	auto waveColorLabel = make_shared<WLabel>("Wave color");
	auto waveColor = make_shared<WColorPicker>();
	waveColor->onChange = bind(&Dlg::myUpdateSettings, this);
	waveColor->setTooltip("Color of the waveform");
	myForm->waveColor = waveColor;

	auto filterColorLabel = make_shared<WLabel>("Filter color");
	auto filterColor = make_shared<WColorPicker>();
	filterColor->onChange = bind(&Dlg::myUpdateSettings, this);
	filterColor->setTooltip("Color of the filtered waveform");
	myForm->filterColor = filterColor;

	auto lumLabel = make_shared<WLabel>("Luminance");
	auto lum = make_shared<WCycleButton>();
	lum->onChange = bind(&Dlg::myUpdateSettings, this);
	lum->addItem("Uniform");
	lum->addItem("Amplitude");
	lum->setTooltip("Determines the lightness of the waveform peaks");
	myForm->luminance = lum;

	auto shapeLabel = make_shared<WLabel>("Wave shape");
	auto shape = make_shared<WCycleButton>();
	shape->onChange = bind(&Dlg::myUpdateSettings, this);
	shape->addItem("Rectified");
	shape->addItem("Signed");
	shape->setTooltip("Determines the shape of the waveform peaks");
	myForm->waveShape = shape;

	auto aaLabel = make_shared<WLabel>("Anti-aliasing");
	auto aa = make_shared<WCycleButton>();
	aa->onChange = bind(&Dlg::myUpdateSettings, this);
	aa->addItem("None");
	aa->addItem("2x");
	aa->addItem("3x");
	aa->addItem("4x");
	aa->setTooltip("Determines the smoothness of the waveform shape");
	myForm->antiAliasing = aa;

	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);

	grid->addColumn(92, 4);
	grid->addColumn(100, 0, 1);

	grid->addWidget(presetsLabel, 0, 0);
	grid->addWidget(presets, 0, 1);
	grid->addWidget(bgColorLabel, 1, 0);
	grid->addWidget(bgColor, 1, 1);
	grid->addWidget(waveColorLabel, 2, 0);
	grid->addWidget(waveColor, 2, 1);
	grid->addWidget(filterColorLabel, 3, 0);
	grid->addWidget(filterColor, 3, 1);
	grid->addWidget(lumLabel, 4, 0);
	grid->addWidget(lum, 4, 1);
	grid->addWidget(shapeLabel, 5, 0);
	grid->addWidget(shape, 5, 1);
	grid->addWidget(aaLabel, 6, 0);
	grid->addWidget(aa, 6, 1);

	myForm->tabs->addTab(grid);
}

void Dlg::myInitFilterTab()
{
	auto typeLabel = make_shared<WLabel>("Filter type");
	auto type = make_shared<WCycleButton>();
	type->addItem("High-pass");
	type->addItem("Low-pass");
	type->setTooltip("Determines the shape of the waveform filter");
	myForm->filterType = type;

	auto strengthLabel = make_shared<WLabel>("Strength");
	auto strength = make_shared<WSlider>();
	strength->setTooltip("The strength of the waveform filter");
	myForm->filterStrength = strength;

	auto overlay = make_shared<WCheckbox>("Overlay filtered waveform");
	overlay->onChange = bind(&Dlg::myToggleOverlayFilter, this);
	overlay->setTooltip("If enabled, the filtered waveform is shown on top of the original waveform");
	myForm->overlayFilter = overlay;

	auto disable = make_shared<WButton>("Disable filter");
	disable->onPress = bind(&Dlg::myDisableFilter, this);
	disable->setTooltip("Hides the filtered waveform");

	auto apply = make_shared<WButton>("Apply filter");
	apply->onPress = bind(&Dlg::myEnableFilter, this);
	apply->setTooltip("Shows the filtered waveform");

	auto grid = make_shared<WGrid>();

	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 4);
	grid->addRow(24, 0);

	grid->addColumn(92, 4);
	grid->addColumn(0, 4, 1);
	grid->addColumn(96, 0, 1);

	grid->addWidget(typeLabel, 0, 0);
	grid->addWidget(type, 0, 1, 1, 2);
	grid->addWidget(strengthLabel, 1, 0);
	grid->addWidget(strength, 1, 1, 1, 2);
	grid->addWidget(overlay, 2, 0, 1, 3);
	grid->addWidget(disable, 3, 0, 1, 2);
	grid->addWidget(apply, 3, 2);

	myForm->tabs->addTab(grid, "Waveform Filter");
}

// =====================================================================================================================
// Widget functions.

void Dlg::myApplyPreset()
{
	Waveform::setPreset((Waveform::Preset)myForm->presetIndex->selectedItem());
	myUpdateFromWaveform();
}

void Dlg::myUpdateFromWaveform()
{
	auto& settings = Settings::waveform();

	myForm->bgColor->setValue(settings.backgroundColor);
	myForm->filterColor->setValue(settings.filterColor);
	myForm->waveColor->setValue(settings.waveColor);

	myForm->luminance->setSelectedItem((int)settings.luminance.value());
	myForm->waveShape->setSelectedItem((int)settings.waveShape.value());
	myForm->antiAliasing->setSelectedItem(settings.antiAliasing);
}

void Dlg::myUpdateSettings()
{
	auto& settings = Settings::waveform();

	settings.backgroundColor.set(myForm->bgColor->value());
	settings.filterColor.set(myForm->filterColor->value());
	settings.waveColor.set(myForm->waveColor->value());
	settings.antiAliasing.set(myForm->antiAliasing->selectedItem());

	auto luminance = myForm->luminance->selectedItem();
	settings.luminance.set((Waveform::Luminance)luminance);

	auto waveShape = myForm->waveShape->selectedItem();
	settings.waveShape.set((Waveform::WaveShape)waveShape);
}

void Dlg::myToggleOverlayFilter()
{
	Waveform::overlayFilter(myForm->overlayFilter->value());
}

void Dlg::myEnableFilter()
{
	auto filterType = (Waveform::FilterType)myForm->filterType->selectedItem();
	auto filterStrength = myForm->filterStrength->value();
	Waveform::enableFilter(filterType, filterStrength);
}

void Dlg::myDisableFilter()
{
	Waveform::disableFilter();
}

} // namespace AV
