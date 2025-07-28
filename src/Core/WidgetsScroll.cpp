#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

#include <stdint.h>

namespace Vortex {
namespace {

enum ScrollActions {
    ACT_NONE,

    ACT_HOVER_BUTTON_H,
    ACT_HOVER_BUTTON_V,

    ACT_HOVER_BUMP_UP,
    ACT_HOVER_BUMP_DOWN,
    ACT_HOVER_BUMP_LEFT,
    ACT_HOVER_BUMP_RIGHT,

    ACT_DRAGGING_H,
    ACT_DRAGGING_V,
};

// ================================================================================================
// Helper functions.

struct ScrollButtonData {
    int pos, size;
};

static ScrollButtonData GetButton(int size, int end, int page, int scroll) {
    ScrollButtonData out = {0, size};
    if (end > page) {
        out.size = (int)(0.5 + size * (double)page / (double)end);
        out.size = max(16, out.size);

        out.pos = (int)(0.5 + scroll * (double)(size - out.size) /
                                  (double)(end - page));
        out.pos = max(0, min(out.pos, size - out.size));
    }
    return out;
}

static int GetScroll(int size, int end, int page, int pos) {
    int out = 0;
    if (end > page) {
        ScrollButtonData button = GetButton(size, end, page, 0);
        out = (int)(0.5 +
                    pos * (double)(end - page) / (double)(size - button.size));
        out = max(0, min(out, end - page));
    }
    return out;
}

static void DrawScrollbar(recti bar, ScrollButtonData button, bool vertical,
                          bool hover, bool focus) {
    auto& scrollbar = GuiDraw::getScrollbar();
    scrollbar.bar.draw(bar);
    recti buttonRect;
    if (vertical) {
        buttonRect = {bar.x, bar.y + button.pos, bar.w, button.size};
        scrollbar.base.draw(buttonRect);
        if (buttonRect.h >= 16) {
            vec2i grabPos = {bar.x + bar.w / 2,
                             buttonRect.y + buttonRect.h / 2};
            Draw::sprite(scrollbar.grab, grabPos, Draw::ROT_90);
        }
    } else {
        buttonRect = {bar.x + button.pos, bar.y, button.size, bar.h};
        scrollbar.base.draw(buttonRect);
        if (buttonRect.w >= 16) {
            vec2i grabPos = {buttonRect.x + buttonRect.w / 2,
                             bar.y + bar.h / 2};
            Draw::sprite(scrollbar.grab, grabPos, 0);
        }
    }
    if (focus) {
        scrollbar.pressed.draw(buttonRect);
    } else if (hover) {
        scrollbar.hover.draw(buttonRect);
    }
}

void ApplyScrollOffset(int& offset, int page, int scroll, bool up) {
    int delta = max(1, page / 8);
    if (up) delta = -delta;
    offset = max(0, min(offset + delta, scroll - page));
}

};  // anonymous namespace.

// ================================================================================================
// WgScrollbar

WgScrollbar::~WgScrollbar() {}

WgScrollbar::WgScrollbar(GuiContext* gui)
    : GuiWidget(gui), scrollbar_action_(0), scrollbar_grab_position_(0) {
    scrollbar_end_ = 1;
    scrollbar_page_ = 1;
}

void WgScrollbar::setEnd(int size) { scrollbar_end_ = size; }

void WgScrollbar::setPage(int size) { scrollbar_page_ = size; }

void WgScrollbar::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            uint32_t action = GetScrollbarActionAtPosition(evt.x, evt.y);
            if (action) {
                startCapturingMouse();
            }
            if (action == ACT_HOVER_BUTTON_H) {
                auto button = GetButton(rect_.w, scrollbar_end_,
                                        scrollbar_page_, value.get());
                scrollbar_grab_position_ = evt.x - button.pos - rect_.x;
                scrollbar_action_ = ACT_DRAGGING_H;
            }
            if (action == ACT_HOVER_BUTTON_V) {
                auto button = GetButton(rect_.h, scrollbar_end_,
                                        scrollbar_page_, value.get());
                scrollbar_grab_position_ = evt.y - button.pos - rect_.y;
                scrollbar_action_ = ACT_DRAGGING_V;
            }
        }
        evt.setHandled();
    }
}

void WgScrollbar::onMouseRelease(MouseRelease& evt) {
    stopCapturingMouse();
    scrollbar_action_ = 0;
}

void WgScrollbar::onTick() {
    if (scrollbar_action_ == ACT_DRAGGING_H) {
        int buttonPos =
            gui_->getMousePos().x - rect_.x - scrollbar_grab_position_;
        ScrollbarUpdateValue(
            GetScroll(rect_.w, scrollbar_end_, scrollbar_page_, buttonPos));
    } else if (scrollbar_action_ == ACT_DRAGGING_V) {
        int buttonPos =
            gui_->getMousePos().y - rect_.y - scrollbar_grab_position_;
        ScrollbarUpdateValue(
            GetScroll(rect_.h, scrollbar_end_, scrollbar_page_, buttonPos));
    }
    GuiWidget::onTick();
}

