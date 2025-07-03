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
	, scrollbarAction_(0)
	, scrollbarGrabPosition_(0)
{
	scrollbarEnd_ = 1;
	scrollbarPage_ = 1;
}

void WgScrollbar::setEnd(int size)
{
	scrollbarEnd_ = size;
}

void WgScrollbar::setPage(int size)
{
	scrollbarPage_ = size;
}

void WgScrollbar::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			uint action = getScrollbarActionAt_(evt.x, evt.y);
			if(action)
			{
				startCapturingMouse();
			}
			if(action == ACT_HOVER_BUTTON_H)
			{
				auto button = GetButton(rect_.w, scrollbarEnd_, scrollbarPage_, value.get());
				scrollbarGrabPosition_ = evt.x - button.pos - rect_.x;
				scrollbarAction_ = ACT_DRAGGING_H;
			}
			if(action == ACT_HOVER_BUTTON_V)
			{
				auto button = GetButton(rect_.h, scrollbarEnd_, scrollbarPage_, value.get());
				scrollbarGrabPosition_ = evt.y - button.pos - rect_.y;
				scrollbarAction_ = ACT_DRAGGING_V;
			}
		}
		evt.setHandled();
	}
}

void WgScrollbar::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
	scrollbarAction_ = 0;
}

void WgScrollbar::onTick()
{
	if(scrollbarAction_ == ACT_DRAGGING_H)
	{
		int buttonPos = gui_->getMousePos().x - rect_.x - scrollbarGrabPosition_;
		scrollbarUpdateValue_(GetScroll(rect_.w, scrollbarEnd_, scrollbarPage_, buttonPos));
	}
	else if(scrollbarAction_ == ACT_DRAGGING_V)
	{
		int buttonPos = gui_->getMousePos().y - rect_.y - scrollbarGrabPosition_;
		scrollbarUpdateValue_(GetScroll(rect_.h, scrollbarEnd_, scrollbarPage_, buttonPos));
	}
	GuiWidget::onTick();
}

void WgScrollbar::onDraw()
{
	vec2i mpos = gui_->getMousePos();
	uint action = getScrollbarActionAt_(mpos.x, mpos.y);

	if(isVertical())
	{
		auto button = GetButton(rect_.h, scrollbarEnd_, scrollbarPage_, value.get());
		DrawScrollbar(rect_, button, true, action == ACT_HOVER_BUTTON_V, action == ACT_DRAGGING_V);
	}
	else
	{
		auto button = GetButton(rect_.w, scrollbarEnd_, scrollbarPage_, value.get());
		DrawScrollbar(rect_, button, false, action == ACT_HOVER_BUTTON_H, action == ACT_DRAGGING_H);
	}
}

void WgScrollbar::scrollbarUpdateValue_(int v)
{
	double prev = value.get();
	v = max(0, min(scrollbarEnd_, v));
	value.set(v);
	if(value.get() != prev) onChange.call();
}

