#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

#include <stdint.h>

namespace Vortex {
namespace {

enum ScrollActions
{
	ACT_NONE,

	ACT_HOVER_BUTTON_H,
	ACT_HOVER_BUTTON_V,

	ACT_HOVER_BUMP_UP,
	ACT_HOVER_BUMP_DOWN,
	ACT_HOVER_BUMP_LEFT,
	ACT_HOVER_BUMP_RIGHT,

	ACT_DRAGGING_H,
	ACT_DRAGGING_V,
};

// ================================================================================================
// Helper functions.

struct ScrollButtonData { int pos, size; };

static ScrollButtonData GetButton(int size, int end, int page, int scroll)
{
	ScrollButtonData out = {0, size};
	if(end > page)
	{
		out.size = (int)(0.5 + size * (double)page / (double)end);
		out.size = max(16, out.size);

		out.pos = (int)(0.5 + scroll * (double)(size - out.size) / (double)(end - page));
		out.pos = max(0, min(out.pos, size - out.size));
	}
	return out;
}

static int GetScroll(int size, int end, int page, int pos)
{
	int out = 0;
	if(end > page)
	{
		ScrollButtonData button = GetButton(size, end, page, 0);
		out = (int)(0.5 + pos * (double)(end - page) / (double)(size - button.size));
		out = max(0, min(out, end - page));
	}
	return out;
}

static void DrawScrollbar(recti bar, ScrollButtonData button, bool vertical, bool hover, bool focus)
{
	auto& scrollbar = GuiDraw::getScrollbar();
	scrollbar.bar.draw(bar);
	recti buttonRect;
	if(vertical)
	{
		buttonRect = {bar.x, bar.y + button.pos, bar.w, button.size};
		scrollbar.base.draw(buttonRect);
		if(buttonRect.h >= 16)
		{
			vec2i grabPos = {bar.x + bar.w / 2, buttonRect.y + buttonRect.h / 2};
			Draw::sprite(scrollbar.grab, grabPos, Draw::ROT_90);
		}
	}
	else
	{
		buttonRect = {bar.x + button.pos, bar.y, button.size, bar.h};
		scrollbar.base.draw(buttonRect);
		if(buttonRect.w >= 16)
		{
			vec2i grabPos = {buttonRect.x + buttonRect.w / 2, bar.y + bar.h / 2};
			Draw::sprite(scrollbar.grab, grabPos, 0);
		}
	}
	if(focus)
	{
		scrollbar.pressed.draw(buttonRect);
	}
	else if(hover)
	{
		scrollbar.hover.draw(buttonRect);
	}
}

void ApplyScrollOffset(int& offset, int page, int scroll, bool up)
{
	int delta = max(1, page / 8);
	if(up) delta = -delta;
	offset = max(0, min(offset + delta, scroll - page));
}

}; // anonymous namespace.

// ================================================================================================
// WgScrollbar

WgScrollbar::~WgScrollbar()
{
}

WgScrollbar::WgScrollbar(GuiContext* gui)
	: GuiWidget(gui)
	, myScrollAction(0)
	, myScrollGrabPos(0)
{
	myEnd = 1;
	myPage = 1;
}

void WgScrollbar::setEnd(int size)
{
	myEnd = size;
}

void WgScrollbar::setPage(int size)
{
	myPage = size;
}

void WgScrollbar::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			uint action = myGetActionAt(evt.x, evt.y);
			if(action)
			{
				startCapturingMouse();
			}
			if(action == ACT_HOVER_BUTTON_H)
			{
				auto button = GetButton(myRect.w, myEnd, myPage, value.get());
				myScrollGrabPos = evt.x - button.pos - myRect.x;
				myScrollAction = ACT_DRAGGING_H;
			}
			if(action == ACT_HOVER_BUTTON_V)
			{
				auto button = GetButton(myRect.h, myEnd, myPage, value.get());
				myScrollGrabPos = evt.y - button.pos - myRect.y;
				myScrollAction = ACT_DRAGGING_V;
			}
		}
		evt.setHandled();
	}
}

void WgScrollbar::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
	myScrollAction = 0;
}

