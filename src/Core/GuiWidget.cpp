#include <Core/GuiWidget.h>

#include <Core/GuiContext.h>
#include <Core/GuiManager.h>

#include <System/File.h>

namespace Vortex {

#define MY_GUI ((GuiContextImpl*)myGui)

// ================================================================================================
// GuiWidget :: constructor / destructor.

	GuiContext* myGui;
	recti myRect;
	int myWidth;
	int myHeight;
	uint myFlags;

GuiWidget::GuiWidget(GuiContext* gui)
	: myGui(gui)
	, myRect({0, 0, 128, 24})
	, myWidth(128)
	, myHeight(24)
	, myFlags(WF_ENABLED)
{
}

GuiWidget::~GuiWidget()
{
	MY_GUI->removeWidget(this);
	GuiManager::removeWidget(this);
}

// ================================================================================================
// GuiWidget :: input handling.

void GuiWidget::captureMouseOver()
{
	GuiManager::makeMouseOver(this);
}

void GuiWidget::blockMouseOver()
{
	GuiManager::blockMouseOver(this);
}

void GuiWidget::unblockMouseOver()
{
	GuiManager::unblockMouseOver(this);
}

bool GuiWidget::isMouseOver() const
{
	return GuiManager::getMouseOver() == this;
}

void GuiWidget::startCapturingMouse()
{
	GuiManager::startCapturingMouse(this);
}

void GuiWidget::stopCapturingMouse()
{
	GuiManager::stopCapturingMouse(this);
}

bool GuiWidget::isCapturingMouse() const
{
	return GuiManager::getMouseCapture() == this;
}

void GuiWidget::startCapturingText()
{
	GuiManager::captureText(this);
}

void GuiWidget::stopCapturingText()
{
	GuiManager::stopCapturingText(this);
}

bool GuiWidget::isCapturingText() const
{
	return GuiManager::getTextCapture() == this;
}

void GuiWidget::onMouseCaptureLost()
{
}

void GuiWidget::onTextCaptureLost()
{
}

void GuiWidget::startCapturingFocus()
{
	MY_GUI->grabFocus(this);
	SetFlags(myFlags, WF_IN_FOCUS, true);
}

void GuiWidget::stopCapturingFocus()
{
	MY_GUI->releaseFocus(this);
	SetFlags(myFlags, WF_IN_FOCUS, false);
}

bool GuiWidget::isCapturingFocus() const
{
	return HasFlags(myFlags, WF_IN_FOCUS);
}

// ================================================================================================
// GuiWidget :: update functions.

void GuiWidget::updateSize()
{
	onUpdateSize();
}

void GuiWidget::onUpdateSize()
{
}

void GuiWidget::arrange(recti r)
{
	onArrange(r);
}

void GuiWidget::onArrange(recti r)
{
	myRect = r;
}

void GuiWidget::tick()
{
	if(!isCapturingFocus())
	{
		onTick();
	}
}

void GuiWidget::draw()
{
	if(!isCapturingFocus())
	{
		onDraw();
	}
}

void GuiWidget::onTick()
{
	if(HasFlags(myFlags, WF_ENABLED))
	{
		vec2i mpos = myGui->getMousePos();
		if(IsInside(myRect, mpos.x, mpos.y))
		{
			captureMouseOver();
		}
		handleInputs(myGui->getEvents());
		if(isMouseOver())
		{
			GuiMain::setTooltip(getTooltip());
		}
	}
}

void GuiWidget::onDraw()
{
}

// ================================================================================================
// GuiWidget :: get / set functions.

void GuiWidget::setId(StringRef id)
{
	GuiManager::setWidgetId(this, id);
}

void GuiWidget::setWidth(int w)
{
	myWidth = w;
}

void GuiWidget::setHeight(int h)
{
	myHeight = h;
}

void GuiWidget::setSize(int w, int h)
{
	myWidth = w;
	myHeight = h;
}

int GuiWidget::getWidth() const
{
	return myWidth;
}

int GuiWidget::getHeight() const
{
	return myHeight;
}

vec2i GuiWidget::getSize() const
{
	return {myWidth, myHeight};
}

recti GuiWidget::getRect() const
{
	return myRect;
}

String GuiWidget::getTooltip() const
{
	return GuiManager::getTooltip(this);
}

void GuiWidget::setEnabled(bool value)
{
	SetFlags(myFlags, WF_ENABLED, value);
}

void GuiWidget::setTooltip(StringRef text)
{
	GuiManager::setTooltip(this, text);
}

bool GuiWidget::isEnabled() const
{
	return HasFlags(myFlags, WF_ENABLED);
}

GuiContext* GuiWidget::getGui() const
{
	return myGui;
}

}; // namespace Vortex
