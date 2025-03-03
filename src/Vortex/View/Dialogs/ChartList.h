#pragma once

#include <Core/Graphics/Draw.h>

#include <Vortex/View/EditorDialog.h>

#include <Vortex/View/Widgets/ChartTabs.h>

namespace AV {

class DialogChartList : public EditorDialog
{
public:
	~DialogChartList();
	DialogChartList();

	void updateList();

private:
	void myHandleChartsChanged();

	shared_ptr<WChartTabs> myTabs;
	EventSubscriptions mySubscriptions;
};

} // namespace AV
