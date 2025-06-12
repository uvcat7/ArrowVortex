#include <Core/Widgets.h>
#include <Core/Gui.h>

#include <math.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/GuiDraw.h>
#include <Core/Shader.h>

namespace Vortex {

// ===================================================================================
// WgLineEdit

enum LineEditDragType
{
	DT_NOT_DRAGGING = 0,
	DT_INITIAL_DRAG,
	DT_REGULAR_DRAG,
};

WgLineEdit::~WgLineEdit()
{
}

WgLineEdit::WgLineEdit(GuiContext* gui)
	: GuiWidget(gui)
	, myShowBackground(1)
{
	myMaxLength = 1 << 24;
	myBlinkTime = 0.f;
	myScroll = 0.f;
	myDrag = DT_NOT_DRAGGING;
	myIsNumerical = false;
	myIsEditable = true;
	myForceScrollUpdate = false;
}

void WgLineEdit::deselect()
{
	if(isCapturingText())
	{
		stopCapturingText();
		myDrag = DT_NOT_DRAGGING;
		myScroll = 0.f;
	}
}

void WgLineEdit::onKeyPress(KeyPress& evt)
{
	if(!isCapturingText()) return;

	evt.handled = true;

	if(evt.key == Key::RETURN) stopCapturingText();

	myDrag = DT_NOT_DRAGGING;
	myBlinkTime = 0.f;

	Key::Code key = evt.key;
	if(evt.keyflags == Keyflag::CTRL)
	{
		if(key == Key::X || key == Key::C)
		{
			if(myCursor.x == myCursor.y)
			{
				GuiMain::setClipboardText(String());
			}
			else
			{
				int a = myCursor.x, b = myCursor.y;
				if(a > b) swapValues(a, b);
				String substring(myEditStr.begin() + a, b - a);
				GuiMain::setClipboardText(substring.str());
				if(key == Key::X) myDeleteSelection();
			}
			evt.handled = true;
		}
		else if(key == Key::V)
		{
			String clip = GuiMain::getClipboardText();
			TextInput evt = {clip.str(), false};
			onTextInput(evt);
			evt.handled = true;
		}
		else if(key == Key::A)
		{
			myCursor.x = 0;
			myCursor.y = myEditStr.len();
			evt.handled = true;
		}
	}
	else if(evt.keyflags == Keyflag::SHIFT)
	{
		int& cy = myCursor.y;
		if(key == Key::HOME) cy = 0;
		if(key == Key::END) cy = myEditStr.len();
		if(key == Key::LEFT) cy = Str::prevChar(myEditStr, cy);
		if(key == Key::RIGHT) cy = Str::nextChar(myEditStr, cy);
	}
	else
	{
		int& cy = myCursor.y, &cx = myCursor.x;
		if(key == Key::HOME) cx = cy = 0;
		if(key == Key::END) cx = cy = myEditStr.len();
		if(cx != cy)
		{
			if(key == Key::LEFT) cx = cy = min(cx, cy); 
			if(key == Key::RIGHT) cx = cy = max(cx, cy);
		}
		else
		{
			if(key == Key::LEFT) cx = cy = Str::prevChar(myEditStr, cy);
			if(key == Key::RIGHT) cx = cy = Str::nextChar(myEditStr, cy);
		}
	}

	if(key == Key::DELETE || key == Key::BACKSPACE)
	{
		if(myCursor.x == myCursor.y) 
		{
			if(key == Key::BACKSPACE)
			{
				myCursor.y = Str::prevChar(myEditStr, myCursor.y);
			}
			else
			{
				myCursor.y = Str::nextChar(myEditStr, myCursor.y);
			}
		}
		myDeleteSelection();
	}

	int len = myEditStr.len();
	myCursor.x = clamp(myCursor.x, 0, len);
	myCursor.y = clamp(myCursor.y, 0, len);
}

void WgLineEdit::onKeyRelease(KeyRelease& evt)
{
	if (!isCapturingText()) return;
	evt.handled = true;
}

void WgLineEdit::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			if(isCapturingText() && evt.doubleClick)
			{
				myCursor.x = 0;
				myCursor.y = myEditStr.len();
			}
			else
			{
				if(isCapturingText())
				{
					myDrag = DT_REGULAR_DRAG;
				}
				else
				{
					myEditStr = text.get();
					myDrag = DT_INITIAL_DRAG;
					startCapturingText();
				}

				vec2i t = myTextPos();
				Text::arrange(Text::ML, mySettings, myEditStr.str());
				int charIndex = Text::getCharIndex(vec2i{t.x, t.y}, {evt.x, evt.y});
				myCursor.x = myCursor.y = charIndex;
			}

