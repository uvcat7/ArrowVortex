#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/File.h>
#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiElement.h>
#include <Core/Interface/UiMan.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// UiEvent

UiEvent::UiEvent()
	: myUnhandled(true)
{
}

bool UiEvent::handled() const
{
	return !myUnhandled;
}

bool UiEvent::unhandled() const
{
	return myUnhandled;
}

void UiEvent::setHandled()
{
	myUnhandled = false;
}

void MousePress::invokeHandler(UiElement* element)
{
	element->onMousePress(*this);
}

void MouseRelease::invokeHandler(UiElement* element)
{
	element->onMouseRelease(*this);
}

void MouseScroll::invokeHandler(UiElement* element)
{
	element->onMouseScroll(*this);
}

void KeyPress::invokeHandler(UiElement* element)
{
	element->onKeyPress(*this);
}

void KeyRelease::invokeHandler(UiElement* element)
{
	element->onKeyRelease(*this);
}

void SystemCommand::invokeHandler(UiElement* element)
{
	element->onSystemCommand(*this);
}

void TextInput::invokeHandler(UiElement* element)
{
	element->onTextInput(*this);
}

// =====================================================================================================================
// UiElement

UiElement::UiElement()
{
	UiMan::registerElement(this);
}

UiElement::~UiElement()
{
	UiMan::unregisterElement(this);
}

bool UiElement::isMouseOver() const
{
	return UiMan::getMouseOver() == this;
}

void UiElement::startCapturingMouse()
{
	UiMan::startCapturingMouse(this);
}

void UiElement::stopCapturingMouse()
{
	UiMan::stopCapturingMouse(this);
}

bool UiElement::isCapturingMouse() const
{
	return UiMan::getMouseCapture() == this;
}

void UiElement::startCapturingText()
{
	UiMan::captureText(this);
}

void UiElement::stopCapturingText()
{
	UiMan::stopCapturingText(this);
}

bool UiElement::isCapturingText() const
{
	return UiMan::getTextCapture() == this;
}

void UiElement::setTooltip(stringref text)
{
	UiMan::setTooltip(this, text);
}

string UiElement::getTooltip() const
{
	return UiMan::getTooltip(this);
}

UiElement* UiElement::findMouseOver(int x, int y)
{
	return nullptr;
}

void UiElement::handleEvent(UiEvent& uiEvent)
{
	uiEvent.invokeHandler(this);
}

void UiElement::tick(bool enabled)
{
}

void UiElement::draw(bool enabled)
{
}

void UiElement::onMousePress(MousePress& press)
{
}

void UiElement::onMouseRelease(MouseRelease& release)
{
}

void UiElement::onMouseScroll(MouseScroll& scroll)
{
}

void UiElement::onKeyPress(KeyPress& press)
{
}

void UiElement::onKeyRelease(KeyRelease& release)
{
}

void UiElement::onSystemCommand(SystemCommand& command)
{
}

void UiElement::onTextInput(TextInput& input)
{
}

void UiElement::onMouseCaptureLost()
{
}

void UiElement::onTextCaptureLost()
{
}

} // namespace AV
