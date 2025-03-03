#include <Core/Widgets.h>
#include <Core/Gui.h>

#include <math.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/GuiDraw.h>
#include <Core/Shader.h>
#include <Core/Widgets.h>

namespace Vortex {

// ================================================================================================
// Utility functions.

struct ColorHSV { float h, s, v, a; };

ColorHSV RGBtoHSV(colorf rgb, float a)
{
	float r = rgb.r, g = rgb.g, b = rgb.b, h, s, v;
	float cmax = max(max(r, g), b);
	float cmin = min(min(r, g), b);
	float delta = cmax - cmin;
	if(delta > 0)
	{
		if(cmax == r)
		{
			h = (1.0f / 6.0f) * (fmod(((g - b) / delta), 6));
		}
		else if(cmax == g)
		{
			h = (1.0f / 6.0f) * (((b - r) / delta) + 2);
		}
		else if(cmax == b)
		{
			h = (1.0f / 6.0f) * (((r - g) / delta) + 4);
		}

		if(cmax > 0)
		{
			s = delta / cmax;
		}
		else
		{
			s = 0;
		}

		v = cmax;
	}
	else
	{
		h = 0;
		s = 0;
		v = cmax;
	}

	if(h < 0)
	{
		h = 1.0f + h;
	}

	return {h, s, v, a};
}

colorf HSVtoRGB(ColorHSV hsv, float a)
{
	float h = hsv.h, s = hsv.s, v = hsv.v, r, g, b;
	float c = v * s; // Chroma
	float hprime = fmod(h * 6.0f, 6);
	float x = c * (1 - fabs(fmod(hprime, 2) - 1));
	float m = v - c;

	if(0 <= hprime && hprime < 1)
	{
		r = c;
		g = x;
		b = 0;
	}
	else if(1 <= hprime && hprime < 2)
	{
		r = x;
		g = c;
		b = 0;
	}
	else if(2 <= hprime && hprime < 3)
	{
		r = 0;
		g = c;
		b = x;
	}
	else if(3 <= hprime && hprime < 4)
	{
		r = 0;
		g = x;
		b = c;
	}
	else if(4 <= hprime && hprime < 5)
	{
		r = x;
		g = 0;
		b = c;
	}
	else if(5 <= hprime && hprime < 6)
	{
		r = c;
		g = 0;
		b = x;
	}
	else
	{
		r = 0;
		g = 0;
		b = 0;
	}

	r += m;
	g += m;
	b += m;

	return {r, g, b, a};
}

// ================================================================================================
// Expanded color picker.

struct WgColorPicker::Expanded
{
	void startDrag(int x, int y);
	void endDrag();
	void tick(recti r, GuiContext* gui);
	void draw();

	recti getAr() const { return {myRect.x + myRect.w - 18, myRect.y + 4, 14, myRect.h - 8}; }
	recti getHr() const { return {myRect.x + myRect.w - 36, myRect.y + 4, 14, myRect.h - 8}; }
	recti getSr() const { return {myRect.x + 4, myRect.y + 4, myRect.w - 44, myRect.h - 8}; }

