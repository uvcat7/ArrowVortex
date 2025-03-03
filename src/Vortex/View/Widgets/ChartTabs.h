#pragma once

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

class WChartTabs : public WVerticalTabs
{
public:
	struct Graphics { TileRect flat, color, hover, press; };

	WChartTabs();
	~WChartTabs();

	void updateList();

private:
	class ButtonList;
	Graphics myGraphics;
	vector<shared_ptr<ButtonList>> myButtonLists;
};

} // namespace AV
