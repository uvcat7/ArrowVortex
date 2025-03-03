#pragma once

#include <Core/Graphics/Draw.h>

#include <Vortex/View/EditorDialog.h>

#include <Vortex/View/Widgets/TempoTabs.h>

namespace AV {

class DialogTempoBreakdown : public EditorDialog
{
public:
	~DialogTempoBreakdown();
	DialogTempoBreakdown();

	void updateList();

private:
	void myHandleSegmentsChanged();

	shared_ptr<WTempoTabs> myTabs;
	EventSubscriptions mySubscriptions;
};

} // namespace AV