void WgScrollbar::onDraw() {
    vec2i mpos = gui_->getMousePos();
    uint32_t action = GetScrollbarActionAtPosition(mpos.x, mpos.y);

    if (isVertical()) {
        auto button =
            GetButton(rect_.h, scrollbar_end_, scrollbar_page_, value.get());
        DrawScrollbar(rect_, button, true, action == ACT_HOVER_BUTTON_V,
                      action == ACT_DRAGGING_V);
    } else {
        auto button =
            GetButton(rect_.w, scrollbar_end_, scrollbar_page_, value.get());
        DrawScrollbar(rect_, button, false, action == ACT_HOVER_BUTTON_H,
                      action == ACT_DRAGGING_H);
    }
}

void WgScrollbar::ScrollbarUpdateValue(int v) {
    double prev = value.get();
    v = max(0, min(scrollbar_end_, v));
    value.set(v);
    if (value.get() != prev) onChange.call();
}

uint32_t WgScrollbar::GetScrollbarActionAtPosition(int x, int y) {
    if (scrollbar_action_) return scrollbar_action_;
    if (!IsInside(rect_, x, y)) return ACT_NONE;

    x -= rect_.x;
    y -= rect_.y;

    if (isVertical()) {
        auto button =
            GetButton(rect_.h, scrollbar_end_, scrollbar_page_, value.get());
        if (y < button.pos) return ACT_HOVER_BUMP_UP;
        if (y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
        return ACT_HOVER_BUMP_DOWN;
    } else {
        auto button =
            GetButton(rect_.w, scrollbar_end_, scrollbar_page_, value.get());
        if (x < button.pos) return ACT_HOVER_BUMP_LEFT;
        if (x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
        return ACT_HOVER_BUMP_RIGHT;
    }

    return ACT_NONE;
}

WgScrollbarV::WgScrollbarV(GuiContext* gui) : WgScrollbar(gui) { width_ = 14; }

bool WgScrollbarV::isVertical() const { return true; }

WgScrollbarH::WgScrollbarH(GuiContext* gui) : WgScrollbar(gui) { height_ = 14; }

bool WgScrollbarH::isVertical() const { return false; }

// ================================================================================================
// WgScrollRegion.

static const int SCROLLBAR_SIZE = 14;

WgScrollRegion::~WgScrollRegion() {}

WgScrollRegion::WgScrollRegion(GuiContext* gui)
    : GuiWidget(gui),
      scroll_type_horizontal_(0),
      scroll_type_vertical_(0),
      is_horizontal_scrollbar_active_(0),
      is_vertical_scrollbar_active_(0),
      scroll_region_action_(0),
      scroll_region_grab_position_(0),
      scroll_width_(0),
      scroll_height_(0),
      scroll_position_x_(0),
      scroll_position_y_(0) {}

void WgScrollRegion::onMouseScroll(MouseScroll& evt) {
    if (is_vertical_scrollbar_active_) {
        if (IsInside(rect_, evt.x, evt.y) && !evt.handled) {
            ApplyScrollOffset(scroll_position_y_, getViewHeight(),
                              scroll_height_, evt.up);
            evt.handled = true;
        }
    }
}

void WgScrollRegion::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            uint32_t action = getScrollRegionActionAt_(evt.x, evt.y);
            if (action) {
                startCapturingMouse();
            }
            if (action == ACT_HOVER_BUTTON_H) {
                int w = getViewWidth();
                auto button =
                    GetButton(w, scroll_width_, w, scroll_position_x_);
                scroll_region_grab_position_ = evt.x - button.pos - rect_.x;
                scroll_region_action_ = ACT_DRAGGING_H;
            }
            if (action == ACT_HOVER_BUTTON_V) {
                int h = getViewHeight();
                auto button =
                    GetButton(h, scroll_height_, h, scroll_position_y_);
                scroll_region_grab_position_ = evt.y - button.pos - rect_.y;
                scroll_region_action_ = ACT_DRAGGING_V;
            }
        }
        evt.setHandled();
    }
}

void WgScrollRegion::onMouseRelease(MouseRelease& evt) {
    stopCapturingMouse();
    scroll_region_action_ = 0;
}

void WgScrollRegion::onTick() {
    PreTick();
    PostTick();
}

