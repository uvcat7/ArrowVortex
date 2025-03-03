#pragma once

#include <Core/Interface/Widgets/SelectList.h>

namespace AV {

class WDroplist : public Widget
{
public:
	~WDroplist();
	WDroplist();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;
	void onMouseScroll(MouseScroll& input) override;

	void onMeasure() override;
	void onArrange(Rect r) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void addItem(stringref text);
	void clearItems();

	void setSelectedItem(int index);
	int selectedItem() const;

	std::function<void()> onChange;

protected:
	void myCloseList();

	shared_ptr<WSelectList> myList;
	vector<std::string> myItems;
	int mySelectedItem;
};

} // namespace AV
