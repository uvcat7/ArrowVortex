#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

/*class WScrollRegion : public Widget
{
public:
	enum class ScrollType { SHOW_ALWAYS, SHOW_WHEN_NEEDED, SHOW_NEVER };

	~WScrollRegion();
	WScrollRegion();

	void onHandleInputs(InputEventList& events) override;
	void onMouseScroll(MouseScroll& input) override;
	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void onMeasure() override;
	void onArrange(Rect r) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;
	void onApply(WidgetAction action) override;

	void setMargin(int margin);
	void setChild(shared_ptr<Widget> child);
	void setScrollType(ScrollType h, ScrollType v);

	int getViewportW() const;
	int getViewportH() const;
	Size2 getInnerSize() const;

protected:
	void myClampScrollValues();

	shared_ptr<Widget> myChild;

	uint myScrollTypeH : 2;
	uint myScrollTypeV : 2;
	uint myScrollbarActiveH : 1;
	uint myScrollbarActiveV : 1;
	uint myScrollAction : 9;
	uint myScrollGrabPos : 16;

	int myMargin;
	int myContentWidth;
	int myContentHeight;
	int myScrollX;
	int myScrollY;

private:
	uint myGetActionAt(Vec2 pos);
};*/

} // namespace AV
