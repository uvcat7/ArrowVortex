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

	delete myScrollbar;
}

WgSelectList::WgSelectList(GuiContext* gui)
	: GuiWidget(gui)
	, myScrollPos(0)
	, myIsInteracted(0)
	, myShowBackground(1)
{
	myScrollbar = new WgScrollbarV(myGui);
	myScrollbar->value.bind(&myScrollPos);
}

void WgSelectList::hideBackground()
{
	myShowBackground = 0;
}

void WgSelectList::addItem(StringRef text)
{
	myItems.push_back(text);
}

void WgSelectList::clearItems()
{
	myItems.clear();
}

void WgSelectList::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();

			// Handle interaction with the item list.
			int i = myHoverItem(evt.x, evt.y);
			if(i >= 0)
			{
				myIsInteracted = 1;
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
	if(isMouseOver() && myHasScrollbar() && !evt.handled)
	{
		scroll(evt.up);
		evt.handled = true;
	}
}

void WgSelectList::scroll(bool up)
{
	if(myHasScrollbar())
	{
		int end = myItems.size() * ITEM_H - myItemRect().h;
		int delta = up ? -ITEM_H : ITEM_H;
		myScrollPos = min(max(myScrollPos + delta, 0), end);
	}
}

void WgSelectList::onArrange(recti r)
{
	GuiWidget::onArrange(r);

	int w = myScrollbar->getWidth();
	myScrollbar->arrange({r.x + r.w - w - 2, r.y + 3, w, r.h - 6});
}

void WgSelectList::onTick()
{
	myScrollbar->setEnd(myItems.size() * ITEM_H);
	myScrollbar->setPage(myItemRect().h);
	myScrollbar->tick();

	GuiWidget::onTick();
}

void WgSelectList::onDraw()
{
	auto& misc = GuiDraw::getMisc();
	auto& textbox = GuiDraw::getTextBox();

	// Draw the list background box.
	recti r = myRect;
	if(myShowBackground)
	{
		textbox.base.draw(r);
	}

	r = myItemRect();
	Renderer::pushScissorRect(r.x, r.y, r.w, r.h);

	// Highlight the currently selected item.
	int item = value.get(), numItems = myItems.size();
	if(item >= 0 && item < numItems)
	{
		misc.imgSelect.draw({r.x, r.y - myScrollPos + item * ITEM_H, r.w, ITEM_H});
	}

	// Highlight the mouse over item.
	if(isMouseOver())
	{
		vec2i mpos = myGui->getMousePos();
		int mo = myHoverItem(mpos.x, mpos.y);
		if(mo != item && mo >= 0 && mo < numItems)
		{
			color32 col = Color32(255, 255, 255, isEnabled() ? 128 : 0);
			misc.imgSelect.draw({r.x, r.y - myScrollPos + mo * ITEM_H, r.w, ITEM_H}, col);
		}
	}

	// Draw the item texts.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	for(int i = 0, ty = r.y - myScrollPos; i<numItems; ++i, ty += ITEM_H)
	{
		if(ty > r.y - ITEM_H && ty < r.y + r.h)
		{
			Text::arrange(Text::MC, style, myItems[i].str());
			Text::draw({r.x + 2, ty, r.w - 2, ITEM_H});
		}
	}

	Renderer::popScissorRect();

	// Draw the scrollbar.
	if(myHasScrollbar())
	{
		myScrollbar->draw();
	}
}

int WgSelectList::myHoverItem(int x, int y)
{
	recti r = myItemRect();
	if(x >= r.x && y >= r.y && x < r.x+r.w && y < r.y+r.h)
	{
		int i = (y - r.y + myScrollPos) / ITEM_H;
		if(i >= 0 && i < myItems.size()) return i;
	}
	return -1;
}

bool WgSelectList::myHasScrollbar() const
{
	return (myItems.size() * ITEM_H) > (myRect.h - 6);
}

recti WgSelectList::myItemRect() const
{
	recti r = myRect;
	if(myHasScrollbar()) r.w -= myScrollbar->getWidth() + 1;
	return {r.x+3, r.y+3, r.w-6, r.h-6};
}

#undef ITEM_H

// ================================================================================================
// WgDroplist

WgDroplist::~WgDroplist()
{
	clearItems();
	myCloseList();
}

