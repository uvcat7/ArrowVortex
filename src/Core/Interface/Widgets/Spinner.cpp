#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Draw.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Spinner.h>

namespace AV {

using namespace std;
using namespace Util;

WSpinner::~WSpinner()
{
}

WSpinner::WSpinner()
	: myIsUpActive(false)
	, myRepeatTimer(0.f)
	, myMin(INT_MIN)
	, myMax(INT_MAX)
	, myStep(1.0)
	, myValue(0.0)
	, myMinDecimalPlaces(0)
	, myMaxDecimalPlaces(6)
{
	myEdit = make_shared<WLineEdit>();
	myEdit->setNumerical(true);
	myEdit->setMaxLength(12);
	myEdit->onChange = std::bind(&WSpinner::myOnTextChanged, this);
	myEdit->hideBackground();

	myUpdateText();
}

void WSpinner::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			myEdit->deselect();
			startCapturingMouse();
			Rect r = myButtonRect();
			myIsUpActive = (input.pos.y < r.cy());
			double sign = myIsUpActive ? 1.0 : -1.0;
			setValue(myValue + myStep*sign);
			myRepeatTimer = 0.5f;
		}
		input.setHandled();
	}
}

void WSpinner::onMouseRelease(MouseRelease& input)
{
	if (input.button == MouseButton::LMB && isCapturingMouse())
		stopCapturingMouse();
}

UiElement* WSpinner::findMouseOver(int x, int y)
{
	Vec2 mpos(x, y);
	if (myButtonRect().contains(mpos))
	{
		return this;
	}
	return nullptr;
}

void WSpinner::onArrange(Rect r)
{
	Widget::onArrange(r);
	r.r -= 14;
	myEdit->arrange(r);
}

void WSpinner::tick(bool enabled)
{
	Vec2 mpos = Window::getMousePos();

	myEdit->tick(enabled);

	if (isMouseOver() || myEdit->isMouseOver())
	{
		setTooltip(getTooltip());
	}

	// Propagate settings to the text edits.
	myEdit->setEnabled(isEnabled());

	// Check if the user is pressing the increment/decrement button.
	if (isCapturingMouse())
	{
		myRepeatTimer -= (float)Window::getDeltaTime();
		if (myRepeatTimer <= 0.f)
		{
			double sign = myIsUpActive ? 1.0 : -1.0;
			setValue(myValue + myStep * sign);
			myRepeatTimer = 0.05f;
		}
	}
}

void WSpinner::draw(bool enabled)
{
	auto& button = UiStyle::getButton();
	auto& spinner = UiStyle::getSpinner();
	auto& misc = UiStyle::getMisc();
	auto& textbox = UiStyle::getTextBox();

	// Draw the background box graphic.
	textbox.base.draw(myRect.shrinkR(12));

	// Draw the number text.
	myEdit->draw(enabled);
	
	// Draw the buttons.
	Rect buttonRect = myButtonRect();
	Rect buttonRectT = buttonRect.sliceT(buttonRect.h() / 2);
	Rect buttonRectB = buttonRect.shrink(0, buttonRectT.h(), 0, 0);

	TileRect* tileT = &spinner.topBase;
	TileRect* tileB = &spinner.bottomBase;

	if (isCapturingMouse())
	{
		if (myIsUpActive)
			tileT = &spinner.topPress;
		else
			tileB = &spinner.bottomPress;
	}
	else if (isEnabled() && isMouseOver())
	{
		if (buttonRectT.contains(Window::getMousePos()))
			tileT = &spinner.topHover;
		else
			tileB = &spinner.bottomHover;
	}

	tileT->draw(buttonRectT, Color::White);
	tileB->draw(buttonRectB, Color::White);

	Color col = Color(isEnabled() ? 255 : 128, 255);
	
	misc.arrow.draw(buttonRectT.center(), col, Sprite::Orientation::Rot270);
	misc.arrow.draw(buttonRectB.center(), col, Sprite::Orientation::Rot90);
}

void WSpinner::setRange(double min, double max)
{
	myMin = min;
	myMax = max;
}

void WSpinner::setStep(double step)
{
	myStep = step;
}

void WSpinner::setPrecision(int minDecimalPlaces, int maxDecimalPlaces)
{
	myMinDecimalPlaces = minDecimalPlaces;
	myMaxDecimalPlaces = maxDecimalPlaces;
	myUpdateText();
}

void WSpinner::setValue(double value)
{
	value = clamp(value, myMin, myMax);
	if (myValue != value)
	{
		myValue = value;
		myUpdateText();
		if (onChange) onChange();
	}
}

double WSpinner::value() const
{
	return myValue;
}

int WSpinner::ivalue() const
{
	return lround(myValue);
}

void WSpinner::myUpdateText()
{
	string text;
	String::appendDouble(text, myValue, myMinDecimalPlaces, myMaxDecimalPlaces);
	myEdit->setText(text);
}

void WSpinner::myOnTextChanged()
{
	double v = 0.0;
	if (String::evaluateExpression(myEdit->text().data(), v))
	{
		myValue = v;
	}
	else
	{
		myUpdateText();
	}
}

Rect WSpinner::myButtonRect()
{
	return myRect.sliceR(14);
}

} // namespace AV