void WgScrollbar::onTick()
{
	if(myScrollAction == ACT_DRAGGING_H)
	{
		int buttonPos = myGui->getMousePos().x - myRect.x - myScrollGrabPos;
		myUpdateValue(GetScroll(myRect.w, myEnd, myPage, buttonPos));
	}
	else if(myScrollAction == ACT_DRAGGING_V)
	{
		int buttonPos = myGui->getMousePos().y - myRect.y - myScrollGrabPos;
		myUpdateValue(GetScroll(myRect.h, myEnd, myPage, buttonPos));
	}
	GuiWidget::onTick();
}

void WgScrollbar::onDraw()
{
	vec2i mpos = myGui->getMousePos();
	uint action = myGetActionAt(mpos.x, mpos.y);

	if(isVertical())
	{
		auto button = GetButton(myRect.h, myEnd, myPage, value.get());
		DrawScrollbar(myRect, button, true, action == ACT_HOVER_BUTTON_V, action == ACT_DRAGGING_V);
	}
	else
	{
		auto button = GetButton(myRect.w, myEnd, myPage, value.get());
		DrawScrollbar(myRect, button, false, action == ACT_HOVER_BUTTON_H, action == ACT_DRAGGING_H);
	}
}

void WgScrollbar::myUpdateValue(int v)
{
	double prev = value.get();
	v = max(0, min(myEnd, v));
	value.set(v);
	if(value.get() != prev) onChange.call();
}

