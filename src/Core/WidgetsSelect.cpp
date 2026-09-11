#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

#include <System/System.h>

namespace Vortex {

// ================================================================================================
// WgSelectList

#define ITEM_H gSystem->applyScaleFactor(18)
#define ICON_W gSystem->applyScaleFactor(8)
#define ICON_SPACE gSystem->applyScaleFactor(10)

WgSelectList::~WgSelectList() {
    clearItems();

    delete scrollbar_;
}

WgSelectList::WgSelectList(GuiContext* gui) : GuiWidget(gui) {
    scrollbar_ = new WgScrollbarV(gui_);
    scrollbar_->value.bind(&scroll_position_);
}

void WgSelectList::hideBackground() { show_background_ = 0; }

void WgSelectList::alignItemsLeft() { align_items_left_ = 1; }

void WgSelectList::addItem(const std::string& text) {
    selectlist_items_.emplace_back(text);
    selectlist_items_right_.emplace_back();
}

void WgSelectList::addItem(const std::string& text,
                           const std::string& rightText) {
    selectlist_items_.emplace_back(text);
    selectlist_items_right_.emplace_back(rightText);
}

void WgSelectList::clearItems() {
    selectlist_items_.clear();
    selectlist_items_right_.clear();
}

void WgSelectList::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && !evt.handled) {
            startCapturingMouse();

            // Handle interaction with the item list.
            int i = HoveredItem(evt.x, evt.y);
            if (i >= 0) {
                is_interacted_ = 1;
                if (value.get() != i) {
                    value.set(i);
                    onChange.call();
                }
            }
        }
        evt.handled = true;
    }
}

void WgSelectList::onMouseRelease(MouseRelease& evt) {
    if (isCapturingMouse() && evt.button == Mouse::LMB) {
        stopCapturingMouse();
    }
}

void WgSelectList::onMouseScroll(MouseScroll& evt) {
    if (isMouseOver() && HasScrollBar() && !evt.handled) {
        scroll(evt.up);
        evt.handled = true;
    }
}

void WgSelectList::scroll(bool up) {
    if (HasScrollBar()) {
        int end = selectlist_items_.size() * ITEM_H - ItemRect().h;
        int delta = up ? -ITEM_H : ITEM_H;
        scroll_position_ = std::clamp(scroll_position_ + delta, 0, end);
    }
}

void WgSelectList::onArrange(recti r) {
    GuiWidget::onArrange(r);

    int w = scrollbar_->getWidth();
    scrollbar_->arrange({r.x + r.w - w - 2, r.y + 3, w, r.h - 6});
}

void WgSelectList::onTick() {
    scrollbar_->setEnd(selectlist_items_.size() * ITEM_H);
    scrollbar_->setPage(ItemRect().h);
    scrollbar_->tick();

    GuiWidget::onTick();
}

void WgSelectList::onDraw() {
    auto& misc = GuiDraw::getMisc();
    auto& textbox = GuiDraw::getTextBox();

    // Draw the list background box.
    recti r = rect_;
    if (show_background_) {
        textbox.base.draw(r);
    }

    r = ItemRect();
    Renderer::pushScissorRect(r.x, r.y, r.w, r.h);

    // Highlight the currently selected item.
    int item = value.get(), numItems = selectlist_items_.size();
    if (item >= 0 && item < numItems) {
        misc.imgSelect.draw(
            {r.x, r.y - scroll_position_ + item * ITEM_H, r.w, ITEM_H});
    }

    // Highlight the mouse over item.
    if (isMouseOver()) {
        vec2i mpos = gui_->getMousePos();
        int mo = HoveredItem(mpos.x, mpos.y);
        if (mo != item && mo >= 0 && mo < numItems) {
            uint32_t col = Color32(255, 255, 255, isEnabled() ? 128 : 0);
            misc.imgSelect.draw(
                {r.x, r.y - scroll_position_ + mo * ITEM_H, r.w, ITEM_H}, col);
        }
    }

    // Draw the item texts.
    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    for (int i = 0, ty = r.y - scroll_position_; i < numItems;
         ++i, ty += ITEM_H) {
        if (ty > r.y - ITEM_H && ty < r.y + r.h) {
            // The right hand text is drawn first, so a long name is cut
            // off by the ellipsis rather than running over it.
            if (selectlist_items_right_[i].length()) {
                Text::arrange(Text::MR, style,
                              selectlist_items_right_[i].c_str());
                Text::draw({r.x + 6, ty, r.w - 12, ITEM_H});
            }
            Text::arrange(align_items_left_ ? Text::ML : Text::MC, style,
                          selectlist_items_[i].c_str());
            Text::draw({r.x + 6, ty, r.w - 8, ITEM_H});
        }
    }

    Renderer::popScissorRect();

    // Draw the scrollbar.
    if (HasScrollBar()) {
        scrollbar_->draw();
    }
}

