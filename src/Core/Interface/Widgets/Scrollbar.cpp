#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Scrollbar.h>

namespace AV {

/*
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

struct ScrollButtonData { int pos, size; };

static ScrollButtonData GetButton(int size, int end, int page, int scroll)
{
	ScrollButtonData out = {0, size};
	if (end > page)
	{
		out.size = int(0.5 + size * (double)page / (double)end);
		out.size = max(16, out.size);

		out.pos = int(0.5 + scroll * (double)(size - out.size) / (double)(end - page));
		out.pos = max(0, min(out.pos, size - out.size));
	}
	return out;
}

static int GetScroll(int size, int end, int page, int pos)
{
	int out = 0;
	if (end > page)
	{
		ScrollButtonData button = GetButton(size, end, page, 0);
		out = int(0.5 + pos * (double)(end - page) / (double)(size - button.size));
		out = max(0, min(out, end - page));
	}
	return out;
}

static void DrawScrollbar(Rect bar, ScrollButtonData button, bool vertical, bool hover, bool focus)
{
	auto& scrollbar = UiStyle::getScrollbar();
	Rect buttonRect;
	if (vertical)
	{
		buttonRect = {bar.x, bar.y + button.pos, bar.w, button.size};
		scrollbar.base.draw(buttonRect);
		if (buttonRect.h >= 16)
		{
			Vec2 grabPos = {bar.x + bar.w / 2, buttonRect.y + buttonRect.h / 2};
			Draw::texture(UiStyle::getMisc()grab, grabPos, Draw::ROT_90);
		}
	}
	else
	{
		buttonRect = {bar.x + button.pos, bar.y, button.size, bar.h};
		scrollbar.base.draw(buttonRect);
		if (buttonRect.w >= 16)
		{
			Vec2 grabPos = {buttonRect.x + buttonRect.w / 2, bar.y + bar.h / 2};
			Draw::texture(UiStyle::getIcons().grab, grabPos, 0);
		}
	}
	if (focus)
	{
		scrollbar.press.draw(buttonRect);
	}
	else if (hover)
	{
		scrollbar.hover.draw(buttonRect);
	}
}

void ApplyScrollOffset(int& offset, int page, int scroll, bool up)
{
	int delta = max(1, page / 8);
	if (up) delta = -delta;
	offset = max(0, min(offset + delta, scroll - page));
}

WScrollbar::~WScrollbar()
{
}

WScrollbar::WScrollbar()
	: myScrollAction(0)
	, myScrollGrabPos(0)
{
	myEnd = 1;
	myPage = 1;
}

void WScrollbar::setEnd(int size)
{
	myEnd = size;
}

void WScrollbar::setPage(int size)
{
	myPage = size;
}

void WScrollbar::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::Lmb && input.unhandled())
		{
			uint action = myGetActionAt(input.x, input.y);
			if (action)
			{
				startCapturingMouse();
			}
			if (action == ACT_HOVER_BUTTON_H)
			{
				auto button = GetButton(myRect.w, myEnd, myPage, value.get());
				myScrollGrabPos = input.x - button.pos - myRect.l;
				myScrollAction = ACT_DRAGGING_H;
			}
			if (action == ACT_HOVER_BUTTON_V)
			{
				auto button = GetButton(myRect.h, myEnd, myPage, value.get());
				myScrollGrabPos = input.y - button.pos - myRect.t;
				myScrollAction = ACT_DRAGGING_V;
			}
		}
		input.setHandled();
	}
}

void WScrollbar::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
	myScrollAction = 0;
}

void WScrollbar::tick()
{
	if (myScrollAction == ACT_DRAGGING_H)
	{
		int buttonPos = Window::getMousePos().x - myRect.l - myScrollGrabPos;
		myUpdateValue(GetScroll(myRect.w, myEnd, myPage, buttonPos));
	}
	else if (myScrollAction == ACT_DRAGGING_V)
	{
		int buttonPos = Window::getMousePos().y - myRect.t - myScrollGrabPos;
		myUpdateValue(GetScroll(myRect.h, myEnd, myPage, buttonPos));
	}
	Widget::tick();
}

void WScrollbar::draw()
{
	Vec2 mpos = Window::getMousePos();
	uint action = myGetActionAt(mpos.x, mpos.y);

	if (isVertical())
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

void WScrollbar::myUpdateValue(int v)
{
	double prev = value.get();
	v = max(0, min(myEnd, v));
	value.set(v);
	if (value.get() != prev) onChange.call();
}

uint WScrollbar::myGetActionAt(int x, int y)
{
	if (myScrollAction) return myScrollAction;
	if (!myRect.contains(x, y)) return ACT_NONE;

	x -= myRect.l;
	y -= myRect.t;

	if (isVertical())
	{
		auto button = GetButton(myRect.h, myEnd, myPage, value.get());
		if (y < button.pos) return ACT_HOVER_BUMP_UP;
		if (y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}
	else
	{
		auto button = GetButton(myRect.w, myEnd, myPage, value.get());
		if (x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if (x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

WScrollbarV::WScrollbarV()
{
}

bool WScrollbarV::isVertical() const
{
	return true;
}

WScrollbarH::WScrollbarH()
{
}

bool WScrollbarH::isVertical() const
{
	return false;
}
*/
} // namespace AV