			startCapturingMouse();
			myBlinkTime = 0.f;
			evt.setHandled();
		}
	}
	else deselect();
}

void WgLineEdit::onMouseRelease(MouseRelease& evt)
{
	if(evt.button == Mouse::LMB && isCapturingMouse())
	{
		stopCapturingMouse();
	}
	if(myDrag != DT_NOT_DRAGGING)
	{
		if(myIsNumerical && myDrag == DT_INITIAL_DRAG && myCursor.x == myCursor.y)
		{
			myCursor.x = 0;
			myCursor.y = myEditStr.len();
		}
		myDrag = DT_NOT_DRAGGING;
	}
}

void WgLineEdit::onTextInput(TextInput& evt)
{
	if(isCapturingText() && myIsEditable && !evt.handled)
	{
		myDeleteSelection();
		if((int)myEditStr.len() < myMaxLength)
		{
			String input(evt.text);
			
			int maxInputLen = myMaxLength - myEditStr.len();

			int i = input.len();
			while(i > maxInputLen) i = Str::prevChar(input, i);
			if(i < (int)input.len()) Str::erase(input, i);

			Str::insert(myEditStr, myCursor.y, input);

			myCursor.y += input.len();
			myCursor.x = myCursor.y;
		}
		myForceScrollUpdate = true;
	}
}

void WgLineEdit::onTextCaptureLost()
{
	if(myEditStr != text.get())
	{
		text.set(myEditStr);
		onChange.call();
	}
	myCursor.x = myCursor.y = 0;
	myScroll = 0.f;
	myEditStr.release();
}

void WgLineEdit::onTick()
{
	GuiWidget::onTick();

	if(isEnabled() && (isMouseOver() || isCapturingMouse()))
	{
		GuiMain::setCursorIcon(Cursor::IBEAM);
	}

	if(!isCapturingText()) return;

	Text::arrange(Text::ML, mySettings, myEditStr.str());

	// Update cursor position.
	vec2i tp = myTextPos();
	if(myDrag != DT_NOT_DRAGGING)
	{
		vec2i mp = myGui->getMousePos();
		myCursor.y = Text::getCharIndex(vec2i{tp.x, tp.y}, {mp.x, tp.y});
		myBlinkTime = 0.f;
	}
	myCursor.x = min(max(myCursor.x, 0), (int)myEditStr.len());
	myCursor.y = min(max(myCursor.y, 0), (int)myEditStr.len());

	// Update text offset
	
	float dt = myGui->getDeltaTime();
	float barW = (float)(myRect.w - 12);
	float textW = (float)Text::getSize().x;
	float cursorX = (float)Text::getCursorPos(vec2i{0, 0}, myCursor.y).x;
	float target = min(max(myScroll, cursorX - barW + 12), cursorX - 12);
	target = max(0.f, min(target, textW - barW));

	float delta = max((float)fabs(myScroll - target) * 10.f * dt, dt * 256.f);
	float smooth = (myScroll < target) ? min(myScroll + delta, target) : max(myScroll - delta, target);
	myScroll = myForceScrollUpdate ? target : smooth;
	myForceScrollUpdate = false;

	// Update blink time
	myBlinkTime = fmod(myBlinkTime + dt, 1.f);
}

