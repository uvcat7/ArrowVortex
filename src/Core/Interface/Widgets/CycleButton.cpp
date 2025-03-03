#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Precomp.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/CycleButton.h>

namespace AV {

WCycleButton::~WCycleButton()
{
	clearItems();
}

WCycleButton::WCycleButton()
	: mySelectedItem(0)
{
}

void WCycleButton::addItem(stringref text)
{
	myItems.push_back(text);
}

void WCycleButton::clearItems()
{
	myItems.clear();
}

void WCycleButton::setSelectedItem(int item)
{
	if (mySelectedItem != item)
	{
		mySelectedItem = item;
		if (onChange) onChange();
	}
}

int WCycleButton::selectedItem() const
{
	return mySelectedItem;
}

void WCycleButton::onMousePress(MousePress& input)
{
	auto numItems = (int)myItems.size();
	if (isMouseOver())
	{
		if (numItems > 1 && isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			stopCapturingText();
			startCapturingMouse();
			if (input.pos.x > myRect.cx())
			{
				setSelectedItem((mySelectedItem + 1) % numItems);
			}
			else
			{
				setSelectedItem((mySelectedItem + numItems - 1) % numItems);
			}
		}
		input.setHandled();
	}
}

void WCycleButton::onMouseRelease(MouseRelease& input)
{
	if (isCapturingMouse() && input.button == MouseButton::LMB)
	{
		stopCapturingMouse();
	}
}

void WCycleButton::draw(bool enabled)
{
	auto& misc = UiStyle::getMisc();
	auto& button = UiStyle::getButton();

	auto numItems = myItems.size();

	// Draw the button graphic.
	Rect r = myRect;
	if (isCapturingMouse())
	{
		button.press.draw(myRect);
	}
	else if (isMouseOver())
	{
		button.hover.draw(myRect);
	}
	else
	{
		button.base.draw(r);
	}

	// Draw the arrow graphics.
	int mouseX = Window::getMousePos().x - myRect.cx();
	if (!isMouseOver()) mouseX = 0;

	Color colL = (isEnabled() && mouseX < 0) ? Color::White : misc.colDisabled;
	misc.arrow.draw(Vec2(r.l + 10, r.cy()), colL, Sprite::Orientation::FlipH);

	Color colR = (isEnabled() && mouseX > 0) ? Color::White : misc.colDisabled;
	misc.arrow.draw(Vec2(r.r - 10, r.cy()), colR);

	// Draw the button text.
	if (mySelectedItem >= 0 && mySelectedItem < numItems)
	{
		auto textStyle = UiText::regular;
		textStyle.flags = TextOptions::Markup | TextOptions::Ellipses;
		if (!isEnabled()) textStyle.textColor = misc.colDisabled;

		Renderer::pushScissorRect(myRect.shrink(2));
		Text::setStyle(textStyle);
		Text::format(TextAlign::MC, myItems[mySelectedItem].data());
		Text::draw(r.shrink(2,0));
		Renderer::popScissorRect();
	}
}

} // namespace AV
