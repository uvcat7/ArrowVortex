#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Droplist.h>

namespace AV {

using namespace std;
using namespace Util;

/*enum class DroplistFlags
{
	IsAnimating = WF_FIRST_FREE_BIT << 0,
};*/

WDroplist::~WDroplist()
{
	clearItems();
	myCloseList();
}

WDroplist::WDroplist()
	: mySelectedItem(0)
{
}

void WDroplist::onMousePress(MousePress& input)
{
	if (myList)
	{
		Rect r = myList->getRect();
		if (input.button == MouseButton::RMB || !r.contains(input.pos))
		{
			myCloseList();
			input.setHandled();
		}
	}
	else if (isMouseOver())
	{
		int numItems = (int)myItems.size();
		if (isEnabled() && numItems && input.button == MouseButton::LMB && input.unhandled())
		{
			myList = make_shared<WSelectList>();
			myList->hideBackground();

			for (int i = 0; i < numItems; ++i)
				myList->addItem(myItems[i].data());
			myList->setSelectedItem(mySelectedItem);

			onArrange(myRect);

			startCapturingMouse();

			// TODO: fix
			// startCapturingFocus();
		}
		input.setHandled();
	}
}

void WDroplist::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
}

void WDroplist::onMouseScroll(MouseScroll& input)
{
	if (myList && input.unhandled())
	{
		myList->scroll(input.isUp);
		input.setHandled();
	}
}

void WDroplist::onMeasure()
{
	if (myList) myList->onMeasure();
}

void WDroplist::onArrange(Rect r)
{
	Widget::onArrange(r);
	if (myList)
	{
		int listH = min(myList->getHeight(), 256);
		myList->onArrange(myRect.offsetY(myRect.h()).sliceT(listH));
	}
}

void WDroplist::tick(bool enabled)
{
	if (myList)
	{
		myList->tick(enabled && myIsEnabled);
		if (myList->wasInteracted())
		{
			setSelectedItem(myList->selectedItem());
			myCloseList();
		}
	}
}

void WDroplist::draw(bool enabled)
{
	auto& button  = UiStyle::getButton();
	auto& misc    = UiStyle::getMisc();
	auto& textbox = UiStyle::getTextBox();

	auto numItems = myItems.size();

	// Draw the list of items.
	if (myList)
	{
		int height = myList->getRect().h() + myRect.h() / 2;
		Rect r = myRect.offsetY(myRect.h() / 2).sliceT(height);

		// List background.
		textbox.framed.draw(r);

		// List.
		myList->draw(enabled && myIsEnabled);
	}

	// Draw the button graphic.
	Rect r = myRect;
	if (isCapturingMouse() || myList)
	{
		button.press.draw(myRect);
	}
	else if (isMouseOver())
	{
		button.hover.draw(myRect);
	}
	else
	{
		button.base.draw(myRect);
	}

	// Draw downward arrow.
	if (isEnabled())
	{
		misc.arrow.draw(Vec2(r.r - 10, r.cy()), Color::White, Sprite::Orientation::Rot90);
	}

	// Draw the selected item text.
	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup | TextOptions::Ellipses;
	if (!isEnabled())
		textStyle.textColor = misc.colDisabled;

	if (mySelectedItem >= 0 && mySelectedItem < numItems)
	{
		Text::setStyle(textStyle);
		Text::format(TextAlign::MC, myRect.w() - 18, myItems[mySelectedItem].data());
		Text::draw(r.shrink(6,0,12,0));
	}	
}

void WDroplist::clearItems()
{
	myItems.clear();
}

void WDroplist::addItem(stringref text)
{
	myItems.push_back(text);
}

void WDroplist::setSelectedItem(int index)
{
	if (mySelectedItem != index)
	{
		mySelectedItem = index;
		if (onChange) onChange();
	}
}

int WDroplist::selectedItem() const
{
	return mySelectedItem;
}

void WDroplist::myCloseList()
{
	if (myList)
	{
		myList = nullptr;
	}
}

} // namespace AV
