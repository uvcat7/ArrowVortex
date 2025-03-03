#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

namespace Vortex {

// ================================================================================================
// WgSeperator

WgSeperator::~WgSeperator()
{
}

WgSeperator::WgSeperator(GuiContext* gui) : GuiWidget(gui)
{
	myHeight = 2;
}

void WgSeperator::onDraw()
{
	recti r = myRect;
	Draw::fill({r.x, r.y, r.w, r.h/2}, Color32(13));
	Draw::fill({r.x, r.y + r.h / 2, r.w, r.h / 2}, Color32(77));
}

// ================================================================================================
// WgLabel

WgLabel::~WgLabel()
{
}

WgLabel::WgLabel(GuiContext* gui) : GuiWidget(gui)
{
}

void WgLabel::onDraw()
{
	recti r = myRect;

	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	if(!isEnabled()) style.textColor = Color32(166);

	Text::arrange(Text::MC, style, text.get());
	Text::draw({r.x + 2, r.y, r.w - 4, r.h});
}

// ================================================================================================
// WgButton

WgButton::~WgButton()
{
}

WgButton::WgButton(GuiContext* gui)
	: GuiWidget(gui)
{
}

void WgButton::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();
			isDown.set(true);
			counter.set(counter.get() + 1);
			onPress.call();
		}
		evt.setHandled();
	}
}

void WgButton::onMouseRelease(MouseRelease& evt)
{
	stopCapturingMouse();
	isDown.set(false);
}

void WgButton::onDraw()
{
	auto& button = GuiDraw::getButton();

	// Draw the button graphic.
	button.base.draw(myRect);

	// Draw the button text.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	if(!isEnabled()) style.textColor = GuiDraw::getMisc().colDisabled;

	recti r = myRect;
	Renderer::pushScissorRect(Shrink(myRect, 2));
	Text::arrange(Text::MC, style, text.get());
	Text::draw({r.x + 2, r.y, r.w - 4, r.h});
	Renderer::popScissorRect();

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

// ================================================================================================
// WgCheckbox

WgCheckbox::~WgCheckbox()
{
}

WgCheckbox::WgCheckbox(GuiContext* gui)
	: GuiWidget(gui)
{
}

void WgCheckbox::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();
			value.set(!value.get());
			onChange.call();
		}
		evt.setHandled();
	}
}

void WgCheckbox::onMouseRelease(MouseRelease& evt)
{
	if(isCapturingMouse() && evt.button == Mouse::LMB)
	{
		stopCapturingMouse();
	}
}

void WgCheckbox::onDraw()
{
	auto& icons = GuiDraw::getIcons();
	auto& textbox = GuiDraw::getTextBox();
	auto& misc = GuiDraw::getMisc();

	// Draw the checkbox graphic.
	recti box = myBoxRect();
	bool checked = value.get();
	textbox.base.draw(box);
	if(checked) Draw::sprite(icons.check, {box.x + box.w / 2, box.y + box.h / 2});

	// Draw the description text.
	recti r = myRect;

	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	if(!isEnabled()) style.textColor = misc.colDisabled;

	Text::arrange(Text::ML, style, text.get());
	Text::draw({r.x + 22, r.y, r.w - 24, r.h});
}

recti WgCheckbox::myBoxRect() const
{
	recti r = myRect;
	return {r.x + 2, r.y + r.h / 2 - 8, 16, 16};
}


// ================================================================================================
// WgSlider

WgSlider::~WgSlider()
{
}

WgSlider::WgSlider(GuiContext* gui)
	: GuiWidget(gui)
{
	myBegin = 0.0;
	myEnd = 1.0;
}

void WgSlider::setRange(double begin, double end)
{
	myBegin = begin, myEnd = end;
}

void WgSlider::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();
			myDrag(evt.x, evt.y);
		}
		evt.setHandled();
	}
}

void WgSlider::onMouseRelease(MouseRelease& evt)
{
	if(evt.button == Mouse::LMB && isCapturingMouse())
	{
		myDrag(evt.x, evt.y);
		stopCapturingMouse();
	}
}

void WgSlider::onTick()
{
	GuiWidget::onTick();

	if(isCapturingMouse())
	{
		vec2i mpos = myGui->getMousePos();
		myDrag(mpos.x, mpos.y);
	}
}

void WgSlider::onDraw()
{
	recti r = myRect;

	auto& button = GuiDraw::getButton();

	// Draw the the entire bar graphic.
	recti bar = {r.x + 3, r.y + r.h / 2 - 1, r.w - 6, 1};
	Draw::fill(bar, Color32(0, 255));
	bar.y += 1;
	Draw::fill(bar, Color32(77, 255));

	// Draw the draggable button graphic.
	if(myBegin != myEnd)
	{
		int boxX = (int)((double)bar.w * (value.get() - myBegin) / (myEnd - myBegin));
		recti box = {bar.x + min(max(boxX, 0), bar.w) - 4, bar.y - 8, 8, 16};
		
		button.base.draw(box, 0);
		if(isCapturingMouse())
		{
			button.pressed.draw(box, 0);
		}
		else if(isMouseOver())
		{
			button.hover.draw(box, 0);
		}
	}
}

void WgSlider::myUpdateValue(double v)
{
	double prev = value.get();
	v = min(v, max(myBegin, myEnd));
	v = max(v, min(myBegin, myEnd));
	value.set(v);
	if(value.get() != prev) onChange.call();
}

void WgSlider::myDrag(int x, int y)
{
	recti r = myRect;
	r.w = max(r.w, 1);
	double val = myBegin + (myEnd - myBegin) * ((double)(x - r.x) / (double)r.w);
	myUpdateValue(val);
}

}; // namespace Vortex
