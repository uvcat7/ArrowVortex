#pragma once

#include <Core/Interface/UiElement.h>
#include <Core/Interface/DialogFrame.h>

namespace AV {

class UiElement;
class Widget;
class Dialog;

namespace UiMan
{	
	void initialize();
	void deinitialize();

	// Registers a top-level UI element. Registered elements will be ticked, drawn and receive inputs.
	void add(unique_ptr<UiElement>&& element);

	// Call every frame to update the dialogs and widgets.
	void tick();
	
	// Call every frame to draw the dialogs and widgets.
	void draw();

	// Sends an UI event to all elements.
	void send(UiEvent& uiEvent);

	// Sets the current tooltip text, which is reset to an empty string at the start of each frame.
	void setTooltip(std::string text);
	
	// Returns the current tooltip text.
	std::string getTooltip();
	
	// Returns true if the mouse is hovering over an elem or if an elem is capturing mouse input.
	bool isCapturingMouse();
	
	// Returns true if an elem is capturing text input.
	bool isCapturingText();
	
	// GUI elements.
	
	// static void startAnimation(Widget* w, int identifier, float duration, Widget::Tween tween);
	// static float getAnimationProgress(Widget* w, int identifier);
	
	// Invoked from the constructor of UiElement, do not call directly.
	void registerElement(UiElement* elem);
	
	// Invoked from the destructor of UiElement, do not call directly.
	void unregisterElement(UiElement* elem);
		
	// Tooltip.
	
	void setTooltip(const UiElement* elem, stringref str);
	std::string getTooltip(const UiElement* elem);
	
	// Input.
	
	void startCapturingMouse(UiElement* elem);
	void captureText(UiElement* elem);
	
	void stopCapturingMouse(UiElement* elem);
	void stopCapturingText(UiElement* elem);
	
	UiElement* getMouseOver();
	UiElement* getMouseCapture();
	UiElement* getTextCapture();
};

} // namespace AV
