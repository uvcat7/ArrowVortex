#include <Precomp.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/Widget.h>
#include <Core/Interface/UiMan.h>

namespace AV {

// =====================================================================================================================
// Widget :: constructor / destructor.

Widget::Widget()
	: myWidth(128)
	, myHeight(24)
	, myMinWidth(128)
	, myMinHeight(24)
	, myRect(Rect(0, 0, 128, 24))
	, myIsEnabled(true)
{
}

Widget::Widget(Widget* parent)
	: Widget()
{
}

Widget::~Widget()
{
}

// =====================================================================================================================
// Widget :: event handling.

void Widget::measure()
{
	return onMeasure();
}

void Widget::arrange(Rect r)
{
	onArrange(r);
}

// =====================================================================================================================
// Widget :: get / set functions.

bool Widget::isEnabled() const
{
	return myIsEnabled;
}

int Widget::getWidth() const
{
	return myWidth;
}

int Widget::getHeight() const
{
	return myHeight;
}

int Widget::getMinWidth() const
{
	return myMinWidth;
}

int Widget::getMinHeight() const
{
	return myMinHeight;
}

Rect Widget::getRect() const
{
	return myRect;
}

void Widget::setEnabled(bool value)
{
	myIsEnabled = value;
}

UiElement* Widget::findMouseOver(int x, int y)
{
	return myRect.contains(x, y) ? this : nullptr;
}

// =====================================================================================================================
// Widget :: virtual functions.

void Widget::onMeasure()
{
}

void Widget::onArrange(Rect r)
{
	myRect = r;
}

} // namespace AV
