#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/ScrollRegion.h>

namespace AV {

/*

using namespace std;
using namespace Util;

enum class ScrollActions
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

// =====================================================================================================================
// Helper functions.

struct ScrollButtonData { int pos, size; };

static ScrollButtonData GetButton(int barSize, int contentSize, int viewSize, int scrollOffset)
{
	ScrollButtonData out = {0, barSize};
	if (contentSize > viewSize)
	{
		out.size = int(0.5 + barSize * (double)viewSize / (double)contentSize);
		out.size = max(16, out.size);

		out.pos = int(0.5 + scrollOffset * (double)(barSize - out.size) / (double)(contentSize - viewSize));
		out.pos = max(0, min(out.pos, barSize - out.size));
	}
	return out;
}

static int GetScroll(int barSize, int contentSize, int viewSize, int pos)
{
	int out = 0;
	if (contentSize > viewSize)
	{
		ScrollButtonData button = GetButton(barSize, contentSize, viewSize, 0);
		out = int(0.5 + pos * (double)(contentSize - viewSize) / (double)(barSize - button.size));
		out = max(0, min(out, contentSize - viewSize));
	}
	return out;
}

static void DrawScrollbar(Rect bar, ScrollButtonData button, bool vertical, bool hover, bool focus)
{
	auto& scrollbar = UiStyle::getScrollbar();
	
	Rect buttonRect;
	if (vertical)
	{
		buttonRect = Rect(bar.l, bar.t + button.pos, bar.r, bar.t + button.pos + button.size);
	}
	else
	{
		buttonRect = Rect(bar.l + button.pos, bar.t, button.size, bar.h);
	}

	if (focus)
	{
		scrollbar.press.draw(buttonRect);
	}
	else if (hover)
	{
		scrollbar.hover.draw(buttonRect);
	}
	else
	{
		scrollbar.base.draw(buttonRect);
	}
}

void ApplyScrollOffset(int& offset, int viewSize, int contentSize, bool isUp)
{
	int delta = max(1, viewSize / 8);
	if (isUp) delta = -delta;
	offset = max(0, min(offset + delta, contentSize - viewSize));
}

// =====================================================================================================================
// WScrollRegion.

static constexpr int ScrollbarThickness = 14;

WScrollRegion::~WScrollRegion()
{
}

WScrollRegion::WScrollRegion()
	: myChild(nullptr)
	, myScrollTypeH(SHOW_WHEN_NEEDED)
	, myScrollTypeV(SHOW_WHEN_NEEDED)
	, myScrollbarActiveH(0)
	, myScrollbarActiveV(0)
	, myScrollAction(0)
	, myScrollGrabPos(0)
	, myMargin(0)
	, myScrollX(0)
	, myScrollY(0)
{
}

void WScrollRegion::onHandleInputs(InputEventList& events)
{
	if (myChild) myChild->onHandleInputs(events);
	InputHandler::onHandleInputs(events);
}

void WScrollRegion::onMouseScroll(MouseScroll& input)
{
	if (myScrollbarActiveV)
	{
		if (myRect.contains(input.pos) && input.unhandled())
		{
			ApplyScrollOffset(myScrollY, getViewportH(), myContentHeight, input.isUp);
			input.setHandled();
		}
	}
}

void WScrollRegion::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::Lmb && input.unhandled())
		{
			uint action = myGetActionAt(input.pos);
			if (action)
			{
				startCapturingMouse();
				if (action == ACT_HOVER_BUTTON_H)
				{
					int w = getViewportW();
					auto button = GetButton(w, myContentWidth, w, myScrollX);
					myScrollGrabPos = input.pos.x - button.pos - myRect.l;
					myScrollAction = ACT_DRAGGING_H;
				}
				if (action == ACT_HOVER_BUTTON_V)
				{
					int h = getViewportH();
					auto button = GetButton(h, myContentHeight, h, myScrollY);
					myScrollGrabPos = input.pos.y - button.pos - myRect.t;
					myScrollAction = ACT_DRAGGING_V;
				}
			}
		}
		input.setHandled();
	}
}

void WScrollRegion::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
	myScrollAction = 0;
}

void WScrollRegion::onMeasure()
{
	if (myChild)
	{
		myChild->measure();

		int margins = myMargin * 2;
		myWidth = myChild->getWidth() + margins;
		myHeight = myChild->getHeight() + margins;
		myMinWidth = myChild->getMinWidth() + margins;
		myMinHeight = myChild->getMinHeight() + margins;

		if (myScrollTypeH != SHOW_NEVER)
			myMinWidth = min(myMinWidth, 100);

		if (myScrollTypeV != SHOW_NEVER)
			myMinHeight = min(myMinHeight, 60);
	}
	else
	{
		myWidth = 0;
		myHeight = 0;
		myMinWidth = 0;
		myMinHeight = 0;
	}
}

void WScrollRegion::onArrange(Rect r)
{
	Widget::onArrange(r);
	if (myChild)
	{
		Size2 innerSize = getInnerSize();

		r.l += myMargin - myScrollX;
		r.t += myMargin - myScrollY;
		r.w = max(innerSize.w, getViewportW()) - myMargin * 2;
		r.h = max(innerSize.h, getViewportH()) - myMargin * 2;

		myChild->arrange(r);
	}
	myClampScrollValues();
}

void WScrollRegion::tick()
{
	Vec2 mpos = Window::getMousePos();

	int typeH = myScrollTypeH;
	int typeV = myScrollTypeV;

	int viewportW = myRect.w();
	int viewportH = myRect.h();

	Size2 size = getInnerSize();

	myScrollbarActiveH = (typeH == SHOW_ALWAYS || (typeH == SHOW_WHEN_NEEDED && size.w > viewportW));
	myScrollbarActiveV = (typeV == SHOW_ALWAYS || (typeV == SHOW_WHEN_NEEDED && size.h > viewportH));

	if (myScrollbarActiveV)
	{
		viewportW = getViewportW();
		myScrollbarActiveV |= (typeH == SHOW_WHEN_NEEDED && size.w > viewportW);
	}
	if (myScrollbarActiveH)
	{
		viewportH = getViewportH();
		myScrollbarActiveV |= (typeV == SHOW_WHEN_NEEDED && size.h > viewportH);
	}

	if (myScrollAction == ACT_DRAGGING_H)
	{
		int buttonPos = Window::getMousePos().x - myRect.l - myScrollGrabPos;
		myScrollX = GetScroll(viewportW, size.w, viewportW, buttonPos);
	}
	else if (myScrollAction == ACT_DRAGGING_V)
	{
		int buttonPos = Window::getMousePos().y - myRect.t - myScrollGrabPos;
		myScrollY = GetScroll(viewportH, size.h, viewportH, buttonPos);
	}

	if (!Rect(myRect.l, myRect.t, viewportW, viewportH).contains(mpos))
	{
		blockMouseOver();
	}

	if (myChild) myChild->tick();

	unblockMouseOver();

	Widget::tick();
}

void WScrollRegion::draw()
{
	Vec2 mpos = Window::getMousePos();
	uint action = myGetActionAt(mpos);

	int viewW = getViewportW();
	int viewH = getViewportH();

	Size2 innerSize = getInnerSize();

	// Scrollbars

	Rect barH(myRect.l, myRect.t + myRect.h() - ScrollbarThickness + 1, myRect.w, ScrollbarThickness - 2);
	Rect barV(myRect.l + myRect.w - ScrollbarThickness + 1, myRect.t, ScrollbarThickness - 2, myRect.h);

	if (myScrollbarActiveH & myScrollbarActiveV)
	{
		barH.w -= ScrollbarThickness;
		barV.h -= ScrollbarThickness;
	}

	if (myScrollbarActiveH)
	{
		auto button = GetButton(viewW, innerSize.w, viewW, myScrollX);
		DrawScrollbar(barH, button, false, action == ACT_HOVER_BUTTON_H, action == ACT_DRAGGING_H);
	}

	if (myScrollbarActiveV)
	{
		auto button = GetButton(viewH, innerSize.h, viewH, myScrollY);
		DrawScrollbar(barV, button, true, action == ACT_HOVER_BUTTON_V, action == ACT_DRAGGING_V);
	}

	// View border.
	Rect viewportRect = {myRect.l, myRect.t, viewW, viewH};
	Draw::fill(viewportRect, Color(51, 255));
	Draw::outline(viewportRect, Color(89, 255));

	// Inner region.
	if (myChild)
	{
		if (myScrollbarActiveH || myScrollbarActiveV)
		{
			viewportRect = viewportRect.shrink(1);
			Renderer::pushScissorRect(viewportRect);
			myChild->draw();
			Renderer::popScissorRect();
		}
		else
		{
			myChild->draw();
		}
	}
}

void WScrollRegion::onApply(WidgetAction action)
{
	if (myChild)
	{
		action(*myChild);
		myChild->apply(action);
	}
}

void WScrollRegion::setMargin(int margin)
{
	myMargin = margin;
}

void WScrollRegion::setChild(shared_ptr<Widget> child)
{
	myChild = child;
}

void WScrollRegion::setScrollType(ScrollType h, ScrollType v)
{
	myScrollTypeH = h;
	myScrollTypeV = v;
}

int WScrollRegion::getViewportW() const
{
	return myRect.w - myScrollbarActiveV * ScrollbarThickness;
}

int WScrollRegion::getViewportH() const
{
	return myRect.h - myScrollbarActiveH * ScrollbarThickness;
}

Size2 WScrollRegion::getInnerSize() const
{
	if (myChild)
	{
		int margins = myMargin * 2;
		int w = myChild->getWidth() + margins;
		int h = myChild->getHeight() + margins;
		return {w, h};
	}
	return {0, 0};
}

uint WScrollRegion::myGetActionAt(Vec2 pos)
{
	if (myScrollAction) return myScrollAction;

	if (!myRect.contains(pos)) return ACT_NONE;

	pos.x -= myRect.l;
	pos.y -= myRect.t;

	int viewW = getViewportW();
	int viewH = getViewportH();

	Size2 innerSize = getInnerSize();

	if (myScrollbarActiveV && pos.x >= viewW && pos.y < viewH)
	{
		auto button = GetButton(viewH, innerSize.h, viewH, myScrollY);
		if (pos.y < button.pos) return ACT_HOVER_BUMP_UP;
		if (pos.y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}

	if (myScrollbarActiveH && pos.y >= viewH && pos.x < viewW)
	{
		auto button = GetButton(viewW, innerSize.w, viewW, myScrollX);
		if (pos.x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if (pos.x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

void WScrollRegion::myClampScrollValues()
{
	if (myChild)
	{
		Size2 innerSize = getInnerSize();

		myScrollX = max(0, min(myScrollX, innerSize.w - getViewportW()));
		myScrollY = max(0, min(myScrollY, innerSize.h - getViewportH()));
	}
}

*/

} // namespace AV
