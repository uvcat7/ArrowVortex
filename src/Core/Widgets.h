#pragma once

#include <Core/Slot.h>
#include <Core/Vector.h>
#include <Core/String.h>
#include <Core/Draw.h>
#include <Core/Text.h>
#include <Core/Gui.h>

namespace Vortex {

/// Seperator Line GuiWidget.
class WgSeperator : public GuiWidget
{
public:
	~WgSeperator();
	WgSeperator(GuiContext* gui);

	void onDraw() override;
};

/// Text Label GuiWidget.
class WgLabel : public GuiWidget
{
public:
	~WgLabel();
	WgLabel(GuiContext* gui);

	void onDraw() override;

	TextSlot text;	
};

/// Push Button GuiWidget.
class WgButton : public GuiWidget
{
public:
	~WgButton();
	WgButton(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onDraw() override;

	TextSlot text;
	IntSlot counter;
	BoolSlot isDown;
	CallSlot onPress;
};

/// Checkbox GuiWidget.
class WgCheckbox : public GuiWidget
{
public:
	~WgCheckbox();
	WgCheckbox(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onDraw() override;

	TextSlot text;
	BoolSlot value;
	CallSlot onChange;

protected:
	recti myBoxRect() const;
};

/// Horizontal Slider GuiWidget.
class WgSlider : public GuiWidget
{
public:
	~WgSlider();
	WgSlider(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onTick() override;
	void onDraw() override;

	void setRange(double begin, double end);

	FloatSlot value;
	CallSlot onChange;

private:
	void myUpdateValue(double v);
	void myDrag(int x, int y);

	double myBegin, myEnd;
};

/// Base Scrollbar GuiWidget.
class WgScrollbar : public GuiWidget
{
public:
	~WgScrollbar();
	WgScrollbar(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onTick() override;
	void onDraw() override;

	void setEnd(int size);
	void setPage(int size);

	IntSlot value;
	CallSlot onChange;

	virtual bool isVertical() const = 0;

protected:
	void myUpdateValue(int v);
	int myEnd, myPage;
	uint myScrollAction : 9;
	uint myScrollGrabPos : 16;

private:
	uint myGetActionAt(int x, int y);
};

/// Base scroll region widgets.
class WgScrollRegion : public GuiWidget
{
public:
	enum ScrollType { SCROLL_ALWAYS, SCROLL_WHEN_NEEDED, SCROLL_NEVER };

	~WgScrollRegion();
	WgScrollRegion(GuiContext* gui);

	void onMouseScroll(MouseScroll& evt) override;
	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;

	void onTick() override;
	void onDraw() override;

	void setScrollType(ScrollType h, ScrollType v);
	void setScrollW(int width);
	void setScrollH(int height);

	int getViewWidth() const;
	int getViewHeight() const;

	int getScrollWidth() const;
	int getScrollHeight() const;

protected:
	void myPreTick();
	void myPostTick();
	void myClampScrollValues();

	uint myScrollTypeH : 2;
	uint myScrollTypeV : 2;
	uint myScrollbarActiveH : 1;
	uint myScrollbarActiveV : 1;
	uint myScrollAction : 9;
	uint myScrollGrabPos : 16;
	int myScrollW, myScrollH;
	int myScrollX, myScrollY;

private:
	uint myGetActionAt(int x, int y);
};

// Vertical Scrollbar GuiWidget.
class WgScrollbarV : public WgScrollbar
{
public:
	WgScrollbarV(GuiContext* gui);
	bool isVertical() const;
};

// Horizontal Scrollbar GuiWidget.
class WgScrollbarH : public WgScrollbar
{
public:
	WgScrollbarH(GuiContext* gui);
	bool isVertical() const;
};

/// Vertical List Selection GuiWidget.
class WgSelectList : public GuiWidget
{
public:
	~WgSelectList();
	WgSelectList(GuiContext* gui);

	void onArrange(recti r) override;
	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onMouseScroll(MouseScroll& evt) override;
	void onTick() override;
	void onDraw() override;

	void hideBackground();
	void addItem(StringRef text);
	void clearItems();	

	void scroll(bool up);
	bool interacted() const { return myIsInteracted; }

	IntSlot value;
	CallSlot onChange;

protected:
	int myHoverItem(int x, int y);
	bool myHasScrollbar() const;
	recti myItemRect() const;

	WgScrollbarV* myScrollbar;
	Vector<String> myItems;
	int myScrollPos;
	uint myIsInteracted : 1;
	uint myShowBackground : 1;
};

/// Vertical Drop Down List GuiWidget.
class WgDroplist : public GuiWidget
{
public:
	~WgDroplist();
	WgDroplist(GuiContext* gui);

	void onArrange(recti r) override;
	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onMouseScroll(MouseScroll& evt) override;
	void onTick() override;
	void onDraw() override;

	void addItem(StringRef text);
	void clearItems();

	IntSlot value;
	CallSlot onChange;

protected:
	void myCloseList();

	WgSelectList* myList;
	Vector<String> myItems;
	int myListItem;
};

/// Button that cycles through items in a list.
class WgCycleButton : public GuiWidget
{
public:
	~WgCycleButton();
	WgCycleButton(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onDraw() override;

	void addItem(StringRef text);
	void clearItems();

	IntSlot value;
	CallSlot onChange;

protected:
	Vector<String> myItems;
};

/// Single Line Text Editor GuiWidget.
class WgLineEdit : public GuiWidget
{
public:
	~WgLineEdit();
	WgLineEdit(GuiContext* gui);

	void onKeyPress(KeyPress& evt) override;
	void onKeyRelease(KeyRelease& evt) override;
	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onTextInput(TextInput& evt) override;
	void onTextCaptureLost() override;
	void onTick() override;
	void onDraw() override;

	void hideBackground();
	void setNumerical(bool numerical);
	void setEditable(bool editable);
	void setMaxLength(int n);

	void deselect();

	TextSlot text;
	CallSlot onChange;

private:
	void myDeleteSelection();
	vec2i myTextPos() const;

	String myEditStr;
	int myMaxLength, myDrag;
	vec2i myCursor;
	float myBlinkTime, myScroll;
	uint myIsNumerical : 1;
	uint myIsEditable : 1;
	uint myForceScrollUpdate : 1;
	uint myShowBackground : 1;
	TextStyle mySettings;
};

/// Spin Box GuiWidget.
class WgSpinner : public GuiWidget
{
public:
	~WgSpinner();
	WgSpinner(GuiContext* gui);

	void onArrange(recti r) override;
	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onTick() override;
	void onDraw() override;

	void setRange(double min, double max);
	void setStep(double step);
	void setPrecision(int minDecimalPlaces, int maxDecimalPlaces);
	
	FloatSlot value;
	CallSlot onChange;

private:
	void myUpdateValue(double v);
	void myUpdateText();
	void myTextChange();
	recti myButtonRect();

	WgLineEdit* myEdit;
	bool myIsUpActive;
	float myRepeatTimer;
	double mymin, myMax, myStep;
	double myDisplayValue;
	int myMinDecimalPlaces;
	int myMaxDecimalPlaces;
	String myText;
};

/// Color picker widget.
class WgColorPicker : public GuiWidget
{
public:
	~WgColorPicker();
	WgColorPicker(GuiContext* gui);

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;
	void onTick() override;
	void onDraw() override;

	FloatSlot red, green, blue, alpha;
	CallSlot onChange;

private:
	struct Expanded;
	Expanded* myExpanded;
};

}; // namespace Vortex
