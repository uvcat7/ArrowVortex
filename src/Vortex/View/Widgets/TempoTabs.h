#pragma once

#include <Core/Graphics/Draw.h>

#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

class WTempoTabs : public WVerticalTabs
{
public:
	struct Graphics { TileRect flat, color, hover, press; };

	WTempoTabs();
	~WTempoTabs();

	void updateList();

private:
	class ButtonList;
	// shared_ptr<ButtonList> myButtonLists[(int)SegmentType::COUNT];
};

} // namespace AV
