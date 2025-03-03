#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WSelectList : public Widget
{
public:
	~WSelectList();
	WSelectList();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;
	void onMouseScroll(MouseScroll& input) override;

	void onMeasure() override;
	void onArrange(Rect r) override;
	void draw(bool enabled) override;

	void hideBackground();
	void addItem(stringref text);
	void clearItems();

	void scroll(bool isUp);
	bool wasInteracted() const;

	void setSelectedItem(uint index);
	int selectedItem() const;

	std::function<void()> onChange;

protected:
	int myHoverItem(Vec2 pos);
	bool myHasScrollbar() const;
	Rect myItemsRect() const;

	vector<std::string> myItems;
	int mySelectedItem;
	int myScrollPos;
	uint myShowBackground : 1;
	uint myWasInteracted : 1;
};

} // namespace AV
