#pragma once

#include <Vortex/View/EditorDialog.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

class DialogAdjustTempo : public EditorDialog
{
public:
	~DialogAdjustTempo();
	DialogAdjustTempo();

	void clear();

	void tick(bool enabled) override;
	void onAction(int id);

private:
	void myInitTabs();
	void myInitMainTab();
	void myInitSm5Tab();
	void myInitOffsetTab();
	void myHandleActiveTempoChanged();

	struct Form;
	Form* myForm;

	double myBeatsToInsert = 0;
	int myInsertTarget = 0;
};

} // namespace AV
