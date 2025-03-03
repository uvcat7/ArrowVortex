#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

#include <Vortex/Notefield/Waveform.h>

namespace AV {

class DialogWaveformSettings : public EditorDialog
{
public:
	~DialogWaveformSettings();
	DialogWaveformSettings();

private:
	void myInitTabs();
	void myInitMainTab();
	void myInitFilterTab();

	struct Form;
	Form* myForm;

	void myApplyPreset();
	void myUpdateSettings();
	void myEnableFilter();
	void myDisableFilter();
	void myToggleOverlayFilter();
	void myUpdateFromWaveform();
};

} // namespace AV
