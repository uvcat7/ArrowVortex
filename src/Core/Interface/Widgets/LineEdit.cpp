#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Unicode.h>
#include <Core/Utils/Flag.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/LineEdit.h>

namespace AV {

using namespace std;
using namespace Util;

WLineEdit::~WLineEdit()
{
}

WLineEdit::WLineEdit()
	: myMaxLength(INT_MAX)
	, myDrag(DragType::NotDragging)
	, myBlinkTime(0.f)
	, myScroll(0.f)
	, myIsNumerical(false)
	, myIsEditable(true)
	, myForceScrollUpdate(false)
	, myShowBackground(true)
{
}

void WLineEdit::deselect()
{
	if (isCapturingText())
	{
		stopCapturingText();
		myDrag = DragType::NotDragging;
		myScroll = 0.f;
	}
}

void WLineEdit::setText(stringref text)
{
	if (myText != text)
	{
		deselect();
		myText = text;
		if (onChange) onChange(text);
	}
}

stringref WLineEdit::text() const
{
	return myText;
}

void WLineEdit::onSystemCommand(SystemCommand& input)
{
	if (!isCapturingText()) return;

	input.setHandled();

	myDrag = DragType::NotDragging;
	myBlinkTime = 0.f;

	if (input.type == SystemCommandType::Cut || input.type == SystemCommandType::Copy)
	{
		if (myCursor.x == myCursor.y)
		{
			System::setClipboardText(string());
		}
		else
		{
			int a = myCursor.x;
			int b = myCursor.y;
			sort(a, b);
			string substring(myEditStr.data() + a, b - a);
			System::setClipboardText(substring.data());
			if (input.type == SystemCommandType::Cut)
				myDeleteSelection();
		}
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Paste)
	{
		string clip = System::getClipboardText();
		TextInput tempEvent;
		tempEvent.text = clip.data();
		onTextInput(tempEvent);
		input.setHandled();
	}
	else if (input.type == SystemCommandType::SelectAll)
	{
		myCursor.x = 0;
		myCursor.y = (int)myEditStr.length();
		input.setHandled();
	}
	else if (input.type == SystemCommandType::Delete)
	{
		if (myCursor.x == myCursor.y)
			myCursor.y = (int)Unicode::nextChar(myEditStr, myCursor.y);

		myDeleteSelection();
	}
}

void WLineEdit::onKeyPress(KeyPress& input)
{
	if (!isCapturingText()) return;

	input.setHandled();

	if (input.key.code == KeyCode::Return) stopCapturingText();

	myDrag = DragType::NotDragging;
	myBlinkTime = 0.f;

	auto key = input.key.code;
	if (input.key.modifiers == ModifierKeys::Shift)
	{
		int& cy = myCursor.y;
		if (key == KeyCode::Home) cy = 0;
		if (key == KeyCode::End) cy = (int)myEditStr.length();
		if (key == KeyCode::Left) cy = (int)Unicode::prevChar(myEditStr, cy);
		if (key == KeyCode::Right) cy = (int)Unicode::nextChar(myEditStr, cy);
	}
	else
	{
		int& cy = myCursor.y, &cx = myCursor.x;
		if (key == KeyCode::Home) cx = cy = 0;
		if (key == KeyCode::End) cx = cy = (int)myEditStr.length();
		if (cx != cy)
		{
			if (key == KeyCode::Left) cx = cy = min(cx, cy); 
			if (key == KeyCode::Right) cx = cy = max(cx, cy);
		}
		else
		{
			if (key == KeyCode::Left) cx = cy = (int)Unicode::prevChar(myEditStr, cy);
			if (key == KeyCode::Right) cx = cy = (int)Unicode::nextChar(myEditStr, cy);
		}
	}

	if (key == KeyCode::Delete || key == KeyCode::Backspace)
	{
		if (myCursor.x == myCursor.y) 
		{
			if (key == KeyCode::Backspace)
			{
				myCursor.y = (int)Unicode::prevChar(myEditStr, myCursor.y);
			}
			else
			{
				myCursor.y = (int)Unicode::nextChar(myEditStr, myCursor.y);
			}
		}
		myDeleteSelection();
	}

	auto len = (int)myEditStr.length();
	myCursor.x = clamp(myCursor.x, 0, len);
	myCursor.y = clamp(myCursor.y, 0, len);
}

void WLineEdit::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			if (isCapturingText() && input.isDoubleClick)
			{
				myCursor.x = 0;
				myCursor.y = (int)myEditStr.length();
			}
			else
			{
				if (isCapturingText())
				{
					myDrag = DragType::RegularDrag;
				}
				else
				{
					myEditStr = myText;
					myDrag = DragType::InitialDrag;
					startCapturingText();
				}

				Text::setStyle(UiText::regular);
				Text::format(TextAlign::ML, myEditStr.data());
				int charIndex = Text::getCharIndex(myTextPos(), input.pos);
				myCursor.x = myCursor.y = charIndex;
			}

			startCapturingMouse();
			myBlinkTime = 0.f;
			input.setHandled();
		}
	}
	else deselect();
}

void WLineEdit::onMouseRelease(MouseRelease& input)
{
	if (input.button == MouseButton::LMB && isCapturingMouse())
	{
		stopCapturingMouse();
	}
	if (myDrag != DragType::NotDragging)
	{
		if (myIsNumerical && myDrag == DragType::InitialDrag && myCursor.x == myCursor.y)
		{
			myCursor.x = 0;
			myCursor.y = (int)myEditStr.length();
		}
		myDrag = DragType::NotDragging;
	}
}

