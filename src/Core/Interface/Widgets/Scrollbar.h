#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

/*
class WScrollbar : public Widget
{
public:
	~WScrollbar();
	WScrollbar();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void setEnd(int size);
	void setPage(int size);

	IntBinding value;
	ActionBinding onChange;

	virtual bool isVertical() const = 0;

protected:
	void myUpdateValue(int v);
	int myEnd, myPage;
	uint myScrollAction : 9;
	uint myScrollGrabPos : 16;

private:
	uint myGetActionAt(int x, int y);
};

class WScrollbarV : public WScrollbar
{
public:
	WScrollbarV();
	bool isVertical() const;
};

class WScrollbarH : public WScrollbar
{
public:
	WScrollbarH();
	bool isVertical() const;
};*/

} // namespace AV
