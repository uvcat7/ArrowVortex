#pragma once

#include <Core/Slot.h>
#include <Core/Draw.h>
#include <Core/Text.h>
#include <Core/Gui.h>

#include <vector>

namespace Vortex {

/// Seperator Line GuiWidget.
class WgSeperator : public GuiWidget {
   public:
    ~WgSeperator();
    explicit WgSeperator(GuiContext* gui);

    void onDraw() override;
};

/// Text Label GuiWidget.
class WgLabel : public GuiWidget {
   public:
    ~WgLabel();
    explicit WgLabel(GuiContext* gui);

    void onDraw() override;

    TextSlot text;
};

/// Push Button GuiWidget.
class WgButton : public GuiWidget {
   public:
    ~WgButton();
    explicit WgButton(GuiContext* gui);

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onDraw() override;

    TextSlot text;
    IntSlot counter;
    BoolSlot isDown;
    CallSlot onPress;
};

/// Checkbox GuiWidget.
class WgCheckbox : public GuiWidget {
   public:
    ~WgCheckbox();
    explicit WgCheckbox(GuiContext* gui);

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onDraw() override;

    TextSlot text;
    BoolSlot value;
    CallSlot onChange;

   protected:
    recti GetCheckboxRect() const;
};

/// Horizontal Slider GuiWidget.
class WgSlider : public GuiWidget {
   public:
    ~WgSlider();
    explicit WgSlider(GuiContext* gui);

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onTick() override;
    void onDraw() override;

    void setRange(double begin, double end);

    FloatSlot value;
    CallSlot onChange;

   private:
    void SliderUpdateValue(double v);
    void SliderDrag(int x, int y);

    double slider_begin_, slider_end_;
};

/// Base Scrollbar GuiWidget.
class WgScrollbar : public GuiWidget {
   public:
    ~WgScrollbar();
    explicit WgScrollbar(GuiContext* gui);

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
    void ScrollbarUpdateValue(int v);
    int scrollbar_end_, scrollbar_page_;
    uint32_t scrollbar_action_ : 9 = 0;
    uint32_t scrollbar_grab_position_ : 16 = 0;

   private:
    uint32_t GetScrollbarActionAtPosition(int x, int y);
};

/// Base scroll region widgets.
class WgScrollRegion : public GuiWidget {
   public:
    enum ScrollType { SCROLL_ALWAYS, SCROLL_WHEN_NEEDED, SCROLL_NEVER };

    ~WgScrollRegion();
    explicit WgScrollRegion(GuiContext* gui);

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
    void PreTick();
    void PostTick();
    void ClampScrollPositions();

    uint32_t scroll_type_horizontal_ : 2 = 0;
    uint32_t scroll_type_vertical_ : 2 = 0;
    uint32_t is_horizontal_scrollbar_active_ : 1 = 0;
    uint32_t is_vertical_scrollbar_active_ : 1 = 0;
    uint32_t scroll_region_action_ : 9 = 0;
    uint32_t scroll_region_grab_position_ : 16 = 0;
    int scroll_width_ = 0, scroll_height_ = 0;
    int scroll_position_x_ = 0, scroll_position_y_ = 0;

   private:
    uint32_t getScrollRegionActionAt_(int x, int y);
};

// Vertical Scrollbar GuiWidget.
class WgScrollbarV : public WgScrollbar {
   public:
    explicit WgScrollbarV(GuiContext* gui);
    bool isVertical() const;
};

// Horizontal Scrollbar GuiWidget.
class WgScrollbarH : public WgScrollbar {
   public:
    explicit WgScrollbarH(GuiContext* gui);
    bool isVertical() const;
};

/// Vertical List Selection GuiWidget.
class WgSelectList : public GuiWidget {
   public:
    ~WgSelectList();
    explicit WgSelectList(GuiContext* gui);

    void onArrange(recti r) override;
    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onMouseScroll(MouseScroll& evt) override;
    void onTick() override;
    void onDraw() override;

    void hideBackground();
    void addItem(const std::string& text);
    void clearItems();

    void scroll(bool up);
    bool interacted() const { return is_interacted_; }

    IntSlot value;
    CallSlot onChange;

   protected:
    int HoveredItem(int x, int y);
    bool HasScrollBar() const;
    recti ItemRect() const;

    WgScrollbarV* scrollbar_ = nullptr;
    std::vector<std::string> selectlist_items_;
    int scroll_position_ = 0;
    uint32_t is_interacted_ : 1 = 0;
    uint32_t show_background_ : 1 = 1;
};

/// Vertical Drop Down List GuiWidget.
class WgDroplist : public GuiWidget {
   public:
    ~WgDroplist();
    explicit WgDroplist(GuiContext* gui);

    void onArrange(recti r) override;
    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onMouseScroll(MouseScroll& evt) override;
    void onTick() override;
    void onDraw() override;

    void addItem(const std::string& text);
    void clearItems();

    IntSlot value;
    CallSlot onChange;

   protected:
    void CloseDroplist();

    WgSelectList* selectlist_widget_;
    std::vector<std::string> droplist_items_;
    int selected_index_;
};

/// Button that cycles through items in a list.
class WgCycleButton : public GuiWidget {
   public:
    ~WgCycleButton();
    explicit WgCycleButton(GuiContext* gui);

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onDraw() override;

    void addItem(const std::string& text);
    void clearItems();

    IntSlot value;
    CallSlot onChange;

   protected:
    std::vector<std::string> cycle_items_;
};

/// Single Line Text Editor GuiWidget.
class WgLineEdit : public GuiWidget {
   public:
    ~WgLineEdit();
    explicit WgLineEdit(GuiContext* gui);

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
    void DeleteSection();
    vec2i TextPosition() const;

    std::string lineedit_text_;
    int lineedit_max_length_, lineedit_drag_;
    vec2i lineedit_cursor_;
    float lineedit_blink_time_, lineedit_scroll_offset_;
    uint32_t is_numerical_ : 1;
    uint32_t is_editable_ : 1;
    uint32_t force_scroll_update_ : 1;
    uint32_t lineedit_show_background_ : 1 = 1;
    TextStyle lineedit_style_;
};

/// Spin Box GuiWidget.
class WgSpinner : public GuiWidget {
   public:
    ~WgSpinner();
    explicit WgSpinner(GuiContext* gui);

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
    void SpinnerUpdateValue(double v);
    void SpinnerUpdateText();
    void SpinnerOnTextChange();
    recti SpinnerButtonRect();

    WgLineEdit* spinner_lineedit_;
    bool spinner_is_up_pressed_;
    float spinner_repeat_timer_;
    double spinner_min_, spinner_max_, spinner_step_size_;
    double spinner_display_value_;
    int spinner_min_decimal_places_;
    int spinner_max_decimal_places_;
    std::string spinner_text_;
};

/// Color picker widget.
class WgColorPicker : public GuiWidget {
   public:
    ~WgColorPicker();
    explicit WgColorPicker(GuiContext* gui);

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;
    void onTick() override;
    void onDraw() override;

    FloatSlot red, green, blue, alpha;
    CallSlot onChange;

   private:
    struct Expanded;
    Expanded* colorpicker_expanded_ = nullptr;
};

};  // namespace Vortex
