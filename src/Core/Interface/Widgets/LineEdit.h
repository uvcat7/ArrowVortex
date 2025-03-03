#pragma once

#include <Core/Graphics/Text.h>

#include <Core/Interface/Widget.h>

namespace AV {

class WLineEdit : public Widget
{
public:
	~WLineEdit();
	WLineEdit();

	void onSystemCommand(SystemCommand& input) override;
	void onKeyPress(KeyPress& input) override;
	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;
	void onTextInput(TextInput& input) override;
	void onTextCaptureLost() override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void hideBackground();
	void setNumerical(bool numerical);
	void setEditable(bool editable);
	void setMaxLength(int n);

	void deselect();

	void setText(stringref text);
	stringref text() const;

	std::function<void(stringref)> onChange;

protected:
	enum class DragType
	{
		NotDragging = 0,
		InitialDrag,
		RegularDrag,
	};

	void myDeleteSelection();
	Vec2 myTextPos() const;

	std::string myText;
	std::string myEditStr;
	int myMaxLength;
	DragType myDrag;
	Vec2 myCursor;
	float myBlinkTime, myScroll;
	uint myIsNumerical : 1;
	uint myIsEditable : 1;
	uint myForceScrollUpdate : 1;
	uint myShowBackground : 1;
};

} // namespace AV