	recti myRect;
	int myDrag;
	ColorHSV myCol;
};

void WgColorPicker::Expanded::startDrag(int x, int y)
{
	     if(IsInside(getSr(), x, y)) myDrag = 1;
	else if(IsInside(getHr(), x, y)) myDrag = 2;
	else if(IsInside(getAr(), x, y)) myDrag = 3;
}

void WgColorPicker::Expanded::endDrag()
{
	myDrag = 0;
}

void WgColorPicker::Expanded::tick(recti r, GuiContext* gui)
{
	myRect = {r.x + r.w + 2, r.y + r.h / 2 - 80, 196, 160};

	recti view = gui->getView();

	myRect.x = clamp(myRect.x, view.x, view.x + view.w - myRect.w);
	myRect.y = clamp(myRect.y, view.y, view.y + view.h - myRect.h);

	vec2i mpos = gui->getMousePos();

	if(myDrag == 1)
	{
		recti r = getSr();
		myCol.s = clamp((float)(mpos.x - r.x) / (float)r.w, 0.0f, 1.0f);
		myCol.v = clamp(1.0f - (float)(mpos.y - r.y) / (float)r.h, 0.0f, 1.0f);
	}
	else if(myDrag == 2)
	{
		recti r = getHr();
		myCol.h = clamp((float)(mpos.y - r.y) / (float)r.h, 0.0f, 1.0f);
	}
	else if(myDrag == 3)
	{
		recti r = getAr();
		myCol.a = clamp(1.0f - (float)(mpos.y - r.y) / (float)r.h, 0.0f, 1.0f);
	}
}

void WgColorPicker::Expanded::draw()
{
	auto& dlg = GuiDraw::getDialog();
	dlg.frame.draw(myRect, 0);

	Draw::fill({myRect.x - 8, myRect.y + myRect.h / 2 - 8, 16, 16}, Colors::white, dlg.vshape.handle());

	recti ar = getAr();
	color32 t = ToColor32(HSVtoRGB(myCol, 1.0f));
	color32 b = ToColor32(HSVtoRGB(myCol, 0.0f));
	GuiDraw::checkerboard(ar, Colors::white);
	Draw::fill(ar, t, t, b, b, false);

	recti hr = getHr();
	for(int i = 0, y = 0; i < 6; ++i)
	{
		int by = hr.h * (i + 1) / 6;
		t = ToColor32({(float)(i + 0) / 6.0f, 1.0f, 1.0f, 1.0f});
		b = ToColor32({(float)(i + 1) / 6.0f, 1.0f, 1.0f, 1.0f});
		Draw::fill({hr.x, hr.y + y, hr.w, by - y}, t, t, b, b, true);
		y = by;
	}

	recti sr = getSr();
	color32 tl = ToColor32({myCol.h, 0, 1, 1}), tr = ToColor32({myCol.h, 1, 1, 1});
	color32 bl = ToColor32({myCol.h, 0, 0, 1}), br = ToColor32({myCol.h, 1, 0, 1});
	Draw::fill(sr, tl, tr, bl, br, true);

	int sx = sr.x + (int)(sr.w * myCol.s);
	int sy = sr.y + (int)(sr.h * (1.0f - myCol.v));
	int hy = hr.y + (int)(hr.h * myCol.h);
	int ay = ar.y + (int)(ar.h * (1.0f - myCol.a));

	Draw::outline({sx - 2, sy - 2, 5, 5}, Colors::black);
	Draw::outline({sx - 1, sy - 1, 3, 3}, Colors::white);

	Draw::outline({hr.x - 2, hy - 2, hr.w + 4, 5}, Colors::black);
	Draw::outline({hr.x - 1, hy - 1, hr.w + 2, 3}, Colors::white);

	Draw::outline({ar.x - 2, ay - 2, ar.w + 4, 5}, Colors::black);
	Draw::outline({ar.x - 1, ay - 1, ar.w + 2, 3}, Colors::white);
}

// ================================================================================================
// Expanded color picker.

WgColorPicker::~WgColorPicker()
{
	if(myExpanded) delete myExpanded;
}

WgColorPicker::WgColorPicker(GuiContext* gui)
	: GuiWidget(gui)
	, myExpanded(nullptr)
{
}

void WgColorPicker::onMousePress(MousePress& evt)
{
	if(myExpanded)
	{
		if(IsInside(myExpanded->myRect, evt.x, evt.y) && evt.button == Mouse::LMB)
		{
			myExpanded->startDrag(evt.x, evt.y);
			startCapturingMouse();
			evt.setHandled();
		}
		else
		{
			delete myExpanded;
			myExpanded = nullptr;
			stopCapturingFocus();
		}
	}
	else if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();
			startCapturingFocus();

			myExpanded = new Expanded;
			myExpanded->tick(myRect, myGui);
		}
		evt.setHandled();
	}
}

void WgColorPicker::onMouseRelease(MouseRelease& evt)
{
	if(myExpanded) myExpanded->endDrag();
	stopCapturingMouse();
}

void WgColorPicker::onTick()
{
	GuiWidget::onTick();
	if(myExpanded)
	{
		vec2i mpos = myGui->getMousePos();
		if(IsInside(myExpanded->myRect, mpos.x, mpos.y))
		{
			captureMouseOver();
		}

		colorf rgb = {(float)red.get(), (float)green.get(), (float)blue.get(), (float)alpha.get()};
		myExpanded->myCol = RGBtoHSV(rgb, rgb.a);

		if(myExpanded->myDrag)
		{
			myExpanded->tick(myRect, myGui);
			rgb = HSVtoRGB(myExpanded->myCol, myExpanded->myCol.a);

			red.set(rgb.r);
			green.set(rgb.g);
			blue.set(rgb.b);
			alpha.set(rgb.a);

			onChange.call();
		}
	}
}

void WgColorPicker::onDraw()
{
	colorf rgb = {(float)red.get(), (float)green.get(), (float)blue.get(), (float)alpha.get()};

	Draw::fill(myRect, Colors::black);
	recti r = Shrink(myRect, 1);

	GuiDraw::checkerboard(r, Colors::white);

	ColorHSV hsv = RGBtoHSV(rgb, rgb.a);
	hsv.v = min(1.0f, hsv.v * 0.75f + isMouseOver() * (hsv.v * 0.25f + 0.25f));
	Draw::fill(r, ToColor32(HSVtoRGB(hsv, rgb.a)));

	r = Shrink(r, 1);
	Draw::fill(r, ToColor32((float)red.get(), (float)green.get(), (float)blue.get(), (float)alpha.get()));

	if(myExpanded) myExpanded->draw();
}

}; // namespace Vortex