int WgSelectList::HoveredItem(int x, int y) {
    recti r = ItemRect();
    if (x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h) {
        int i = (y - r.y + scroll_position_) / ITEM_H;
        if (i >= 0 && i < selectlist_items_.size()) return i;
    }
    return -1;
}

bool WgSelectList::HasScrollBar() const {
    return (selectlist_items_.size() * ITEM_H) > (rect_.h - 6);
}

recti WgSelectList::ItemRect() const {
    recti r = rect_;
    if (HasScrollBar()) r.w -= scrollbar_->getWidth() + 1;
    return {r.x + 3, r.y + 3, r.w - 6, r.h - 6};
}

#undef ITEM_H

// ================================================================================================
// WgDroplist

WgDroplist::~WgDroplist() {
    clearItems();
    CloseDroplist();
}

WgDroplist::WgDroplist(GuiContext* gui) : GuiWidget(gui) {
    selectlist_widget_ = nullptr;
}

void WgDroplist::onMousePress(MousePress& evt) {
    if (selectlist_widget_) {
        recti r = selectlist_widget_->getRect();
        if (evt.button == Mouse::RMB || !IsInside(r, evt.x, evt.y)) {
            CloseDroplist();
            evt.handled = true;
        }
    } else if (isMouseOver()) {
        int numItems = droplist_items_.size();
        if (isEnabled() && numItems && evt.button == Mouse::LMB &&
            !evt.handled) {
            int h = std::min(numItems * gSystem->applyScaleFactor(18) + 8,
                             gSystem->applyScaleFactor(128));
            recti r = {rect_.x, rect_.y + rect_.h, rect_.w, h};
            selectlist_widget_ = new WgSelectList(gui_);
            selectlist_widget_->setHeight(h);
            selectlist_widget_->onArrange(r);
            selectlist_widget_->hideBackground();

            for (int i = 0; i < numItems; ++i) {
                selectlist_widget_->addItem(droplist_items_[i].c_str());
            }

            selected_index_ = value.get();
            selectlist_widget_->value.bind(&selected_index_);

            startCapturingMouse();
            startCapturingFocus();
        }
        evt.handled = true;
    }
}

void WgDroplist::onMouseRelease(MouseRelease& evt) { stopCapturingMouse(); }

void WgDroplist::onMouseScroll(MouseScroll& evt) {
    if (selectlist_widget_ && !evt.handled) {
        selectlist_widget_->scroll(evt.up);
        evt.handled = true;
    }
}

void WgDroplist::onArrange(recti r) {
    GuiWidget::onArrange(r);
    if (selectlist_widget_) {
        selectlist_widget_->onArrange({rect_.x, rect_.y + rect_.h, rect_.w,
                                       selectlist_widget_->getSize().y});
    }
}

void WgDroplist::onTick() {
    GuiWidget::onTick();
    if (selectlist_widget_) {
        selectlist_widget_->tick();
        if (selectlist_widget_->interacted()) {
            if (selected_index_ != value.get()) {
                value.set(selected_index_);
                onChange.call();
            }
            CloseDroplist();
        }
    }
}

