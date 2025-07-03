#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

namespace Vortex {

// ================================================================================================
// WgSelectList

#define ITEM_H 18

WgSelectList::~WgSelectList()
{
	clearItems();

	delete scrollbar_;
}

WgSelectList::WgSelectList(GuiContext* gui)
	: GuiWidget(gui)
	, scrollPosition_(0)
	, isInteracted_(0)
	, showBackground_(1)
{
	scrollbar_ = new WgScrollbarV(gui_);
	scrollbar_->value.bind(&scrollPosition_);
}

void WgSelectList::hideBackground()
{
	showBackground_ = 0;
}

void WgSelectList::addItem(StringRef text)
{
	scrollbarItems_.push_back(text);
}

void WgSelectList::clearItems()
{
	scrollbarItems_.clear();
}

void WgSelectList::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();

			// Handle interaction with the item list.
			int i = HoveredItem_(evt.x, evt.y);
			if(i >= 0)
			{
				isInteracted_ = 1;
				if(value.get() != i)
				{
					value.set(i);
					onChange.call();
				}
			}
		}
		evt.setHandled();
	}
}

void WgSelectList::onMouseRelease(MouseRelease& evt)
{
	if(isCapturingMouse() && evt.button == Mouse::LMB)
	{
		stopCapturingMouse();
	}
}

void WgSelectList::onMouseScroll(MouseScroll& evt)
{
	if(isMouseOver() && hasScrollbar_() && !evt.handled)
	{
		scroll(evt.up);
		evt.handled = true;
	}
}

void WgSelectList::scroll(bool up)
{
	if(hasScrollbar_())
	{
		int end = scrollbarItems_.size() * ITEM_H - itemRect_().h;
		int delta = up ? -ITEM_H : ITEM_H;
		scrollPosition_ = min(max(scrollPosition_ + delta, 0), end);
	}
}

void WgSelectList::onArrange(recti r)
{
	GuiWidget::onArrange(r);

	int w = scrollbar_->getWidth();
	scrollbar_->arrange({r.x + r.w - w - 2, r.y + 3, w, r.h - 6});
}

void WgSelectList::onTick()
{
	scrollbar_->setEnd(scrollbarItems_.size() * ITEM_H);
	scrollbar_->setPage(itemRect_().h);
	scrollbar_->tick();

	GuiWidget::onTick();
}

void WgSelectList::onDraw()
{
	auto& misc = GuiDraw::getMisc();
	auto& textbox = GuiDraw::getTextBox();

	// Draw the list background box.
	recti r = rect_;
	if(showBackground_)
	{
		textbox.base.draw(r);
	}

	r = itemRect_();
	Renderer::pushScissorRect(r.x, r.y, r.w, r.h);

	// Highlight the currently selected item.
	int item = value.get(), numItems = scrollbarItems_.size();
	if(item >= 0 && item < numItems)
	{
		misc.imgSelect.draw({r.x, r.y - scrollPosition_ + item * ITEM_H, r.w, ITEM_H});
	}

	// Highlight the mouse over item.
	if(isMouseOver())
	{
		vec2i mpos = gui_->getMousePos();
		int mo = HoveredItem_(mpos.x, mpos.y);
		if(mo != item && mo >= 0 && mo < numItems)
		{
			color32 col = Color32(255, 255, 255, isEnabled() ? 128 : 0);
			misc.imgSelect.draw({r.x, r.y - scrollPosition_ + mo * ITEM_H, r.w, ITEM_H}, col);
		}
	}

	// Draw the item texts.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	for(int i = 0, ty = r.y - scrollPosition_; i<numItems; ++i, ty += ITEM_H)
	{
		if(ty > r.y - ITEM_H && ty < r.y + r.h)
		{
			Text::arrange(Text::MC, style, scrollbarItems_[i].str());
			Text::draw({r.x + 2, ty, r.w - 2, ITEM_H});
		}
	}

	Renderer::popScissorRect();

	// Draw the scrollbar.
	if(hasScrollbar_())
	{
		scrollbar_->draw();
	}
}

int WgSelectList::HoveredItem_(int x, int y)
{
	recti r = itemRect_();
	if(x >= r.x && y >= r.y && x < r.x+r.w && y < r.y+r.h)
	{
		int i = (y - r.y + scrollPosition_) / ITEM_H;
		if(i >= 0 && i < scrollbarItems_.size()) return i;
	}
	return -1;
}

bool WgSelectList::hasScrollbar_() const
{
	return (scrollbarItems_.size() * ITEM_H) > (rect_.h - 6);
}

recti WgSelectList::itemRect_() const
{
	recti r = rect_;
	if(hasScrollbar_()) r.w -= scrollbar_->getWidth() + 1;
	return {r.x+3, r.y+3, r.w-6, r.h-6};
}

#undef ITEM_H

// ================================================================================================
// WgDroplist

WgDroplist::~WgDroplist()
{
	clearItems();
	dropListClose_();
}

WgDroplist::WgDroplist(GuiContext* gui)
	: GuiWidget(gui)
{
	selectList_ = nullptr;
}

void WgDroplist::onMousePress(MousePress& evt)
{
	if(selectList_)
	{
		recti r = selectList_->getRect();
		if(evt.button == Mouse::RMB || !IsInside(r, evt.x, evt.y))
		{
			dropListClose_();
			evt.setHandled();
		}
	}
	else if(isMouseOver())
	{
		int numItems = listItems_.size();
		if(isEnabled() && numItems && evt.button == Mouse::LMB && evt.unhandled())
		{
			int h = min(numItems * 18 + 8, 128);
			recti r = {rect_.x, rect_.y + rect_.h, rect_.w, h};
			selectList_ = new WgSelectList(gui_);
			selectList_->setHeight(h);
			selectList_->onArrange(r);
			selectList_->hideBackground();

			for(int i = 0; i < numItems; ++i)
			{
				selectList_->addItem(listItems_[i].str());
			}

			selectedIndex_ = value.get();
			selectList_->value.bind(&selectedIndex_);

			startCapturingMouse();
			startCapturingFocus();
		}
		evt.setHandled();
	}
}