void WgLineEdit::onDraw()
{
	recti r = myRect;
	vec2i tp = myTextPos();
	bool active = isCapturingText();

	const char* str = active ? myEditStr.str() : text.get();

	// Draw the background box graphic.
	if(myShowBackground)
	{
		auto& textbox = GuiDraw::getTextBox();
		textbox.base.draw(r);
		if(isCapturingText())
		{
			textbox.active.draw(r);
		}
		else if(isMouseOver())
		{
			textbox.hover.draw(r);
		}
	}

	Renderer::pushScissorRect(r.x + 3, r.y + 1, r.w - 6, r.h - 2);

	// Draw highlighted text.
	if(active && myCursor.x != myCursor.y)
	{
		String hlstr = Text::escapeMarkup(str);

		int cx = Text::getEscapedCharIndex(str, myCursor.x);
		int cy = Text::getEscapedCharIndex(str, myCursor.y);
		if(cx > cy) swapValues(cx, cy);

		Str::insert(hlstr, cy, "{tc}{bc}{sc}");
		Str::insert(hlstr, cx, "{tc:000F}{bc:FFFF}{sc:0000}");

		mySettings.textFlags |= Text::MARKUP;
		mySettings.textColor = Color32a(mySettings.textColor, 255);
		Text::arrange(Text::ML, mySettings, hlstr.str());
		Text::draw(tp);
		mySettings.textFlags &= ~Text::MARKUP;
	}
	// Draw regular text.
	else
	{
		mySettings.textColor = Color32a(mySettings.textColor, isEnabled() ? 255 : 128);
		Text::arrange(Text::ML, mySettings, str);
		Text::draw(tp);
	}

	// Draw the cursor position I-beam graphic.
	if(active && myIsEditable && myBlinkTime < 0.5f)
	{
		Text::arrange(Text::ML, mySettings, str);
		Text::CursorPos pos = Text::getCursorPos(tp, myCursor.y);
		Draw::fill({pos.x, pos.y, 1, pos.h}, Colors::white);
	}	

	Renderer::popScissorRect();
}

void WgLineEdit::hideBackground()
{
	myShowBackground = 0;
}

void WgLineEdit::setMaxLength(int n)
{
	myMaxLength = max(0, n);
}

void WgLineEdit::setNumerical(bool numerical)
{
	myIsNumerical = numerical;
}

void WgLineEdit::setEditable(bool editable)
{
	myIsEditable = editable;
}

void WgLineEdit::myDeleteSelection()
{
	if(myIsEditable && myCursor.x != myCursor.y)
	{
		if(myCursor.x > myCursor.y) swapValues(myCursor.x, myCursor.y);
		Str::erase(myEditStr, myCursor.x, myCursor.y - myCursor.x);
		myCursor.y = myCursor.x;
		myForceScrollUpdate = true;
	}
}

vec2i WgLineEdit::myTextPos() const
{
	recti r = myRect;
	return{r.x - (int)myScroll + 6, r.y + r.h / 2};
}

// ================================================================================================
// WgSpinner

WgSpinner::~WgSpinner()
{
	delete myEdit;
}

WgSpinner::WgSpinner(GuiContext* gui)
	: GuiWidget(gui)
{
	myIsUpActive = false;
	myRepeatTimer = 0.f;
	mymin = INT_MIN;
	myMax = INT_MAX;
	myStep = 1.0;
	myMinDecimalPlaces = 0;
	myMaxDecimalPlaces = 6;
	myUpdateValue(0.0);

	myEdit = new WgLineEdit(myGui);
	myEdit->setNumerical(true);
	myEdit->setMaxLength(12);
	myEdit->text.bind(&myText);
	myEdit->onChange.bind(this, &WgSpinner::myTextChange);
	myEdit->hideBackground();
}

void WgSpinner::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			myEdit->deselect();
			startCapturingMouse();
			recti r = myButtonRect();
			myIsUpActive = (evt.y <= r.y + r.h / 2);
			double sign = myIsUpActive ? 1.0 : -1.0;
			myUpdateValue(value.get() + myStep*sign);
			myRepeatTimer = 0.5f;
		}
		evt.setHandled();
	}
}

void WgSpinner::onMouseRelease(MouseRelease& evt)
{
	if(evt.button == Mouse::LMB && isCapturingMouse())
	{
		stopCapturingMouse();
	}
}

void WgSpinner::onArrange(recti r)
{
	GuiWidget::onArrange(r);
	r.w -= 14;
	myEdit->arrange(r);
}

