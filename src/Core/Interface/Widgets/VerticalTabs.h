#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WVerticalTabs : public Widget
{
public:
	struct TabImpl;

	struct Tab
	{
		std::function<void()> onClose;
		void setVisible(bool state);
		void setExpanded(bool state);
		void setStretching(bool state);
		void setCollapsable(bool state);
		void setDescription(std::string text);
	};

	~WVerticalTabs();
	WVerticalTabs();

	void onMousePress(MousePress& input) override;

	void onMeasure() override;
	void onArrange(Rect r) override;

	UiElement* findMouseOver(int x, int y) override;
	void handleEvent(UiEvent& uiEvent) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void setMargin(int margin);

	// NOTE: tab takes ownership of the widget.
	Tab* addTab(shared_ptr<Widget> widget, const char* description = nullptr,  bool expanded = true);

	Tab* getTab(int index);
	int getNumTabs() const;

protected:
	int myMargin;
	vector<TabImpl*> myTabs;
};

} // namespace AV
