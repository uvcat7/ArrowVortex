#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WBreakdownList : public Widget
{
public:
	~WBreakdownList();
	WBreakdownList();

	void update(const Chart* chart, bool simplify);

	std::string convertToText();

	void onMeasure() override;
	void onArrange(Rect r) override;
	void draw(bool enabled) override;

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	int getTotalMeasureCount() const;

	int getItemAtPos(Vec2 pos);

private:
	struct Item { int x, y, w; int startRow, endRow; std::string text; };
	vector<Item> myItems;
	int myTotalMeasures;
	int myPressedButton;
};

} // namespace AV