void WgDroplist::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
}

void WgDroplist::onMouseScroll(MouseScroll& evt)
{
	if(selectList_ && !evt.handled)
	{
		selectList_->scroll(evt.up);
		evt.handled = true;
	}
}

void WgDroplist::onArrange(recti r)
{
	GuiWidget::onArrange(r);
	if(selectList_)
	{
		selectList_->onArrange({rect_.x, rect_.y + rect_.h, rect_.w, selectList_->getSize().y});
	}
}

void WgDroplist::onTick()
{
	GuiWidget::onTick();
	if(selectList_)
	{
		selectList_->tick();
		if(selectList_->interacted())
		{
			if(selectedIndex_ != value.get())
			{
				value.set(selectedIndex_);
				onChange.call();
			}
			dropListClose_();
		}
	}	
}

void WgDroplist::onDraw()
{
	auto& button = GuiDraw::getButton();
	auto& misc = GuiDraw::getMisc();
	auto& icons = GuiDraw::getIcons();

	int numItems = listItems_.size();

	// Draw the list of items.
	if(selectList_)
	{
		recti r = Expand(rect_, 1);
		r.h += selectList_->getHeight();
		auto& textbox = GuiDraw::getTextBox();
		textbox.base.draw(r);
		selectList_->draw();
	}

	// Draw the button graphic.
	recti r = rect_;
	button.base.draw(rect_);
	if(isEnabled()) Draw::sprite(icons.arrow, {r.x + r.w - 10, r.y + r.h / 2}, Draw::ROT_90);

	// Draw the selected item text.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	if(!isEnabled()) style.textColor = misc.colDisabled;

	int item = value.get();
	if(item >= 0 && item < numItems)
	{
		Text::arrange(Text::MC, style, rect_.w - 18, listItems_[item].str());
		Text::draw({r.x + 6, r.y, r.w - 18, r.h});
	}

	// Draw interaction effects.
	if(isCapturingMouse())
	{
		button.pressed.draw(rect_);
	}
	else if(isMouseOver())
	{
		button.hover.draw(rect_);
	}
}

void WgDroplist::clearItems()
{
	listItems_.clear();
}

void WgDroplist::addItem(StringRef text)
{
	listItems_.push_back(text);
}

void WgDroplist::dropListClose_()
{
	if(selectList_)
	{
		delete selectList_;
		selectList_ = nullptr;
		stopCapturingFocus();
	}
}

// ================================================================================================
// WgCycleButton

WgCycleButton::~WgCycleButton()
{
	clearItems();
}

WgCycleButton::WgCycleButton(GuiContext* gui)
	: GuiWidget(gui)
{
}

void WgCycleButton::addItem(StringRef text)
{
	cycleItems_.push_back(text);
}

void WgCycleButton::clearItems()
{
	cycleItems_.clear();
}

void WgCycleButton::onMousePress(MousePress& evt)
{
	int numItems = cycleItems_.size();
	if(isMouseOver())
	{
		if(numItems > 1 && isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			stopCapturingText();
			startCapturingMouse();
			if(evt.x > CenterX(rect_))
			{
				value.set((value.get() + 1) % numItems);
			}
			else
			{
				value.set((value.get() + numItems - 1) % numItems);
			}
			onChange.call();
		}
		evt.setHandled();
	}
}

void WgCycleButton::onMouseRelease(MouseRelease& evt)
{
	if(isCapturingMouse() && evt.button == Mouse::LMB)
	{
		stopCapturingMouse();
	}
}

void WgCycleButton::onDraw()
{
	auto& misc = GuiDraw::getMisc();
	auto& icons = GuiDraw::getIcons();
	auto& button = GuiDraw::getButton();

	int numItems = cycleItems_.size(), item = value.get();

	// Draw the button graphic.
	recti r = rect_;
	button.base.draw(r);

	// Draw the arrow graphics.
	int mouseX = gui_->getMousePos().x - CenterX(rect_);
	if(!isMouseOver()) mouseX = 0;

	color32 colL = (isEnabled() && mouseX < 0) ? Colors::white : misc.colDisabled;
	Draw::sprite(icons.arrow, {r.x + 10, r.y + r.h / 2}, colL, Draw::FLIP_H);

	color32 colR = (isEnabled() && mouseX > 0) ? Colors::white : misc.colDisabled;
	Draw::sprite(icons.arrow, {r.x + r.w - 10, r.y + r.h / 2}, colR);

	// Draw the button text.
	if(item >= 0 && item < numItems)
	{
		TextStyle style;
		style.textFlags = Text::MARKUP | Text::ELLIPSES;
		if(!isEnabled()) style.textColor = misc.colDisabled;

		Renderer::pushScissorRect(Shrink(rect_, 2));
		Text::arrange(Text::MC, style, cycleItems_[item].str());
		Text::draw({r.x + 2, r.y, r.w - 4, r.h});
		Renderer::popScissorRect();
	}

	// Draw interaction effects.
	if(isCapturingMouse())
	{
		button.pressed.draw(rect_);
	}
	else if(isMouseOver())
	{
		button.hover.draw(rect_);
	}
}

}; // namespace Vortex
