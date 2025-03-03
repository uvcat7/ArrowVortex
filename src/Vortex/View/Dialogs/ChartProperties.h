#pragma once

#include <Vortex/View/EditorDialog.h>
#include <Vortex/View/Widgets/BreakdownList.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

class DialogChartProperties : public EditorDialog
{
public:
	~DialogChartProperties();
	DialogChartProperties();

private:
	void myInitTabs();
	void myInitMainTab();
	void myInitMetadataTab();
	void myInitNoteInformationTab();
	void myInitStreamBreakdownTab();
	
	void mySetDifficulty();
	void mySetMeter();
	void myCalcMeter();

	void mySetChartName();
	void mySetChartStyle();
	void mySetDescription();
	void mySetStepAuthor();

	void myUpdateChart();
	void myUpdateNoteInfo();
	void myUpdateBreakdown();
	void myHandleChartMetaChanged();

	struct Form;
	Form* myForm;
};

} // namespace AV