WgDroplist::WgDroplist(GuiContext* gui)
	: GuiWidget(gui)
{
	myList = nullptr;
}

void WgDroplist::onMousePress(MousePress& evt)
{
	if(myList)
	{
		recti r = myList->getRect();
		if(evt.button == Mouse::RMB || !IsInside(r, evt.x, evt.y))
		{
			myCloseList();
			evt.setHandled();
		}
	}
	else if(isMouseOver())
	{
		int numItems = myItems.size();
		if(isEnabled() && numItems && evt.button == Mouse::LMB && evt.unhandled())
		{
			int h = min(numItems * 18 + 8, 128);
			recti r = {myRect.x, myRect.y + myRect.h, myRect.w, h};
			myList = new WgSelectList(myGui);
			myList->setHeight(h);
			myList->onArrange(r);
			myList->hideBackground();

			for(int i = 0; i < numItems; ++i)
			{
				myList->addItem(myItems[i].str());
			}

			myListItem = value.get();
			myList->value.bind(&myListItem);

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
	if(myList && !evt.handled)
	{
		myList->scroll(evt.up);
		evt.handled = true;
	}
}

void WgDroplist::onArrange(recti r)
{
	GuiWidget::onArrange(r);
	if(myList)
	{
		myList->onArrange({myRect.x, myRect.y + myRect.h, myRect.w, myList->getSize().y});
	}
}

void WgDroplist::onTick()
{
	GuiWidget::onTick();
	if(myList)
	{
		myList->tick();
		if(myList->interacted())
		{
			if(myListItem != value.get())
			{
				value.set(myListItem);
				onChange.call();
			}
			myCloseList();
		}
	}	
}

void WgDroplist::onDraw()
{
	auto& button = GuiDraw::getButton();
	auto& misc = GuiDraw::getMisc();
	auto& icons = GuiDraw::getIcons();

	int numItems = myItems.size();

	// Draw the list of items.
	if(myList)
	{
		recti r = Expand(myRect, 1);
		r.h += myList->getHeight();
		auto& textbox = GuiDraw::getTextBox();
		textbox.base.draw(r);
		myList->draw();
	}

	// Draw the button graphic.
	recti r = myRect;
	button.base.draw(myRect);
	if(isEnabled()) Draw::sprite(icons.arrow, {r.x + r.w - 10, r.y + r.h / 2}, Draw::ROT_90);

	// Draw the selected item text.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	if(!isEnabled()) style.textColor = misc.colDisabled;

	int item = value.get();
	if(item >= 0 && item < numItems)
	{
		Text::arrange(Text::MC, style, myRect.w - 18, myItems[item].str());
		Text::draw({r.x + 6, r.y, r.w - 18, r.h});
	}

	// Draw interaction effects.
	if(isCapturingMouse())
	{
		button.pressed.draw(myRect);
	}
	else if(isMouseOver())
	{
		button.hover.draw(myRect);
	}
}

void WgDroplist::clearItems()
{
	myItems.clear();
}

void WgDroplist::addItem(StringRef text)
{
	myItems.push_back(text);
}

void WgDroplist::myCloseList()
{
	if(myList)
	{
		delete myList;
		myList = nullptr;
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
	myItems.push_back(text);
}

void WgCycleButton::clearItems()
{
	myItems.clear();
}

void WgCycleButton::onMousePress(MousePress& evt)
{
	int numItems = myItems.size();
	if(isMouseOver())
	{
		if(numItems > 1 && isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			stopCapturingText();
			startCapturingMouse();
			if(evt.x > CenterX(myRect))
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

	int numItems = myItems.size(), item = value.get();

	// Draw the button graphic.
	recti r = myRect;
	button.base.draw(r);

	// Draw the arrow graphics.
	int mouseX = myGui->getMousePos().x - CenterX(myRect);
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

		Renderer::pushScissorRect(Shrink(myRect, 2));
		Text::arrange(Text::MC, style, myItems[item].str());
		Text::draw({r.x + 2, r.y, r.w - 4, r.h});
		Renderer::popScissorRect();
	}

	// Draw interaction effects.
	if(isCapturingMouse())
	{
		button.pressed.draw(myRect);
	}
	else if(isMouseOver())
	{
		button.hover.draw(myRect);
	}
}

}; // namespace Vortex
