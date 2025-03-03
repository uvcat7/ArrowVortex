#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Colorf.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/ColorPicker.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Expanded color picker.

struct WColorPicker::Expanded
{
	void startDrag(Vec2 pos);
	void endDrag();
	void tick(Rect r);
	void draw();

	Rect getAr() const { return myRect.sliceR(18).shrink(0,4,4,4); }
	Rect getHr() const { return myRect.sliceR(36).shrink(0,22,4,4); }
	Rect getSr() const { return myRect.shrink(4,4,40,4); }

	Rect myRect;
	int myDrag;
	Colorf::HSVA myCol;
};

void WColorPicker::Expanded::startDrag(Vec2 pos)
{
	     if (getSr().contains(pos)) myDrag = 1;
	else if (getHr().contains(pos)) myDrag = 2;
	else if (getAr().contains(pos)) myDrag = 3;
}

void WColorPicker::Expanded::endDrag()
{
	myDrag = 0;
}

void WColorPicker::Expanded::tick(Rect r)
{
	myRect = {r.r + 2, r.cy() - 80, 196, 160};

	Size2 viewSize = Window::getSize();

	myRect.l = clamp(myRect.l, 0, viewSize.w - myRect.w());
	myRect.t = clamp(myRect.t, 0, viewSize.h - myRect.h());

	Vec2 mpos = Window::getMousePos();

	if (myDrag == 1)
	{
		Rect r = getSr();
		myCol.saturation = clamp(float(mpos.x - r.l) / (float)r.w(), 0.0f, 1.0f);
		myCol.value = clamp(1.0f - float(mpos.y - r.t) / (float)r.h(), 0.0f, 1.0f);
	}
	else if (myDrag == 2)
	{
		Rect r = getHr();
		myCol.hue = clamp(float(mpos.y - r.t) / (float)r.h(), 0.0f, 1.0f);
	}
	else if (myDrag == 3)
	{
		Rect r = getAr();
		myCol.alpha = clamp(1.0f - float(mpos.y - r.t) / (float)r.h(), 0.0f, 1.0f);
	}
}

void WColorPicker::Expanded::draw()
{
	// Draw frame.
	auto& dlg = UiStyle::getDialog();
	dlg.outerFrame.draw(myRect);
	// TODO: fix
	// dlg.vshape.draw(myRect.ml(), Color::White, Sprite::ROT_90);

	Rect ar = getAr();
	Color t = Colorf(myCol, 1.0f).toU8();
	Color b = Colorf(myCol, 0.0f).toU8();
	UiStyle::checkerboard(ar, Color::White);
	Draw::fill(ar, t, t, b, b, false);

	Rect hr = getHr();
	for (int i = 0, y = 0; i < 6; ++i)
	{
		int by = hr.h() * (i + 1) / 6;
		t = Colorf(Colorf::HSVA{float(i + 0) / 6.0f, 1.0f, 1.0f, 1.0f}).toU8();
		b = Colorf(Colorf::HSVA{float(i + 1) / 6.0f, 1.0f, 1.0f, 1.0f}).toU8();
		Draw::fill(Rect(hr.l, hr.t + y, hr.r, by - y), t, t, b, b, true);
		y = by;
	}

	Rect sr = getSr();
	Color tl = Colorf(Colorf::HSVA{myCol.hue, 0, 1, 1}).toU8();
	Color tr = Colorf(Colorf::HSVA{myCol.hue, 1, 1, 1}).toU8();
	Color bl = Colorf(Colorf::HSVA{myCol.hue, 0, 0, 1}).toU8();
	Color br = Colorf(Colorf::HSVA{myCol.hue, 1, 0, 1}).toU8();
	Draw::fill(sr, tl, tr, bl, br, true);

	int sx = sr.l + int(sr.w() * myCol.saturation);
	int sy = sr.t + int(sr.h() * (1.0f - myCol.value));
	int hy = hr.t + int(hr.h() * myCol.hue);
	int ay = ar.t + int(ar.h() * (1.0f - myCol.alpha));

	Draw::outline(Rect(sx - 2, sy - 2, 5, 5), Color::Black);
	Draw::outline(Rect(sx - 1, sy - 1, 3, 3), Color::White);

	Draw::outline(Rect(sr.l - 2, hy - 2, hr.r + 4, 5), Color::Black);
	Draw::outline(Rect(sr.l - 1, hy - 1, hr.r + 2, 3), Color::White);

	Draw::outline(Rect(ar.l - 2, ay - 2, ar.r + 4, 5), Color::Black);
	Draw::outline(Rect(ar.l - 1, ay - 1, ar.r + 2, 3), Color::White);
}

// =====================================================================================================================
// Color picker.

WColorPicker::~WColorPicker()
{
	if (myExpanded) delete myExpanded;
}

WColorPicker::WColorPicker()
	: myExpanded(nullptr)
{
}

void WColorPicker::onMousePress(MousePress& input)
{
	if (myExpanded)
	{
		if (myExpanded->myRect.contains(input.pos) && input.button == MouseButton::LMB)
		{
			myExpanded->startDrag(input.pos);
			startCapturingMouse();
			input.setHandled();
		}
		else
		{
			Util::reset(myExpanded);
		}
	}
	else if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();

			// TODO: fix
			// startCapturingFocus();

			myExpanded = new Expanded;
			myExpanded->tick(myRect);
		}
		input.setHandled();
	}
}

void WColorPicker::onMouseRelease(MouseRelease& input)
{
	if (myExpanded) myExpanded->endDrag();
	stopCapturingMouse();
}

UiElement* WColorPicker::findMouseOver(int x, int y)
{
	Vec2 mpos(x, y);
	if (myExpanded)
	{
		if (myExpanded->myRect.contains(mpos))
		{
			return this;
		}
	}
	return this;
}

void WColorPicker::tick(bool enabled)
{
	if (myExpanded)
	{
		myExpanded->myCol = myValue.toHSVA();

		if (myExpanded->myDrag)
		{
			myExpanded->tick(myRect);
			setValue(Colorf(myExpanded->myCol));
		}
	}
}

void WColorPicker::draw(bool enabled)
{
	Draw::fill(myRect, Color::Black);
	Rect r = myRect.shrink(1);

	UiStyle::checkerboard(r, Color::White);

	auto hsva = myValue.toHSVA();
	hsva.value = min(1.0f, hsva.value * 0.75f + isMouseOver() * (hsva.value * 0.25f + 0.25f));
	Draw::fill(r, Colorf(hsva).toU8());

	r = r.shrink(1);
	Draw::fill(r, myValue.toU8());

	if (myExpanded) myExpanded->draw();
}

void WColorPicker::setValue(Colorf value)
{
	if (myValue != value)
	{
		myValue = value;
		if (onChange) onChange();
	}
}

Colorf WColorPicker::value() const
{
	return myValue;
}

} // namespace AV