uint WgScrollbar::getScrollbarActionAt_(int x, int y)
{
	if(scrollbarAction_) return scrollbarAction_;
	if(!IsInside(rect_, x, y)) return ACT_NONE;

	x -= rect_.x;
	y -= rect_.y;

	if(isVertical())
	{
		auto button = GetButton(rect_.h, scrollbarEnd_, scrollbarPage_, value.get());
		if(y < button.pos) return ACT_HOVER_BUMP_UP;
		if(y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}
	else
	{
		auto button = GetButton(rect_.w, scrollbarEnd_, scrollbarPage_, value.get());
		if(x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if(x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

WgScrollbarV::WgScrollbarV(GuiContext* gui)
	: WgScrollbar(gui)
{
	width_ = 14;
}

bool WgScrollbarV::isVertical() const
{
	return true;
}

WgScrollbarH::WgScrollbarH(GuiContext* gui)
	: WgScrollbar(gui)
{
	height_ = 14;
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
	, scrollTypeHorizontal_(0)
	, scrollTypeVertical_(0)
	, isHorizontalScrollbarActive_(0)
	, isVerticalScrollbarActive_(0)
	, scrollRegionAction_(0)
	, scrollRegionGrabPosition_(0)
	, scrollWidth_(0)
	, scrollHeight_(0)
	, scrollPositionX_(0)
	, scrollPositionY_(0)
{
}

void WgScrollRegion::onMouseScroll(MouseScroll& evt)
{
	if(isVerticalScrollbarActive_)
	{
		if(IsInside(rect_, evt.x, evt.y) && !evt.handled)
		{
			ApplyScrollOffset(scrollPositionY_, getViewHeight(), scrollHeight_, evt.up);
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
			uint action = getScrollRegionActionAt_(evt.x, evt.y);
			if(action)
			{
				startCapturingMouse();
			}
			if(action == ACT_HOVER_BUTTON_H)
			{
				int w = getViewWidth();
				auto button = GetButton(w, scrollWidth_, w, scrollPositionX_);
				scrollRegionGrabPosition_ = evt.x - button.pos - rect_.x;
				scrollRegionAction_ = ACT_DRAGGING_H;
			}
			if(action == ACT_HOVER_BUTTON_V)
			{
				int h = getViewHeight();
				auto button = GetButton(h, scrollHeight_, h, scrollPositionY_);
				scrollRegionGrabPosition_ = evt.y - button.pos - rect_.y;
				scrollRegionAction_ = ACT_DRAGGING_V;
			}
		}
		evt.setHandled();
	}
}

void WgScrollRegion::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
	scrollRegionAction_ = 0;
}

void WgScrollRegion::onTick()
{
	preTick_();
	postTick_();
}

void WgScrollRegion::onDraw()
{
	vec2i mpos = gui_->getMousePos();
	uint action = getScrollRegionActionAt_(mpos.x, mpos.y);

	int viewW = getViewWidth();
	int viewH = getViewHeight();

	if(isHorizontalScrollbarActive_)
	{
		recti bar = {rect_.x, rect_.y + rect_.h - SCROLLBAR_SIZE, viewW, SCROLLBAR_SIZE};
		auto button = GetButton(viewW, scrollWidth_, viewW, scrollPositionX_);
		DrawScrollbar(bar, button, false, action == ACT_HOVER_BUTTON_H, action == ACT_DRAGGING_H);
	}
	if(isVerticalScrollbarActive_)
	{
		recti bar = {rect_.x + rect_.w - SCROLLBAR_SIZE, rect_.y, SCROLLBAR_SIZE, viewH};
		auto button = GetButton(viewH, scrollHeight_, viewH, scrollPositionY_);
		DrawScrollbar(bar, button, true, action == ACT_HOVER_BUTTON_V, action == ACT_DRAGGING_V);
	}
}

void WgScrollRegion::setScrollType(ScrollType h, ScrollType v)
{
	scrollTypeHorizontal_ = h;
	scrollTypeVertical_ = v;
}

void WgScrollRegion::setScrollW(int width)
{
	scrollWidth_ = max(0, width);
}

void WgScrollRegion::setScrollH(int height)
{
	scrollHeight_ = max(0, height);
}

int WgScrollRegion::getViewWidth() const
{
	return rect_.w - isVerticalScrollbarActive_ * SCROLLBAR_SIZE;
}

int WgScrollRegion::getViewHeight() const
{
	return rect_.h - isHorizontalScrollbarActive_ * SCROLLBAR_SIZE;
}

int WgScrollRegion::getScrollWidth() const
{
	return scrollWidth_;
}

int WgScrollRegion::getScrollHeight() const
{
	return scrollHeight_;
}

void WgScrollRegion::preTick_()
{
	vec2i mpos = gui_->getMousePos();

	int sh = scrollTypeHorizontal_;
	int sv = scrollTypeVertical_;

	isHorizontalScrollbarActive_ = (sh == SCROLL_ALWAYS || (sh == SCROLL_WHEN_NEEDED && scrollWidth_ > rect_.w));
	isVerticalScrollbarActive_ = (sv == SCROLL_ALWAYS || (sv == SCROLL_WHEN_NEEDED && scrollHeight_ > rect_.h));

	if(scrollRegionAction_ == ACT_DRAGGING_H)
	{
		int w = getViewWidth();
		int buttonPos = gui_->getMousePos().x - rect_.x - scrollRegionGrabPosition_;
		scrollPositionX_ = GetScroll(w, scrollWidth_, w, buttonPos);
	}
	else if(scrollRegionAction_ == ACT_DRAGGING_V)
	{
		int h = getViewHeight();
		int buttonPos = gui_->getMousePos().y - rect_.y - scrollRegionGrabPosition_;
		scrollPositionY_ = GetScroll(h, scrollHeight_, h, buttonPos);
	}

	if(!IsInside(recti{rect_.x, rect_.y, getViewWidth(), getViewHeight()}, mpos.x, mpos.y))
	{
		blockMouseOver();
	}
}

void WgScrollRegion::postTick_()
{
	unblockMouseOver();

	GuiWidget::onTick();
}

uint WgScrollRegion::getScrollRegionActionAt_(int x, int y)
{
	if(scrollRegionAction_) return scrollRegionAction_;

	if(!IsInside(rect_, x, y)) return ACT_NONE;

	x -= rect_.x;
	y -= rect_.y;

	int viewW = getViewWidth();
	int viewH = getViewHeight();
	if(isVerticalScrollbarActive_ && x >= viewW && y < viewH)
	{
		auto button = GetButton(viewH, scrollHeight_, viewH, scrollPositionY_);
		if(y < button.pos) return ACT_HOVER_BUMP_UP;
		if(y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
		return ACT_HOVER_BUMP_DOWN;
	}

	if(isHorizontalScrollbarActive_ && y >= viewH && x < viewW)
	{
		auto button = GetButton(viewW, scrollWidth_, viewW, scrollPositionX_);
		if(x < button.pos) return ACT_HOVER_BUMP_LEFT;
		if(x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
		return ACT_HOVER_BUMP_RIGHT;
	}

	return ACT_NONE;
}

void WgScrollRegion::clampScrollValues_()
{
	scrollPositionX_ = max(0, min(scrollPositionX_, scrollWidth_ - getViewWidth()));
	scrollPositionY_ = max(0, min(scrollPositionY_, scrollHeight_ - getViewHeight()));
}

}; // namespace Vortex
