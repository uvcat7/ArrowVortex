#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/Flag.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Button.h>

namespace AV {

WButton::~WButton()
{
}

WButton::WButton()
{
}

WButton::WButton(const char* text)
{
	myText = text;
}

void WButton::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();
			if (onPress) onPress();
		}
		input.setHandled();
	}
}

void WButton::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
}

void WButton::draw(bool enabled)
{
	auto& button = UiStyle::getButton();

	// Draw the button graphic.
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
		button.base.draw(myRect);
	}

	// Draw the button text.
	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup | TextOptions::Ellipses;
	if (!isEnabled()) textStyle.textColor = UiStyle::getMisc().colDisabled;

	Rect r = myRect.shrink(2);
	Renderer::pushScissorRect(r);
	Text::setStyle(textStyle);
	Text::format(TextAlign::MC, myText);
	Text::draw(r);
	Renderer::popScissorRect();
}

} // namespace AV
