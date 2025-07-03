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
	, backgroundVisible_(1)
{
	maxLength_ = 1 << 24;
	blinkTime_ = 0.f;
	scroll_ = 0.f;
	drag_ = DT_NOT_DRAGGING;
	isNumerical_ = false;
	isEditable_ = true;
	forceScrollUpdate_ = false;
}

void WgLineEdit::deselect()
{
	if(isCapturingText())
	{
		stopCapturingText();
		drag_ = DT_NOT_DRAGGING;
		scroll_ = 0.f;
	}
}

void WgLineEdit::onKeyPress(KeyPress& evt)
{
	if(!isCapturingText()) return;

	evt.handled = true;

	if(evt.key == Key::RETURN) stopCapturingText();

	drag_ = DT_NOT_DRAGGING;
	blinkTime_ = 0.f;

	Key::Code key = evt.key;
	if(evt.keyflags == Keyflag::CTRL)
	{
		if(key == Key::X || key == Key::C)
		{
			if(cursor_.x == cursor_.y)
			{
				GuiMain::setClipboardText(String());
			}
			else
			{
				int a = cursor_.x, b = cursor_.y;
				if(a > b) swapValues(a, b);
				String substring(editStr_.begin() + a, b - a);
				GuiMain::setClipboardText(substring.str());
				if(key == Key::X) deleteSection_();
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
			cursor_.x = 0;
			cursor_.y = editStr_.len();
			evt.handled = true;
		}
	}
	else if(evt.keyflags == Keyflag::SHIFT)
	{
		int& cy = cursor_.y;
		if(key == Key::HOME) cy = 0;
		if(key == Key::END) cy = editStr_.len();
		if(key == Key::LEFT) cy = Str::prevChar(editStr_, cy);
		if(key == Key::RIGHT) cy = Str::nextChar(editStr_, cy);
	}
	else
	{
		int& cy = cursor_.y, &cx = cursor_.x;
		if(key == Key::HOME) cx = cy = 0;
		if(key == Key::END) cx = cy = editStr_.len();
		if(cx != cy)
		{
			if(key == Key::LEFT) cx = cy = min(cx, cy); 
			if(key == Key::RIGHT) cx = cy = max(cx, cy);
		}
		else
		{
			if(key == Key::LEFT) cx = cy = Str::prevChar(editStr_, cy);
			if(key == Key::RIGHT) cx = cy = Str::nextChar(editStr_, cy);
		}
	}

	if(key == Key::DELETE || key == Key::BACKSPACE)
	{
		if(cursor_.x == cursor_.y) 
		{
			if(key == Key::BACKSPACE)
			{
				cursor_.y = Str::prevChar(editStr_, cursor_.y);
			}
			else
			{
				cursor_.y = Str::nextChar(editStr_, cursor_.y);
			}
		}
		deleteSection_();
	}

	int len = editStr_.len();
	cursor_.x = clamp(cursor_.x, 0, len);
	cursor_.y = clamp(cursor_.y, 0, len);
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
				cursor_.x = 0;
				cursor_.y = editStr_.len();
			}
			else
			{
				if(isCapturingText())
				{
					drag_ = DT_REGULAR_DRAG;
				}
				else
				{
					editStr_ = text.get();
					drag_ = DT_INITIAL_DRAG;
					startCapturingText();
				}

				vec2i t = textPos_();
				Text::arrange(Text::ML, lineedit_style_, editStr_.str());
				int charIndex = Text::getCharIndex(vec2i{t.x, t.y}, {evt.x, evt.y});
				cursor_.x = cursor_.y = charIndex;
			}

			startCapturingMouse();
			blinkTime_ = 0.f;
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
	if(drag_ != DT_NOT_DRAGGING)
	{
		if(isNumerical_ && drag_ == DT_INITIAL_DRAG && cursor_.x == cursor_.y)
		{
			cursor_.x = 0;
			cursor_.y = editStr_.len();
		}
		drag_ = DT_NOT_DRAGGING;
	}
}

void WgLineEdit::onTextInput(TextInput& evt)
{
	if(isCapturingText() && isEditable_ && !evt.handled)
	{
		deleteSection_();
		if((int)editStr_.len() < maxLength_)
		{
			String input(evt.text);
			
			int maxInputLen = maxLength_ - editStr_.len();

			int i = input.len();
			while(i > maxInputLen) i = Str::prevChar(input, i);
			if(i < (int)input.len()) Str::erase(input, i);

			Str::insert(editStr_, cursor_.y, input);

			cursor_.y += input.len();
			cursor_.x = cursor_.y;
		}
		forceScrollUpdate_ = true;
	}
}

