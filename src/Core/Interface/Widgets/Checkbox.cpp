#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Checkbox.h>

namespace AV {

WCheckbox::~WCheckbox()
{
}

WCheckbox::WCheckbox()
	: myValue(false)
{
}

WCheckbox::WCheckbox(const char* text)
	: myValue(false)
{
	myText = text;
}

void WCheckbox::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();
			setValue(!myValue);
		}
		input.setHandled();
	}
}

void WCheckbox::onMouseRelease(MouseRelease& input)
{
	if (isCapturingMouse() && input.button == MouseButton::LMB)
	{
		stopCapturingMouse();
	}
}

void WCheckbox::draw(bool enabled)
{
	auto& textbox = UiStyle::getTextBox();
	auto& misc = UiStyle::getMisc();

	// Draw the checkbox graphic.
	Rect box = myBoxRect();
	textbox.base.draw(box);
	if (myValue)
		misc.checkmark.draw(box);

	// Draw the description text.
	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup | TextOptions::Ellipses;
	if (!isEnabled()) textStyle.textColor = misc.colDisabled;

	Text::setStyle(textStyle);
	Text::format(TextAlign::ML, myText);
	Text::draw(myRect.shrink(22, 0, 2, 0));
}

void WCheckbox::setValue(bool value)
{
	if (myValue != value)
	{
		myValue = value;
		if (onChange) onChange();
	}
}

bool WCheckbox::value() const
{
	return myValue;
}

Rect WCheckbox::myBoxRect() const
{
	return myRect.offsetX(2).sliceL(16).sliceH(16);
}

} // namespace AV