void WLineEdit::onTextInput(TextInput& input)
{
	if (isCapturingText() && myIsEditable && input.unhandled())
	{
		myDeleteSelection();
		if ((int)myEditStr.length() < myMaxLength)
		{
			string str(input.text);

			replace(str.begin(), str.end(), '\n', ' ');
			replace(str.begin(), str.end(), '\r', ' ');
			
			auto maxInputLen = (ptrdiff_t)(myMaxLength - myEditStr.length());
			auto i = (ptrdiff_t)str.length();
			while (i > maxInputLen)
				i = Unicode::prevChar(str, i);

			String::truncate(str, i);
			myEditStr.insert(myCursor.y, str);

			myCursor.y += (int)str.length();
			myCursor.x = myCursor.y;
		}
		myForceScrollUpdate = true;
	}
}

void WLineEdit::onTextCaptureLost()
{
	setText(myEditStr);
	myCursor.x = myCursor.y = 0;
	myScroll = 0.f;
	myEditStr.clear();
}

void WLineEdit::tick(bool enabled)
{
	if (isEnabled() && (isMouseOver() || isCapturingMouse()))
	{
		Window::setCursor(Window::CursorIcon::IBeam);
	}

	if (!isCapturingText()) return;

	Text::setStyle(UiText::regular);
	Text::format(TextAlign::ML, myEditStr.data());

	// Update cursor position.
	Vec2 tp = myTextPos();
	if (myDrag != DragType::NotDragging)
	{
		Vec2 mp = Window::getMousePos();
		myCursor.y = Text::getCharIndex(tp, mp);
		myBlinkTime = 0.f;
	}
	myCursor.x = min(max(myCursor.x, 0), (int)myEditStr.length());
	myCursor.y = min(max(myCursor.y, 0), (int)myEditStr.length());

	// Update text offset.
	float dt = (float)Window::getDeltaTime();
	float barW = float(myRect.w() - 12);
	float textW = (float)Text::getWidth();
	float cursorX = (float)Text::getCursorPos(Vec2(0, 0), myCursor.y).x;
	float target = min(max(myScroll, cursorX - barW + 12), cursorX - 12);
	target = max(0.f, min(target, textW - barW));

	float delta = max((float)fabs(myScroll - target) * 10.f * dt, dt * 256.f);
	float smooth = (myScroll < target) ? min(myScroll + delta, target) : max(myScroll - delta, target);
	myScroll = myForceScrollUpdate ? target : smooth;
	myForceScrollUpdate = false;

	// Update blink startTime
	myBlinkTime = (float)fmod(myBlinkTime + dt, 1.f);
}

void WLineEdit::draw(bool enabled)
{
	Rect r = myRect;
	Vec2 tp = myTextPos();
	bool active = isCapturingText();

	const char* str = active ? myEditStr.data() : myText.data();

	// Draw the background box graphic.
	if (myShowBackground)
	{
		auto& textbox = UiStyle::getTextBox();
		textbox.base.draw(r);
	}

	Renderer::pushScissorRect(r.shrink(3,1));

	// Draw highlighted text.
	if (active && myCursor.x != myCursor.y)
	{
		string hlstr = Text::escapeMarkup(str);

		int cx = Text::getEscapedCharIndex(str, myCursor.x);
		int cy = Text::getEscapedCharIndex(str, myCursor.y);
		sort(cx, cy);

		hlstr.insert(cy, "{tc}{bc}{sc}");
		hlstr.insert(cx, "{tc:000F}{bc:FFFF}{sc:0000}");

		auto textStyle = UiText::regular;
		textStyle.flags |= TextOptions::Markup;
		textStyle.textColor = Color(textStyle.textColor, 255);

		Text::setStyle(textStyle);
		Text::format(TextAlign::ML, hlstr.data());
		Text::draw(tp);
	}
	// Draw regular text.
	else
	{
		auto textStyle = UiText::regular;
		textStyle.textColor = Color(textStyle.textColor, isEnabled() ? 255 : 128);
		
		Text::setStyle(textStyle);
		Text::format(TextAlign::ML, str);
		Text::draw(tp);
	}

	// Draw the cursor position I-beam graphic.
	if (active && myIsEditable && myBlinkTime < 0.5f)
	{
		Text::setStyle(UiText::regular);
		Text::format(TextAlign::ML, str);
		auto pos = Text::getCursorPos(tp, myCursor.y);
		Draw::fill(Rect(pos.x, pos.y, pos.x + 1, pos.y + pos.h), Color::White);
	}

	Renderer::popScissorRect();
}

void WLineEdit::hideBackground()
{
	myShowBackground = 0;
}

void WLineEdit::setMaxLength(int n)
{
	myMaxLength = max(0, n);
}

void WLineEdit::setNumerical(bool numerical)
{
	myIsNumerical = numerical;
}

void WLineEdit::setEditable(bool editable)
{
	myIsEditable = editable;
}

void WLineEdit::myDeleteSelection()
{
	if (myIsEditable && myCursor.x != myCursor.y)
	{
		sort(myCursor.x, myCursor.y);
		myEditStr.erase(myCursor.x, myCursor.y - myCursor.x);
		myCursor.y = myCursor.x;
		myForceScrollUpdate = true;
	}
}

Vec2 WLineEdit::myTextPos() const
{
	return Vec2(myRect.l - (int)myScroll + 6, myRect.cy());
}

} // namespace AV