void WgLineEdit::onTextCaptureLost()
{
	if(editStr_ != text.get())
	{
		text.set(editStr_);
		onChange.call();
	}
	cursor_.x = cursor_.y = 0;
	scroll_ = 0.f;
	editStr_.release();
}

void WgLineEdit::onTick()
{
	GuiWidget::onTick();

	if(isEnabled() && (isMouseOver() || isCapturingMouse()))
	{
		GuiMain::setCursorIcon(Cursor::IBEAM);
	}

	if(!isCapturingText()) return;

	Text::arrange(Text::ML, lineedit_style_, editStr_.str());

	// Update cursor position.
	vec2i tp = textPos_();
	if(drag_ != DT_NOT_DRAGGING)
	{
		vec2i mp = gui_->getMousePos();
		cursor_.y = Text::getCharIndex(vec2i{tp.x, tp.y}, {mp.x, tp.y});
		blinkTime_ = 0.f;
	}
	cursor_.x = min(max(cursor_.x, 0), (int)editStr_.len());
	cursor_.y = min(max(cursor_.y, 0), (int)editStr_.len());

	// Update text offset
	
	float dt = gui_->getDeltaTime();
	float barW = (float)(rect_.w - 12);
	float textW = (float)Text::getSize().x;
	float cursorX = (float)Text::getCursorPos(vec2i{0, 0}, cursor_.y).x;
	float target = min(max(scroll_, cursorX - barW + 12), cursorX - 12);
	target = max(0.f, min(target, textW - barW));

	float delta = max((float)fabs(scroll_ - target) * 10.f * dt, dt * 256.f);
	float smooth = (scroll_ < target) ? min(scroll_ + delta, target) : max(scroll_ - delta, target);
	scroll_ = forceScrollUpdate_ ? target : smooth;
	forceScrollUpdate_ = false;

	// Update blink time
	blinkTime_ = fmod(blinkTime_ + dt, 1.f);
}

void WgLineEdit::onDraw()
{
	recti r = rect_;
	vec2i tp = textPos_();
	bool active = isCapturingText();

	const char* str = active ? editStr_.str() : text.get();

	// Draw the background box graphic.
	if(backgroundVisible_)
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
	if(active && cursor_.x != cursor_.y)
	{
		String hlstr = Text::escapeMarkup(str);

		int cx = Text::getEscapedCharIndex(str, cursor_.x);
		int cy = Text::getEscapedCharIndex(str, cursor_.y);
		if(cx > cy) swapValues(cx, cy);

		Str::insert(hlstr, cy, "{tc}{bc}{sc}");
		Str::insert(hlstr, cx, "{tc:000F}{bc:FFFF}{sc:0000}");

		lineedit_style_.textFlags |= Text::MARKUP;
		lineedit_style_.textColor = Color32a(lineedit_style_.textColor, 255);
		Text::arrange(Text::ML, lineedit_style_, hlstr.str());
		Text::draw(tp);
		lineedit_style_.textFlags &= ~Text::MARKUP;
	}
	// Draw regular text.
	else
	{
		lineedit_style_.textColor = Color32a(lineedit_style_.textColor, isEnabled() ? 255 : 128);
		Text::arrange(Text::ML, lineedit_style_, str);
		Text::draw(tp);
	}

	// Draw the cursor position I-beam graphic.
	if(active && isEditable_ && blinkTime_ < 0.5f)
	{
		Text::arrange(Text::ML, lineedit_style_, str);
		Text::CursorPos pos = Text::getCursorPos(tp, cursor_.y);
		Draw::fill({pos.x, pos.y, 1, pos.h}, Colors::white);
	}	

	Renderer::popScissorRect();
}

void WgLineEdit::hideBackground()
{
	backgroundVisible_ = 0;
}

void WgLineEdit::setMaxLength(int n)
{
	maxLength_ = max(0, n);
}

void WgLineEdit::setNumerical(bool numerical)
{
	isNumerical_ = numerical;
}

void WgLineEdit::setEditable(bool editable)
{
	isEditable_ = editable;
}

void WgLineEdit::deleteSection_()
{
	if(isEditable_ && cursor_.x != cursor_.y)
	{
		if(cursor_.x > cursor_.y) swapValues(cursor_.x, cursor_.y);
		Str::erase(editStr_, cursor_.x, cursor_.y - cursor_.x);
		cursor_.y = cursor_.x;
		forceScrollUpdate_ = true;
	}
}

vec2i WgLineEdit::textPos_() const
{
	recti r = rect_;
	return{r.x - (int)scroll_ + 6, r.y + r.h / 2};
}

// ================================================================================================
// WgSpinner

WgSpinner::~WgSpinner()
{
	delete spinnerInput_;
}

