#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WCycleButton : public Widget
{
public:
	~WCycleButton();
	WCycleButton();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void draw(bool enabled) override;

	void addItem(stringref text);
	void clearItems();

	void setSelectedItem(int item);
	int selectedItem() const;

	std::function<void()> onChange;

protected:
	vector<std::string> myItems;
	int mySelectedItem;
};

} // namespace AV
