#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

class DialogNewChart : public EditorDialog
{
public:
	~DialogNewChart();
	DialogNewChart();

private:
	void myInitTabs();
	void myInitMainTab();
	void myInitMetadataTab();
	void myInitCreateTab();

	struct Form;
	Form* myForm;

	void myCreateChart();
	void myUpdateModes();
};

} // namespace AV
