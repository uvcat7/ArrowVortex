#pragma once

#include <Dialogs/Dialog.h>

#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Editor/Waveform.h>

namespace Vortex {

class DialogWaveformSettings : public EditorDialog
{
public:
	~DialogWaveformSettings();
	DialogWaveformSettings();

private:
	void myApplyPreset();
	void myUpdateSettings();
	void myEnableFilter();
	void myDisableFilter();
	void myToggleOverlayFilter();

	Waveform::ColorScheme myColors;
	int myPresetIndex;
	int myLuminance;
	int myWaveShape;
	int myAntiAliasing;

	int myFilterType;
	float myFilterStrength;
	bool myOverlayFilter;
};

}; // namespace Vortex