void WgScrollRegion::onDraw() {
    vec2i mpos = gui_->getMousePos();
    uint32_t action = getScrollRegionActionAt_(mpos.x, mpos.y);

    int viewW = getViewWidth();
    int viewH = getViewHeight();

    if (is_horizontal_scrollbar_active_) {
        recti bar = {rect_.x, rect_.y + rect_.h - SCROLLBAR_SIZE, viewW,
                     SCROLLBAR_SIZE};
        auto button =
            GetButton(viewW, scroll_width_, viewW, scroll_position_x_);
        DrawScrollbar(bar, button, false, action == ACT_HOVER_BUTTON_H,
                      action == ACT_DRAGGING_H);
    }
    if (is_vertical_scrollbar_active_) {
        recti bar = {rect_.x + rect_.w - SCROLLBAR_SIZE, rect_.y,
                     SCROLLBAR_SIZE, viewH};
        auto button =
            GetButton(viewH, scroll_height_, viewH, scroll_position_y_);
        DrawScrollbar(bar, button, true, action == ACT_HOVER_BUTTON_V,
                      action == ACT_DRAGGING_V);
    }
}

void WgScrollRegion::setScrollType(ScrollType h, ScrollType v) {
    scroll_type_horizontal_ = h;
    scroll_type_vertical_ = v;
}

void WgScrollRegion::setScrollW(int width) { scroll_width_ = max(0, width); }

void WgScrollRegion::setScrollH(int height) { scroll_height_ = max(0, height); }

int WgScrollRegion::getViewWidth() const {
    return rect_.w - is_vertical_scrollbar_active_ * SCROLLBAR_SIZE;
}

int WgScrollRegion::getViewHeight() const {
    return rect_.h - is_horizontal_scrollbar_active_ * SCROLLBAR_SIZE;
}

int WgScrollRegion::getScrollWidth() const { return scroll_width_; }

int WgScrollRegion::getScrollHeight() const { return scroll_height_; }

void WgScrollRegion::PreTick() {
    vec2i mpos = gui_->getMousePos();

    int sh = scroll_type_horizontal_;
    int sv = scroll_type_vertical_;

    is_horizontal_scrollbar_active_ =
        (sh == SCROLL_ALWAYS ||
         (sh == SCROLL_WHEN_NEEDED && scroll_width_ > rect_.w));
    is_vertical_scrollbar_active_ =
        (sv == SCROLL_ALWAYS ||
         (sv == SCROLL_WHEN_NEEDED && scroll_height_ > rect_.h));

    if (scroll_region_action_ == ACT_DRAGGING_H) {
        int w = getViewWidth();
        int buttonPos =
            gui_->getMousePos().x - rect_.x - scroll_region_grab_position_;
        scroll_position_x_ = GetScroll(w, scroll_width_, w, buttonPos);
    } else if (scroll_region_action_ == ACT_DRAGGING_V) {
        int h = getViewHeight();
        int buttonPos =
            gui_->getMousePos().y - rect_.y - scroll_region_grab_position_;
        scroll_position_y_ = GetScroll(h, scroll_height_, h, buttonPos);
    }

    if (!IsInside(recti{rect_.x, rect_.y, getViewWidth(), getViewHeight()},
                  mpos.x, mpos.y)) {
        blockMouseOver();
    }
}

void WgScrollRegion::PostTick() {
    unblockMouseOver();

    GuiWidget::onTick();
}

uint32_t WgScrollRegion::getScrollRegionActionAt_(int x, int y) {
    if (scroll_region_action_) return scroll_region_action_;

    if (!IsInside(rect_, x, y)) return ACT_NONE;

    x -= rect_.x;
    y -= rect_.y;

    int viewW = getViewWidth();
    int viewH = getViewHeight();
    if (is_vertical_scrollbar_active_ && x >= viewW && y < viewH) {
        auto button =
            GetButton(viewH, scroll_height_, viewH, scroll_position_y_);
        if (y < button.pos) return ACT_HOVER_BUMP_UP;
        if (y < button.pos + button.size) return ACT_HOVER_BUTTON_V;
        return ACT_HOVER_BUMP_DOWN;
    }

    if (is_horizontal_scrollbar_active_ && y >= viewH && x < viewW) {
        auto button =
            GetButton(viewW, scroll_width_, viewW, scroll_position_x_);
        if (x < button.pos) return ACT_HOVER_BUMP_LEFT;
        if (x < button.pos + button.size) return ACT_HOVER_BUTTON_H;
        return ACT_HOVER_BUMP_RIGHT;
    }

    return ACT_NONE;
}

void WgScrollRegion::ClampScrollPositions() {
    scroll_position_x_ =
        max(0, min(scroll_position_x_, scroll_width_ - getViewWidth()));
    scroll_position_y_ =
        max(0, min(scroll_position_y_, scroll_height_ - getViewHeight()));
}

};  // namespace Vortex