void WgSpinner::onTick()
{
	vec2i mpos = myGui->getMousePos();
	if(IsInside(myButtonRect(), mpos.x, mpos.y))
	{
		captureMouseOver();
	}

	myEdit->tick();

	GuiWidget::onTick();
	if(isMouseOver() || myEdit->isMouseOver())
	{
		GuiMain::setTooltip(getTooltip());
	}

	// Propagate settings to the text edits.
	myEdit->setEnabled(isEnabled());

	// Check if the user is pressing the increment/decrement button.
	if(isCapturingMouse())
	{
		myRepeatTimer -= myGui->getDeltaTime();
		if(myRepeatTimer <= 0.f)
		{
			double sign = myIsUpActive ? 1.0 : -1.0;
			myUpdateValue(value.get() + myStep*sign);
			myRepeatTimer = 0.05f;
		}
	}	

	// Check if the text display is still up to date.
	double val = value.get();
	if(myDisplayValue != val) myUpdateText();
}

void WgSpinner::onDraw()
{
	auto& button = GuiDraw::getButton();
	auto& icons = GuiDraw::getIcons();
	auto& textbox = GuiDraw::getTextBox();

	// Draw the background box graphic.
	recti rtext = SideL(myRect, myRect.w - 12);
	textbox.base.draw(rtext, TileRect2::L);
	if(myEdit->isCapturingText())
	{
		rtext.w -= 2;
		textbox.active.draw(rtext, TileRect2::L);
	}
	else if(myEdit->isMouseOver())
	{
		rtext.w -= 2;
		textbox.hover.draw(rtext, TileRect2::L);
	}
	myEdit->draw();
	
	// Draw the buttons.
	recti r = myButtonRect();
	button.base.draw(r, TileRect2::R);
	
	if(isCapturingMouse())
	{
		if(myIsUpActive)
		{
			recti top = {r.x, r.y, r.w, r.h / 2 + 1};
			button.pressed.draw(top, TileRect2::TR);
		}
		else
		{
			recti btm = {r.x, r.y + r.h / 2 - 1, r.w, r.h / 2 + 1};
			button.pressed.draw(btm, TileRect2::BR);
		}
	}
	else if(isEnabled() && isMouseOver())
	{
		if(myGui->getMousePos().y < r.y + r.h / 2)
		{
			recti top = {r.x, r.y, r.w, r.h / 2 + 1};
			button.hover.draw(top, TileRect2::TR);
		}
		else
		{
			recti btm = {r.x, r.y + r.h / 2 - 1, r.w, r.h / 2 + 1};
			button.hover.draw(btm, TileRect2::BR);
		}
	}

	color32 col = Color32(isEnabled() ? 255 : 128);
	
	Draw::sprite(icons.arrow, {r.x + r.w / 2, r.y + r.h * 1 / 4}, col, Draw::ROT_90 | Draw::FLIP_H);
	Draw::sprite(icons.arrow, {r.x + r.w / 2, r.y + r.h * 3 / 4}, col, Draw::ROT_90);
}

void WgSpinner::setRange(double min, double max)
{
	mymin = min, myMax = max;
}

void WgSpinner::setStep(double step)
{
	myStep = step;
}

void WgSpinner::setPrecision(int minDecimalPlaces, int maxDecimalPlaces)
{
	myMinDecimalPlaces = minDecimalPlaces;
	myMaxDecimalPlaces = maxDecimalPlaces;
	myUpdateText();
}

void WgSpinner::myUpdateValue(double v)
{
	double prev = value.get();
	value.set(max(mymin, min(myMax, v)));
	myUpdateText();
	if(value.get() != prev) onChange.call();
}

void WgSpinner::myUpdateText()
{
	myDisplayValue = value.get();
	myText.clear();
	Str::appendVal(myText, myDisplayValue, myMinDecimalPlaces, myMaxDecimalPlaces);
}

void WgSpinner::myTextChange()
{
	double v = 0.0;
	if(Str::parse(myText.str(), v))
	{
		myUpdateValue(v);
	}
	else
	{
		myUpdateText();
	}
}

recti WgSpinner::myButtonRect()
{
	return SideR(myRect, 14);
}

}; // namespace Vortex
