#include <Core/GuiDialog.h>
#include <Core/GuiContext.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>
#include <Core/GuiContext.h>
#include <Core/GuiWidget.h>

namespace Vortex {

#define MY_GUI ((GuiContextImpl*)gui_)

static const int FRAME_TITLEBAR_H = 24;
static const int FRAME_PADDING = 4;
static const int FRAME_RESIZE_BORDER = 5;
static const int FRAME_BUTTON_W = 16;

// ================================================================================================
// Dialog Frame implementation.

DialogData::~DialogData() {
    delete dialog_ptr_;
    delete current_action_;
}

DialogData::DialogData(GuiContext* gui, GuiDialog* dialog)
    : GuiWidget(gui),
      dialog_ptr_(dialog),
      gui_(gui),
      is_pinnable_(true),
      is_closeable_(true),
      is_minimizable_(true),
      is_horizontally_resizable_(false),
      is_vertically_resizable_(false),
      request_close_(false),
      request_pin_(false),
      request_minimize_(false),
      request_move_to_top_(false),
      pinned_state_(false),
      minimized_state_(false),
      min_size_({0, 0}),
      max_size_({INT_MAX, INT_MAX}),
      pinned_position_({0, 0}),
      current_action_(nullptr) {
    rect_ = {16, 16, 256, 256};
    MY_GUI->addDialog(this);
}

// ================================================================================================
// DialogData :: frame dragging.

static DialogData::DragAction* StartDrag(recti r, int mx, int my) {
    auto out = new DialogData::DragAction;

    out->type = DialogData::ACT_DRAG;
    out->offset.x = r.x - mx;
    out->offset.y = r.y - my;

    return out;
}

void DialogData::HandleDrag() {
    if (current_action_ && current_action_->type == ACT_DRAG) {
        auto action = static_cast<DragAction*>(current_action_);
        vec2i mpos = gui_->getMousePos();

        rect_.x = mpos.x + action->offset.x;
        rect_.y = mpos.y + action->offset.y;
    }
}

// ================================================================================================
// DialogData :: frame resizing.

static DialogData::ResizeAction* StartResize(DialogData::ActionType type,
                                             recti r, int mx, int my) {
    auto out = new DialogData::ResizeAction;
    out->type = type;

    static const int dirH[] = {0, -1, 0, 1, 0, -1, 1, -1, 1};
    static const int dirV[] = {0, 0, -1, 0, 1, 1, 1, -1, -1};

    out->dirH = dirH[type - DialogData::ACT_RESIZE];
    out->dirV = dirV[type - DialogData::ACT_RESIZE];

    if (out->dirH < 0) {
        out->anchor.x = r.x + r.w;
        out->offset.x = mx - r.x;
    } else {
        out->anchor.x = r.x;
        out->offset.x = out->anchor.x + r.w - mx;
    }

    if (out->dirV < 0) {
        out->anchor.y = r.y + r.h;
        out->offset.y = my - r.y;
    } else {
        out->anchor.y = r.y;
        out->offset.y = out->anchor.y + r.h - my;
    }

    return out;
}

void DialogData::HandleResize() {
    if (current_action_ && current_action_->type >= ACT_RESIZE) {
        auto action = static_cast<ResizeAction*>(current_action_);
        vec2i mpos = gui_->getMousePos();
        vec2i anchor = action->anchor;

        if (action->dirH < 0) {
            rect_.w = anchor.x - mpos.x + action->offset.x;
            rect_.x = anchor.x - rect_.w;

        } else if (action->dirH > 0) {
            rect_.x = anchor.x;
            rect_.w = mpos.x - anchor.x + action->offset.x;
        }

        if (action->dirV < 0) {
            rect_.h = anchor.y - mpos.y + action->offset.y;
            rect_.y = anchor.y - rect_.h;

        } else if (action->dirV > 0) {
            rect_.y = anchor.y;
            rect_.h = mpos.y - anchor.y + action->offset.y;
        }
    }
}

// ================================================================================================
// DialogData :: event handling.

void DialogData::onMousePress(MousePress& evt) {
    FinishActions();
    stopCapturingMouse();
    if (isMouseOver()) {
        if (evt.button == Mouse::LMB && evt.unhandled()) {
            auto actionType = GetAction(evt.x, evt.y);

            if (actionType != ACT_NONE || IsInside(rect_, evt.x, evt.y)) {
                request_move_to_top_ = true;
            }
            if (actionType == ACT_DRAG) {
                startCapturingMouse();
                current_action_ = StartDrag(rect_, evt.x, evt.y);
            } else if (actionType >= ACT_RESIZE) {
                startCapturingMouse();
                current_action_ = StartResize(actionType, rect_, evt.x, evt.y);
            } else if (actionType == ACT_CLOSE) {
                request_close_ = true;
            } else if (actionType == ACT_MINIMIZE) {
                request_minimize_ = true;
            } else if (actionType == ACT_PIN) {
                request_pin_ = true;
            }
        }
        evt.setHandled();
    }
}

void DialogData::onMouseRelease(MouseRelease& evt) {
    FinishActions();
    stopCapturingMouse();
}

// ================================================================================================
// DialogData :: update functions.

void DialogData::ClampRect() {
    recti bounds =
        Shrink(gui_->getView(), FRAME_PADDING, FRAME_PADDING + FRAME_TITLEBAR_H,
               FRAME_PADDING, FRAME_PADDING);

    if (pinned_state_) {
        rect_.x = pinned_position_.x;
        rect_.y = pinned_position_.y;
    }

    if (current_action_ && current_action_->type >= ACT_RESIZE) {
        auto a = static_cast<ResizeAction*>(current_action_);
        if (a->dirH < 0) rect_.w = min(rect_.w, a->anchor.x - bounds.x);
        if (a->dirH > 0)
            rect_.w = min(rect_.w, bounds.x + bounds.w - a->anchor.x);
        if (a->dirV < 0) rect_.h = min(rect_.h, a->anchor.y - bounds.y);
        if (a->dirV > 0)
            rect_.h = min(rect_.h, bounds.y + bounds.h - a->anchor.y);
    }

    rect_.w = max(min_size_.x, min(max_size_.x, min(bounds.w, rect_.w)));
    rect_.h = max(min_size_.y, min(max_size_.y, min(bounds.h, rect_.h)));

    if (current_action_ && current_action_->type >= ACT_RESIZE) {
        auto a = static_cast<ResizeAction*>(current_action_);
        if (a->dirH < 0) rect_.x = a->anchor.x - rect_.w;
        if (a->dirV < 0) rect_.y = a->anchor.y - rect_.h;
    }

    int marginH = minimized_state_ ? (FRAME_PADDING * -2) : rect_.h;
    rect_.x = max(min(rect_.x, bounds.x + bounds.w - rect_.w), bounds.x);
    rect_.y = max(min(rect_.y, bounds.y + bounds.h - marginH), bounds.y);
}

void DialogData::arrange() {
    if (request_minimize_) {
        request_minimize_ = false;
        minimized_state_ = !minimized_state_;
        if (minimized_state_) pinned_state_ = false;
    }
    if (request_pin_) {
        request_pin_ = false;
        pinned_state_ = !pinned_state_;
        if (pinned_state_) {
            minimized_state_ = false;
            pinned_position_ = Pos(rect_);
        }
    }

    dialog_ptr_->onUpdateSize();
    HandleResize();
    HandleDrag();
    ClampRect();
}

void DialogData::UpdateMouseCursor() {
    if (!isMouseOver()) return;

    vec2i mpos = gui_->getMousePos();
    Cursor::Icon icon = Cursor::ARROW;
    switch (GetAction(mpos.x, mpos.y)) {
        case ACT_RESIZE_L:
        case ACT_RESIZE_R:
            icon = Cursor::SIZE_WE;
            break;
        case ACT_RESIZE_B:
        case ACT_RESIZE_T:
            icon = Cursor::SIZE_NS;
            break;
        case ACT_RESIZE_TL:
        case ACT_RESIZE_BR:
            icon = Cursor::SIZE_NWSE;
            break;
        case ACT_RESIZE_TR:
        case ACT_RESIZE_BL:
            icon = Cursor::SIZE_NESW;
            break;
        default:
            break;
    };

    GuiMain::setCursorIcon(icon);
}

void DialogData::tick() {
    if (!minimized_state_) {
        dialog_ptr_->onTick();
    }

    vec2i mpos = gui_->getMousePos();
    recti rect = dialog_ptr_->getOuterRect();
    rect = Expand(rect, FRAME_RESIZE_BORDER);

    if (IsInside(rect, mpos.x, mpos.y)) {
        captureMouseOver();
    }

    handleInputs(gui_->getEvents());

    UpdateMouseCursor();
}

void DialogData::draw() {
    auto& dlg = GuiDraw::getDialog();

    vec2i mpos = gui_->getMousePos();
    auto action = GetAction(mpos.x, mpos.y);
    recti r = dialog_ptr_->getOuterRect();

    // Draw the frame.
    if (pinned_state_) {
        dlg.frame.draw({r.x, r.y, r.w, r.h}, 0);
        recti tb = Shrink(r, 2);
        tb.h = FRAME_TITLEBAR_H - 2;
        Draw::fill(tb, RGBAtoColor32(0, 0, 0, 48));
    } else {
        if (!minimized_state_) {
            dlg.titlebar.draw({r.x, r.y, r.w, FRAME_TITLEBAR_H + 8},
                              TileRect2::T);
            dlg.frame.draw(
                {r.x, r.y + FRAME_TITLEBAR_H, r.w, r.h - FRAME_TITLEBAR_H},
                TileRect2::B);
        } else {
            dlg.titlebar.draw({r.x, r.y, r.w, FRAME_TITLEBAR_H});
        }
    }

    // Draw the titlebar buttons.
    auto& icons = GuiDraw::getIcons();
    int titleTextW = r.w;
    int buttonX = RightX(r) - FRAME_BUTTON_W / 2 - 4;
    if (!pinned_state_) {
        if (is_closeable_) {
            uint32_t col = Color32((action == ACT_CLOSE) ? 200 : 100);
            Draw::sprite(icons.cross, {buttonX, r.y + FRAME_TITLEBAR_H / 2},
                         col);
            buttonX -= FRAME_BUTTON_W;
            titleTextW -= FRAME_BUTTON_W;
            if (action == ACT_CLOSE) {
                GuiMain::setTooltip("Close the dialog");
            }
        }
        if (is_minimizable_) {
            auto& tex = minimized_state_ ? icons.plus : icons.minus;
            uint32_t col = Color32((action == ACT_MINIMIZE) ? 200 : 100);
            Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
            buttonX -= FRAME_BUTTON_W;
            titleTextW -= FRAME_BUTTON_W;
            if (action == ACT_MINIMIZE) {
                GuiMain::setTooltip(minimized_state_
                                        ? "Show the dialog contents"
                                        : "Hide the dialog contents");
            }
        }
    }
    if (is_pinnable_) {
        auto& tex = pinned_state_ ? icons.unpin : icons.pin;
        uint32_t col = Color32((action == ACT_PIN) ? 200 : 100);
        Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
        buttonX -= FRAME_BUTTON_W;
        titleTextW -= FRAME_BUTTON_W;
        if (action == ACT_PIN) {
            GuiMain::setTooltip(pinned_state_ ? "Unpin the dialog"
                                              : "Pin the dialog");
        }
    }

    // Draw the title text.
    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    Text::arrange(Text::MC, style, titleTextW - 8, dialog_title_.c_str());
    Text::draw({r.x, r.y, titleTextW, FRAME_TITLEBAR_H});

    // Draw inner dialog area.
    if (!minimized_state_) {
        dialog_ptr_->onDraw();
    }
}

DialogData::ActionType DialogData::GetAction(int x, int y) const {
    if (current_action_) {
        return current_action_->type;
    } else {
        recti rect = dialog_ptr_->getOuterRect();
        if (IsInside(rect, x, y)) {
            // Titlebar buttons.
            int dx = x - rect.x - rect.w + FRAME_BUTTON_W, dy = y - rect.y;
            if (dy < FRAME_TITLEBAR_H) {
                if (!pinned_state_) {
                    if (is_closeable_) {
                        if (dx >= 0) return ACT_CLOSE;
                        dx += FRAME_BUTTON_W;
                    }
                    if (is_minimizable_) {
                        if (dx >= 0) return ACT_MINIMIZE;
                        dx += FRAME_BUTTON_W;
                    }
                }
                if (is_pinnable_) {
                    if (dx >= 0) return ACT_PIN;
                    dx += FRAME_BUTTON_W;
                }
                return pinned_state_ ? ACT_NONE : ACT_DRAG;
            }
        }

        // Horizontal and vertical resizing.
        rect = Expand(rect, FRAME_RESIZE_BORDER);
        if (IsInside(rect, x, y) && !pinned_state_) {
            int resizeH = 0, resizeV = 0;
            int dx = x - rect.x, dy = y - rect.y;
            int border = FRAME_RESIZE_BORDER * 2;
            if (is_horizontally_resizable_) {
                resizeH = (dx > rect.w - border) - (dx < border);
            }
            if (is_vertically_resizable_) {
                resizeV = (dy > rect.h - border) - (dy < border);
            }
            if (resizeH != 0 || resizeV != 0) {
                int index = resizeV * 3 + resizeH + 4;
                ActionType actions[] = {
                    ACT_RESIZE_TL, ACT_RESIZE_T, ACT_RESIZE_TR,
                    ACT_RESIZE_L,  ACT_RESIZE,   ACT_RESIZE_R,
                    ACT_RESIZE_BL, ACT_RESIZE_B, ACT_RESIZE_BR};
                return actions[index];
            }
        }
    }
    return ACT_NONE;
}

void DialogData::FinishActions() {
    delete current_action_;
    current_action_ = nullptr;
}

// ================================================================================================
// GuiDialog :: implementation.

#define DATA ((DialogData*)data_)

GuiDialog::GuiDialog(GuiContext* gui) : data_(new DialogData(gui, this)) {}

GuiDialog::~GuiDialog() = default;

void GuiDialog::onUpdateSize() {}

void GuiDialog::onArrange(recti r) {}

void GuiDialog::onTick() {}

void GuiDialog::onDraw() {}

void GuiDialog::requestClose() { DATA->request_close_ = true; }

void GuiDialog::requestPin() { DATA->request_pin_ = true; }

void GuiDialog::setPosition(int x, int y) {
    DATA->rect_.x = x;
    DATA->rect_.y = y;
}

void GuiDialog::setTitle(const std::string& str) { DATA->dialog_title_ = str; }

void GuiDialog::setWidth(int w) { DATA->rect_.w = w; }

void GuiDialog::setHeight(int h) { DATA->rect_.h = h; }

void GuiDialog::setMinimumWidth(int w) { DATA->min_size_.x = max(0, w); }

void GuiDialog::setMinimumHeight(int h) { DATA->min_size_.y = max(0, h); }

void GuiDialog::setMaximumWidth(int w) { DATA->max_size_.x = max(0, w); }

void GuiDialog::setMaximumHeight(int h) { DATA->max_size_.y = max(0, h); }

void GuiDialog::setCloseable(bool enable) { DATA->is_closeable_ = true; }

void GuiDialog::setMinimizable(bool enable) { DATA->is_minimizable_ = true; }

void GuiDialog::setResizeable(bool horizontal, bool vertical) {
    DATA->is_horizontally_resizable_ = horizontal;
    DATA->is_vertically_resizable_ = vertical;
}

GuiContext* GuiDialog::getGui() const { return DATA->gui_; }

recti GuiDialog::getOuterRect() const {
    int pad = FRAME_PADDING;
    recti r = Expand(DATA->rect_, pad, pad + FRAME_TITLEBAR_H, pad, pad);
    if (DATA->minimized_state_) r.h = FRAME_TITLEBAR_H + pad;
    return r;
}

recti GuiDialog::getInnerRect() const { return DATA->rect_; }

bool GuiDialog::isPinned() const { return DATA->pinned_state_; }

};  // namespace Vortex
