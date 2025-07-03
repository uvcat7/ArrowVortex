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
	recti getCheckboxRect_() const;
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
	void sliderUpdateValue_(double v);
	void sliderDrag_(int x, int y);

	double slider_begin_, slider_end_;
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
	void scrollbarUpdateValue_(int v);
	int scrollbarEnd_, scrollbarPage_;
	uint scrollbarAction_ : 9;
	uint scrollbarGrabPosition_ : 16;

private:
	uint getScrollbarActionAt_(int x, int y);
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
	void preTick_();
	void postTick_();
	void clampScrollValues_();

	uint scrollTypeHorizontal_ : 2;
	uint scrollTypeVertical_ : 2;
	uint isHorizontalScrollbarActive_ : 1;
	uint isVerticalScrollbarActive_ : 1;
	uint scrollRegionAction_ : 9;
	uint scrollRegionGrabPosition_ : 16;
	int scrollWidth_, scrollHeight_;
	int scrollPositionX_, scrollPositionY_;

private:
	uint getScrollRegionActionAt_(int x, int y);
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
	bool interacted() const { return isInteracted_; }

	IntSlot value;
	CallSlot onChange;

protected:
	int HoveredItem_(int x, int y);
	bool hasScrollbar_() const;
	recti itemRect_() const;

	WgScrollbarV* scrollbar_;
	Vector<String> scrollbarItems_;
	int scrollPosition_;
	uint isInteracted_ : 1;
	uint showBackground_ : 1;
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
	void dropListClose_();

	WgSelectList* selectList_;
	Vector<String> listItems_;
	int selectedIndex_;
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
	Vector<String> cycleItems_;
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
	void deleteSection_();
	vec2i textPos_() const;

	String editStr_;
	int maxLength_, drag_;
	vec2i cursor_;
	float blinkTime_, scroll_;
	uint isNumerical_ : 1;
	uint isEditable_ : 1;
	uint forceScrollUpdate_ : 1;
	uint backgroundVisible_ : 1;
	TextStyle lineedit_style_;
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
	void spinnerUpdateValue_(double v);
	void spinnerUpdateText_();
	void spinnerOnTextChange_();
	recti spinnerButtonRect_();

	WgLineEdit* spinnerInput_;
	bool spinnerIsUpActive;
	float spinnerRepeatTimer_;
	double spinnerMin_, spinnerMax_, spinnerStepSize_;
	double spinnerDisplayValue_;
	int spinnerMinDecimalPlaces_;
	int spinnerMaxDecimalPlaces_;
	String spinnerText_;
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
	Expanded* colorpicker_expanded_;
};

}; // namespace Vortex
