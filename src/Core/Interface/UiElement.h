#pragma once

#include <Core/Common/Input.h>
#include <Core/Types/Rect.h>

namespace AV {

class UiElement;

// Base class for input events.
class UiEvent
{
public:
	UiEvent();

	bool handled() const;
	bool unhandled() const;
	void setHandled();

	virtual void invokeHandler(UiElement* element) = 0;

private:
	bool myUnhandled;
};

// Contains data from a mouse button press event.
class MousePress : public UiEvent
{
public:
	MouseButton button;     // The mouse button that was pressed.
	Vec2 pos;               // The position of the mouse cursor.
	ModifierKeys modifiers; // Modifier keys that were down.
	bool isDoubleClick;     // True if the press is the second press of a double click.

	void invokeHandler(UiElement* element) override;
};

// Contains data from a mouse button release event.
class MouseRelease : public UiEvent
{
public:
	MouseButton button;     // The mouse button that was released.
	Vec2 pos;               // The position of the mouse cursor.
	ModifierKeys modifiers; // Modifier keys that were down.

	void invokeHandler(UiElement* element) override;
};

// Contains data from a mouse scroll event.
class MouseScroll : public UiEvent
{
public:
	bool isUp;              // True when scrolling up, false when scrolling down.
	Vec2 pos;               // The position of the mouse cursor.
	ModifierKeys modifiers; // Modifier keys that were down.

	void invokeHandler(UiElement* element) override;
};

// Contains data from a keyboard key press event.
class KeyPress : public UiEvent
{
public:
	KeyCombination key; // The key that was pressed, plus modifier keys that were down.
	bool isRepeated;    // Indicates the press was an autorepeat triggered by holding down the key.

	void invokeHandler(UiElement* element) override;
};

// Contains data from a keyboard key release event.
class KeyRelease : public UiEvent
{
public:
	KeyCombination key; // The key that was released, plus modifier keys that were down.

	void invokeHandler(UiElement* element) override;
};

// Enumeration of system commands with context-specific uses.
enum class SystemCommandType
{
	Undo,
	Redo,
	Cut,
	Copy,
	Paste,
	Delete,
	SelectAll,
};

// Contains data from the usage of a special key sequence.
class SystemCommand : public UiEvent
{
public:
	SystemCommandType type;

	void invokeHandler(UiElement* element) override;
};

// Contains data from a text input event.
class TextInput : public UiEvent
{
public:
	std::string text; // The text that was entered (for example, by a key press or IME).

	void invokeHandler(UiElement* element) override;
};

// Base class for UI elements.
class UiElement
{
public:
	UiElement();

	virtual ~UiElement();

	// Returns whether the element is currently the mouse over element.
	bool isMouseOver() const;

	// Preventing other elements from receiving mouse input and becoming the mouse over element.
	void startCapturingMouse();
	void stopCapturingMouse();
	bool isCapturingMouse() const;

	// Prevents other elements from receiving text input.
	void startCapturingText();
	void stopCapturingText();
	bool isCapturingText() const;

	// Sets the tooltip text for the element.
	void setTooltip(stringref text);
	std::string getTooltip() const;

	// Invoked by the GUI when trying to determine the mouse over element.
	virtual UiElement* findMouseOver(int x, int y);

	// Occurs when an UI event is received.
	virtual void handleEvent(UiEvent& uiEvent);

	// Invoked every frame to update the element.
	virtual void tick(bool enabled);

	// Invoked every frame to draw the element.
	virtual void draw(bool enabled);

	// Occurs when the user presses a mouse button.
	virtual void onMousePress(MousePress& press);

	// Occurs when the user releases a mouse button.
	virtual void onMouseRelease(MouseRelease& release);

	// Occurs when the user scrolls the mouse wheel.
	virtual void onMouseScroll(MouseScroll& scroll);

	// Occurs when the user presses a key.
	virtual void onKeyPress(KeyPress& press);

	// Occurs when the user releases a key.
	virtual void onKeyRelease(KeyRelease& release);

	// Occurs when a special key sequence was used.
	virtual void onSystemCommand(SystemCommand& command);

	// Occurs when the user enters text (e.g. through a key press or IME).
	virtual void onTextInput(TextInput& input);

	// Invoked by the GUI when the element stops or is forced to stop capturing the mouse.
	virtual void onMouseCaptureLost();

	// Invoked by the GUI when the element stops or is forced to stop capturing text.
	virtual void onTextCaptureLost();
};

} // namespace AV