uint WgScrollbar::myGetActionAt(int x, int y)
{
	if(myScrollAction) return myScrollAction;
	if(!IsInside(myRect, x, y)) return ACT_NONE;

	x -= myRect.x;
	y -= myRect.y;

	if(isVertical())
	{
		auto button = GetButton(myRect.h, myEnd, myPage, value.get());
		if(y < button.pos) return ACT_HOVER_BUMP_UP;
		if(y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}
	else
	{
		auto button = GetButton(myRect.w, myEnd, myPage, value.get());
		if(x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if(x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

WgScrollbarV::WgScrollbarV(GuiContext* gui)
	: WgScrollbar(gui)
{
	myWidth = 14;
}

bool WgScrollbarV::isVertical() const
{
	return true;
}

WgScrollbarH::WgScrollbarH(GuiContext* gui)
	: WgScrollbar(gui)
{
	myHeight = 14;
}

bool WgScrollbarH::isVertical() const
{
	return false;
}

// ================================================================================================
// WgScrollRegion.

static const int SCROLLBAR_SIZE = 14;

WgScrollRegion::~WgScrollRegion()
{
}

WgScrollRegion::WgScrollRegion(GuiContext* gui)
	: GuiWidget(gui)
	, myScrollTypeH(0)
	, myScrollTypeV(0)
	, myScrollbarActiveH(0)
	, myScrollbarActiveV(0)
	, myScrollAction(0)
	, myScrollGrabPos(0)
	, myScrollW(0)
	, myScrollH(0)
	, myScrollX(0)
	, myScrollY(0)
{
}

void WgScrollRegion::onMouseScroll(MouseScroll& evt)
{
	if(myScrollbarActiveV)
	{
		if(IsInside(myRect, evt.x, evt.y) && !evt.handled)
		{
			ApplyScrollOffset(myScrollY, getViewHeight(), myScrollH, evt.up);
			evt.handled = true;
		}
	}
}

void WgScrollRegion::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			uint action = myGetActionAt(evt.x, evt.y);
			if(action)
			{
				startCapturingMouse();
			}
			if(action == ACT_HOVER_BUTTON_H)
			{
				int w = getViewWidth();
				auto button = GetButton(w, myScrollW, w, myScrollX);
				myScrollGrabPos = evt.x - button.pos - myRect.x;
				myScrollAction = ACT_DRAGGING_H;
			}
			if(action == ACT_HOVER_BUTTON_V)
			{
				int h = getViewHeight();
				auto button = GetButton(h, myScrollH, h, myScrollY);
				myScrollGrabPos = evt.y - button.pos - myRect.y;
				myScrollAction = ACT_DRAGGING_V;
			}
		}
		evt.setHandled();
	}
}

void WgScrollRegion::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
	myScrollAction = 0;
}

void WgScrollRegion::onTick()
{
	myPreTick();
	myPostTick();
}

void WgScrollRegion::onDraw()
{
	vec2i mpos = myGui->getMousePos();
	uint action = myGetActionAt(mpos.x, mpos.y);

	int viewW = getViewWidth();
	int viewH = getViewHeight();

	if(myScrollbarActiveH)
	{
		recti bar = {myRect.x, myRect.y + myRect.h - SCROLLBAR_SIZE, viewW, SCROLLBAR_SIZE};
		auto button = GetButton(viewW, myScrollW, viewW, myScrollX);
		DrawScrollbar(bar, button, false, action == ACT_HOVER_BUTTON_H, action == ACT_DRAGGING_H);
	}
	if(myScrollbarActiveV)
	{
		recti bar = {myRect.x + myRect.w - SCROLLBAR_SIZE, myRect.y, SCROLLBAR_SIZE, viewH};
		auto button = GetButton(viewH, myScrollH, viewH, myScrollY);
		DrawScrollbar(bar, button, true, action == ACT_HOVER_BUTTON_V, action == ACT_DRAGGING_V);
	}
}

void WgScrollRegion::setScrollType(ScrollType h, ScrollType v)
{
	myScrollTypeH = h;
	myScrollTypeV = v;
}

void WgScrollRegion::setScrollW(int width)
{
	myScrollW = max(0, width);
}

void WgScrollRegion::setScrollH(int height)
{
	myScrollH = max(0, height);
}

int WgScrollRegion::getViewWidth() const
{
	return myRect.w - myScrollbarActiveV * SCROLLBAR_SIZE;
}

int WgScrollRegion::getViewHeight() const
{
	return myRect.h - myScrollbarActiveH * SCROLLBAR_SIZE;
}

int WgScrollRegion::getScrollWidth() const
{
	return myScrollW;
}

int WgScrollRegion::getScrollHeight() const
{
	return myScrollH;
}

void WgScrollRegion::myPreTick()
{
	vec2i mpos = myGui->getMousePos();

	int sh = myScrollTypeH;
	int sv = myScrollTypeV;

	myScrollbarActiveH = (sh == SCROLL_ALWAYS || (sh == SCROLL_WHEN_NEEDED && myScrollW > myRect.w));
	myScrollbarActiveV = (sv == SCROLL_ALWAYS || (sv == SCROLL_WHEN_NEEDED && myScrollH > myRect.h));

	if(myScrollAction == ACT_DRAGGING_H)
	{
		int w = getViewWidth();
		int buttonPos = myGui->getMousePos().x - myRect.x - myScrollGrabPos;
		myScrollX = GetScroll(w, myScrollW, w, buttonPos);
	}
	else if(myScrollAction == ACT_DRAGGING_V)
	{
		int h = getViewHeight();
		int buttonPos = myGui->getMousePos().y - myRect.y - myScrollGrabPos;
		myScrollY = GetScroll(h, myScrollH, h, buttonPos);
	}

	if(!IsInside(recti{myRect.x, myRect.y, getViewWidth(), getViewHeight()}, mpos.x, mpos.y))
	{
		blockMouseOver();
	}
}

void WgScrollRegion::myPostTick()
{
	unblockMouseOver();

	GuiWidget::onTick();
}

uint WgScrollRegion::myGetActionAt(int x, int y)
{
	if(myScrollAction) return myScrollAction;

	if(!IsInside(myRect, x, y)) return ACT_NONE;

	x -= myRect.x;
	y -= myRect.y;

	int viewW = getViewWidth();
	int viewH = getViewHeight();
	if(myScrollbarActiveV && x >= viewW && y < viewH)
	{
		auto button = GetButton(viewH, myScrollH, viewH, myScrollY);
		if(y < button.pos) return ACT_HOVER_BUMP_UP;
		if(y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}

	if(myScrollbarActiveH && y >= viewH && x < viewW)
	{
		auto button = GetButton(viewW, myScrollW, viewW, myScrollX);
		if(x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if(x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

void WgScrollRegion::myClampScrollValues()
{
	myScrollX = max(0, min(myScrollX, myScrollW - getViewWidth()));
	myScrollY = max(0, min(myScrollY, myScrollH - getViewHeight()));
}

}; // namespace Vortex