WgSpinner::WgSpinner(GuiContext* gui)
	: GuiWidget(gui)
{
	spinnerIsUpActive = false;
	spinnerRepeatTimer_ = 0.f;
	spinnerMin_ = INT_MIN;
	spinnerMax_ = INT_MAX;
	spinnerStepSize_ = 1.0;
	spinnerMinDecimalPlaces_ = 0;
	spinnerMaxDecimalPlaces_ = 6;
	spinnerUpdateValue_(0.0);

	spinnerInput_ = new WgLineEdit(gui_);
	spinnerInput_->setNumerical(true);
	spinnerInput_->setMaxLength(12);
	spinnerInput_->text.bind(&spinnerText_);
	spinnerInput_->onChange.bind(this, &WgSpinner::spinnerOnTextChange_);
	spinnerInput_->hideBackground();
}

void WgSpinner::onMousePress(MousePress& evt)
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			spinnerInput_->deselect();
			startCapturingMouse();
			recti r = spinnerButtonRect_();
			spinnerIsUpActive = (evt.y <= r.y + r.h / 2);
			double sign = spinnerIsUpActive ? 1.0 : -1.0;
			spinnerUpdateValue_(value.get() + spinnerStepSize_*sign);
			spinnerRepeatTimer_ = 0.5f;
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
	spinnerInput_->arrange(r);
}

void WgSpinner::onTick()
{
	vec2i mpos = gui_->getMousePos();
	if(IsInside(spinnerButtonRect_(), mpos.x, mpos.y))
	{
		captureMouseOver();
	}

	spinnerInput_->tick();

	GuiWidget::onTick();
	if(isMouseOver() || spinnerInput_->isMouseOver())
	{
		GuiMain::setTooltip(getTooltip());
	}

	// Propagate settings to the text edits.
	spinnerInput_->setEnabled(isEnabled());

	// Check if the user is pressing the increment/decrement button.
	if(isCapturingMouse())
	{
		spinnerRepeatTimer_ -= gui_->getDeltaTime();
		if(spinnerRepeatTimer_ <= 0.f)
		{
			double sign = spinnerIsUpActive ? 1.0 : -1.0;
			spinnerUpdateValue_(value.get() + spinnerStepSize_*sign);
			spinnerRepeatTimer_ = 0.05f;
		}
	}	

	// Check if the text display is still up to date.
	double val = value.get();
	if(spinnerDisplayValue_ != val) spinnerUpdateText_();
}

void WgSpinner::onDraw()
{
	auto& button = GuiDraw::getButton();
	auto& icons = GuiDraw::getIcons();
	auto& textbox = GuiDraw::getTextBox();

	// Draw the background box graphic.
	recti rtext = SideL(rect_, rect_.w - 12);
	textbox.base.draw(rtext, TileRect2::L);
	if(spinnerInput_->isCapturingText())
	{
		rtext.w -= 2;
		textbox.active.draw(rtext, TileRect2::L);
	}
	else if(spinnerInput_->isMouseOver())
	{
		rtext.w -= 2;
		textbox.hover.draw(rtext, TileRect2::L);
	}
	spinnerInput_->draw();
	
	// Draw the buttons.
	recti r = spinnerButtonRect_();
	button.base.draw(r, TileRect2::R);
	
	if(isCapturingMouse())
	{
		if(spinnerIsUpActive)
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
		if(gui_->getMousePos().y < r.y + r.h / 2)
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
	spinnerMin_ = min, spinnerMax_ = max;
}

void WgSpinner::setStep(double step)
{
	spinnerStepSize_ = step;
}

void WgSpinner::setPrecision(int minDecimalPlaces, int maxDecimalPlaces)
{
	spinnerMinDecimalPlaces_ = minDecimalPlaces;
	spinnerMaxDecimalPlaces_ = maxDecimalPlaces;
	spinnerUpdateText_();
}

void WgSpinner::spinnerUpdateValue_(double v)
{
	double prev = value.get();
	value.set(max(spinnerMin_, min(spinnerMax_, v)));
	spinnerUpdateText_();
	if(value.get() != prev) onChange.call();
}

void WgSpinner::spinnerUpdateText_()
{
	spinnerDisplayValue_ = value.get();
	spinnerText_.clear();
	Str::appendVal(spinnerText_, spinnerDisplayValue_, spinnerMinDecimalPlaces_, spinnerMaxDecimalPlaces_);
}

void WgSpinner::spinnerOnTextChange_()
{
	double v = 0.0;
	if(Str::parse(spinnerText_.str(), v))
	{
		spinnerUpdateValue_(v);
	}
	else
	{
		spinnerUpdateText_();
	}
}

recti WgSpinner::spinnerButtonRect_()
{
	return SideR(rect_, 14);
}

}; // namespace Vortex