void WgDroplist::onDraw() {
    auto& button = GuiDraw::getButton();
    auto& misc = GuiDraw::getMisc();
    auto& icons = GuiDraw::getIcons();

    int numItems = droplist_items_.size();

    // Draw the list of items.
    if (selectlist_widget_) {
        recti r = Expand(rect_, 1);
        r.h += selectlist_widget_->getHeight();
        auto& textbox = GuiDraw::getTextBox();
        textbox.base.draw(r);
        selectlist_widget_->draw();
    }

    // Draw the button graphic.
    recti r = rect_;
    button.base.draw(rect_);
    if (isEnabled())
        Draw::sprite(icons.arrow,
                     {r.x + r.w - ICON_SPACE, r.y + r.h / 2 - ICON_W / 2,
                      ICON_W, ICON_W},
                     Draw::ROT_90);

    // Draw the selected item text.
    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    if (!isEnabled()) style.textColor = misc.colDisabled;

    int item = value.get();
    if (item >= 0 && item < numItems) {
        Text::arrange(Text::MC, style, rect_.w - 18,
                      droplist_items_[item].c_str());
        Text::draw({r.x + 6, r.y, r.w - 18, r.h});
    }

    // Draw interaction effects.
    if (isCapturingMouse()) {
        button.pressed.draw(rect_);
    } else if (isMouseOver()) {
        button.hover.draw(rect_);
    }
}

void WgDroplist::clearItems() { droplist_items_.clear(); }

void WgDroplist::addItem(const std::string& text) {
    droplist_items_.emplace_back(text);
}

void WgDroplist::CloseDroplist() {
    if (selectlist_widget_) {
        delete selectlist_widget_;
        selectlist_widget_ = nullptr;
        stopCapturingFocus();
    }
}

// ================================================================================================
// WgCycleButton

WgCycleButton::~WgCycleButton() { clearItems(); }

WgCycleButton::WgCycleButton(GuiContext* gui) : GuiWidget(gui) {}

void WgCycleButton::addItem(const std::string& text) {
    cycle_items_.emplace_back(text);
}

void WgCycleButton::clearItems() { cycle_items_.clear(); }

void WgCycleButton::onMousePress(MousePress& evt) {
    int numItems = cycle_items_.size();
    if (isMouseOver()) {
        if (numItems > 1 && isEnabled() && evt.button == Mouse::LMB &&
            !evt.handled) {
            stopCapturingText();
            startCapturingMouse();
            if (evt.x > CenterX(rect_)) {
                value.set((value.get() + 1) % numItems);
            } else {
                value.set((value.get() + numItems - 1) % numItems);
            }
            onChange.call();
        }
        evt.handled = true;
    }
}

void WgCycleButton::onMouseRelease(MouseRelease& evt) {
    if (isCapturingMouse() && evt.button == Mouse::LMB) {
        stopCapturingMouse();
    }
}

void WgCycleButton::onDraw() {
    auto& misc = GuiDraw::getMisc();
    auto& icons = GuiDraw::getIcons();
    auto& button = GuiDraw::getButton();

    int numItems = cycle_items_.size(), item = value.get();

    // Draw the button graphic.
    recti r = rect_;
    button.base.draw(r);

    // Draw the arrow graphics.
    int mouseX = gui_->getMousePos().x - CenterX(rect_);
    if (!isMouseOver()) mouseX = 0;

    uint32_t colL =
        (isEnabled() && mouseX < 0) ? Colors::white : misc.colDisabled;
    Draw::sprite(
        icons.arrow,
        {r.x + ICON_SPACE - ICON_W, r.y + r.h / 2 - ICON_W / 2, ICON_W, ICON_W},
        colL, Draw::FLIP_H);

    uint32_t colR =
        (isEnabled() && mouseX > 0) ? Colors::white : misc.colDisabled;
    Draw::sprite(
        icons.arrow,
        {r.x + r.w - ICON_SPACE, r.y + r.h / 2 - ICON_W / 2, ICON_W, ICON_W},
        colR);

    // Draw the button text.
    if (item >= 0 && item < numItems) {
        TextStyle style;
        style.textFlags = Text::MARKUP | Text::ELLIPSES;
        if (!isEnabled()) style.textColor = misc.colDisabled;

        Renderer::pushScissorRect(Shrink(rect_, 2));
        Text::arrange(Text::MC, style, cycle_items_[item].c_str());
        Text::draw({r.x + 2, r.y, r.w - 4, r.h});
        Renderer::popScissorRect();
    }

    // Draw interaction effects.
    if (isCapturingMouse()) {
        button.pressed.draw(rect_);
    } else if (isMouseOver()) {
        button.hover.draw(rect_);
    }
}

};  // namespace Vortex
