#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

#include <Audio/FindTempo.h>

namespace AV {

class DialogAdjustSync : public EditorDialog
{
public:
	~DialogAdjustSync();
	DialogAdjustSync();

	void tick(bool enabled) override;

	void onAction(int id);	
	void onApplyTempo();
	void onFindTempo();

private:
	void myInitTabs();
	void myInitMainTab();
	void myInitTempoDetectionTab();
	void myResetTempoDetection();
	void myHandleTimingChanged();

	struct Form;
	Form* myForm;

	TempoDetector* myTempoDetector = nullptr;
	vector<TempoResult> myDetectionResults;
	int myDetectionRow = 0;
};

} // namespace AV
