#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/Slider.h>

namespace AV {

using namespace std;
using namespace Util;

WSlider::~WSlider()
{
}

WSlider::WSlider()
	: myBegin(0.0)
	, myEnd(1.0)
	, myValue(0.0)
{
}

void WSlider::setRange(double begin, double end)
{
	myBegin = begin;
	myEnd = end;
}

void WSlider::setValue(double value)
{
	value = min(value, max(myBegin, myEnd));
	value = max(value, min(myBegin, myEnd));
	if (myValue != value)
	{
		myValue = value;
		if (onChange) onChange();
	}
}

double WSlider::value() const
{
	return myValue;
}

void WSlider::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();
			myDrag(input.pos);
		}
		input.setHandled();
	}
}

void WSlider::onMouseRelease(MouseRelease& input)
{
	if (input.button == MouseButton::LMB && isCapturingMouse())
	{
		myDrag(input.pos);
		stopCapturingMouse();
	}
}

void WSlider::tick(bool enabled)
{
	if (isCapturingMouse())
		myDrag(Window::getMousePos());
}

void WSlider::draw(bool enabled)
{
	Rect r = myRect;

	auto& button = UiStyle::getButton();

	// Draw the the entire bar graphic.
	Rect bar = r.shrinkX(3).sliceH(1);
	Draw::fill(bar, Color(0, 255));
	Draw::fill(bar.offsetY(1), Color(77, 255));

	// Draw the draggable button graphic.
	if (myBegin != myEnd)
	{
		int boxX = clamp(int((double)bar.w() * (myValue - myBegin) / (myEnd - myBegin)), 0, bar.w());
		Rect box = bar.shrink(4, 8).offsetX(boxX);
		
		button.base.draw(box);
		if (isCapturingMouse())
		{
			button.press.draw(box);
		}
		else if (isMouseOver())
		{
			button.hover.draw(box);
		}
	}
}

void WSlider::myDrag(Vec2 pos)
{
	double t = (double)(pos.x - myRect.l) / (double)max(myRect.w(), 1);
	setValue(lerp(myBegin, myEnd, t));
}

} // namespace AV
