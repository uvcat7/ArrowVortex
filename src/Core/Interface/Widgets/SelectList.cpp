#include <Precomp.h>

#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/SelectList.h>

namespace AV {

using namespace std;

#define ITEM_H 18

// TODO: reimplement scrollbar.

WSelectList::~WSelectList()
{
	clearItems();
}

WSelectList::WSelectList()
	: myScrollPos(0)
	, mySelectedItem(0)
	, myShowBackground(1)
	, myWasInteracted(1)
{
}

void WSelectList::hideBackground()
{
	myShowBackground = 0;
}

void WSelectList::addItem(stringref text)
{
	myItems.push_back(text);
}

void WSelectList::clearItems()
{
	myItems.clear();
	setSelectedItem(-1);
}

void WSelectList::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();

			// Handle interaction with the item list.
			int i = myHoverItem(input.pos);
			if (i >= 0)
			{
				myWasInteracted = 1;
				setSelectedItem(i);
			}
		}
		input.setHandled();
	}
}

void WSelectList::onMouseRelease(MouseRelease& input)
{
	if (isCapturingMouse() && input.button == MouseButton::LMB)
	{
		stopCapturingMouse();
	}
}

void WSelectList::onMouseScroll(MouseScroll& input)
{
	if (isMouseOver() && myHasScrollbar() && input.unhandled())
	{
		scroll(input.isUp);
		input.setHandled();
	}
}

void WSelectList::scroll(bool isUp)
{
	if (myHasScrollbar())
	{
		int end = (int)myItems.size() * ITEM_H - myItemsRect().h();
		int delta = isUp ? -ITEM_H : ITEM_H;
		myScrollPos = min(max(myScrollPos + delta, 0), end);
	}
}

bool WSelectList::wasInteracted() const
{
	return (bool)myWasInteracted;
}

void WSelectList::setSelectedItem(uint index)
{
	if (mySelectedItem != index)
	{
		mySelectedItem = index;
		if (onChange) onChange();
	}
}

int WSelectList::selectedItem() const
{
	return mySelectedItem;
}

void WSelectList::onMeasure()
{
	myWidth = 48;
	myMinWidth = 48;
	myHeight = (int)myItems.size() * ITEM_H + 6;
	myMinHeight = myHeight;
}

void WSelectList::onArrange(Rect r)
{
	Widget::onArrange(r);
}

void WSelectList::draw(bool enabled)
{
	auto& misc = UiStyle::getMisc();
	auto& textbox = UiStyle::getTextBox();

	// Draw the list background box.
	if (myShowBackground)
		textbox.base.draw(myRect);

	Rect itemsRect = myItemsRect();
	Renderer::pushScissorRect(itemsRect);

	// Highlight the mouse over item, or the currently selected item.
	auto numItems = myItems.size();
	if (isMouseOver())
	{
		Vec2 mpos = Window::getMousePos();
		mySelectedItem = myHoverItem(mpos);
	}

	Rect singleItemRect = itemsRect.sliceT(ITEM_H).offsetY(-myScrollPos);
	if (mySelectedItem >= 0 && mySelectedItem < numItems)
		textbox.highlight.draw(singleItemRect.offsetY(mySelectedItem * ITEM_H));

	// Draw the item texts.
	Text::setStyle(UiText::regular, TextOptions::Markup | TextOptions::Ellipses);
	for (int i = 0; i < numItems; ++i)
	{
		auto itemRect = singleItemRect.offsetY(i * ITEM_H);
		if (itemsRect.overlaps(itemRect))
		{
			Text::format(TextAlign::MC, myItems[i].data());
			Text::draw(itemRect);
		}
	}

	Renderer::popScissorRect();
}

int WSelectList::myHoverItem(Vec2 pos)
{
	Rect r = myItemsRect();
	if (r.contains(pos))
	{
		int i = (pos.y - r.t + myScrollPos) / ITEM_H;
		if (i >= 0 && i < (int)myItems.size()) return i;
	}
	return -1;
}

bool WSelectList::myHasScrollbar() const
{
	return ((int)myItems.size() * ITEM_H) > myItemsRect().h();
}

Rect WSelectList::myItemsRect() const
{
	return myRect.shrink(3);
}

} // namespace AV
