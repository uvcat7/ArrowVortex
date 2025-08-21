#include <Core/Widgets.h>
#include <Core/Gui.h>

#include <math.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/GuiDraw.h>
#include <Core/Shader.h>

#include <algorithm>

constexpr int MaximumPastedTextLength = 200;
constexpr int MaximumTextBoxLength = 255;
namespace Vortex {

// ===================================================================================
// WgLineEdit

enum LineEditDragType {
    DT_NOT_DRAGGING = 0,
    DT_INITIAL_DRAG,
    DT_REGULAR_DRAG,
};

WgLineEdit::~WgLineEdit() = default;

WgLineEdit::WgLineEdit(GuiContext* gui)
    : GuiWidget(gui), lineedit_show_background_(1) {
    lineedit_max_length_ = MaximumTextBoxLength;
    lineedit_blink_time_ = 0.f;
    lineedit_scroll_offset_ = 0.f;
    lineedit_drag_ = DT_NOT_DRAGGING;
    is_numerical_ = false;
    is_editable_ = true;
    force_scroll_update_ = false;
}

void WgLineEdit::deselect() {
    if (isCapturingText()) {
        stopCapturingText();
        lineedit_drag_ = DT_NOT_DRAGGING;
        lineedit_scroll_offset_ = 0.f;
    }
}

void WgLineEdit::onKeyPress(KeyPress& evt) {
    if (!isCapturingText()) return;

    evt.handled = true;

    if (evt.key == Key::RETURN) stopCapturingText();

    lineedit_drag_ = DT_NOT_DRAGGING;
    lineedit_blink_time_ = 0.f;

    Key::Code key = evt.key;
    if (evt.keyflags == Keyflag::CTRL) {
        if (key == Key::X || key == Key::C) {
            if (lineedit_cursor_.x == lineedit_cursor_.y) {
                GuiMain::setClipboardText(std::string());
            } else {
                int a = lineedit_cursor_.x, b = lineedit_cursor_.y;
                if (a > b) swapValues(a, b);
                std::string substring(lineedit_text_.data() + a, b - a);
                GuiMain::setClipboardText(substring.c_str());
                if (key == Key::X) DeleteSection();
            }
            evt.handled = true;
        } else if (key == Key::V) {
            std::string clip = GuiMain::getClipboardText();
            TextInput evt = {clip.c_str(), false};
            onTextInput(evt);
            evt.handled = true;
        } else if (key == Key::A) {
            lineedit_cursor_.x = 0;
            lineedit_cursor_.y = static_cast<int>(lineedit_text_.length());
            evt.handled = true;
        }
    } else if (evt.keyflags == Keyflag::SHIFT) {
        int& cy = lineedit_cursor_.y;
        if (key == Key::HOME) cy = 0;
        if (key == Key::END) cy = static_cast<int>(lineedit_text_.length());
        if (key == Key::LEFT) cy = Str::prevChar(lineedit_text_, cy);
        if (key == Key::RIGHT) cy = Str::nextChar(lineedit_text_, cy);
    } else {
        int &cy = lineedit_cursor_.y, &cx = lineedit_cursor_.x;
        if (key == Key::HOME) cx = cy = 0;
        if (key == Key::END)
            cx = cy = static_cast<int>(lineedit_text_.length());
        if (cx != cy) {
            if (key == Key::LEFT) cx = cy = min(cx, cy);
            if (key == Key::RIGHT) cx = cy = max(cx, cy);
        } else {
            if (key == Key::LEFT) cx = cy = Str::prevChar(lineedit_text_, cy);
            if (key == Key::RIGHT) cx = cy = Str::nextChar(lineedit_text_, cy);
        }
    }

    if (key == Key::DELETE || key == Key::BACKSPACE) {
        if (lineedit_cursor_.x == lineedit_cursor_.y) {
            if (key == Key::BACKSPACE) {
                lineedit_cursor_.y =
                    Str::prevChar(lineedit_text_, lineedit_cursor_.y);
            } else {
                lineedit_cursor_.y =
                    Str::nextChar(lineedit_text_, lineedit_cursor_.y);
            }
        }
        DeleteSection();
    }

    int len = static_cast<int>(lineedit_text_.length());
    lineedit_cursor_.x = clamp(lineedit_cursor_.x, 0, len);
    lineedit_cursor_.y = clamp(lineedit_cursor_.y, 0, len);
}

void WgLineEdit::onKeyRelease(KeyRelease& evt) {
    if (!isCapturingText()) return;
    evt.handled = true;
}

void WgLineEdit::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            if (isCapturingText() && evt.doubleClick) {
                lineedit_cursor_.x = 0;
                lineedit_cursor_.y = static_cast<int>(lineedit_text_.length());
            } else {
                if (isCapturingText()) {
                    lineedit_drag_ = DT_REGULAR_DRAG;
                } else {
                    lineedit_text_ = text.get();
                    lineedit_drag_ = DT_INITIAL_DRAG;
                    startCapturingText();
                }

                vec2i t = TextPosition();
                Text::arrange(Text::ML, lineedit_style_,
                              lineedit_text_.c_str());
                int charIndex =
                    Text::getCharIndex(vec2i{t.x, t.y}, {evt.x, evt.y});
                lineedit_cursor_.x = lineedit_cursor_.y = charIndex;
            }

            startCapturingMouse();
            lineedit_blink_time_ = 0.f;
            evt.setHandled();
        }
    } else
        deselect();
}

void WgLineEdit::onMouseRelease(MouseRelease& evt) {
    if (evt.button == Mouse::LMB && isCapturingMouse()) {
        stopCapturingMouse();
    }
    if (lineedit_drag_ != DT_NOT_DRAGGING) {
        if (is_numerical_ && lineedit_drag_ == DT_INITIAL_DRAG &&
            lineedit_cursor_.x == lineedit_cursor_.y) {
            lineedit_cursor_.x = 0;
            lineedit_cursor_.y = static_cast<int>(lineedit_text_.length());
        }
        lineedit_drag_ = DT_NOT_DRAGGING;
    }
}

void WgLineEdit::onTextInput(TextInput& evt) {
    if (isCapturingText() && is_editable_ && !evt.handled) {
        DeleteSection();
        if (static_cast<int>(lineedit_text_.length()) < lineedit_max_length_) {
            std::string input(evt.text);

            // replace newlines with spaces
            for (int i = 0; i < input.length(); ++i) {
                if (input[i] == '\n' || input[i] == '\r') {
                    char* mutableInput = const_cast<char*>(input.c_str());
                    mutableInput[i] = ' ';
                }
            }

            // truncate input
            if (input.length() > MaximumPastedTextLength) {
                Str::erase(input, MaximumPastedTextLength,
                           input.length() - MaximumPastedTextLength);
            }

            // ensure total length does not exceed max length
            int maxInputLen =
                std::min(MaximumPastedTextLength,
                         static_cast<int>(lineedit_max_length_ -
                                          lineedit_text_.length()));
            if (input.length() > maxInputLen) {
                Str::erase(input, maxInputLen, input.length() - maxInputLen);
            }

            Str::insert(lineedit_text_, lineedit_cursor_.y, input);

            lineedit_cursor_.y += static_cast<int>(input.length());
            lineedit_cursor_.x = lineedit_cursor_.y;
        }
        force_scroll_update_ = true;
    }
}

void WgLineEdit::onTextCaptureLost() {
    if (lineedit_text_ != text.get()) {
        text.set(lineedit_text_);
        onChange.call();
    }
    lineedit_cursor_.x = lineedit_cursor_.y = 0;
    lineedit_scroll_offset_ = 0.f;
    lineedit_text_.clear();
    lineedit_text_.shrink_to_fit();
}

void WgLineEdit::onTick() {
    GuiWidget::onTick();

    if (isEnabled() && (isMouseOver() || isCapturingMouse())) {
        GuiMain::setCursorIcon(Cursor::IBEAM);
    }

    if (!isCapturingText()) return;

    Text::arrange(Text::ML, lineedit_style_, lineedit_text_.c_str());

    // Update cursor position.
    vec2i tp = TextPosition();
    if (lineedit_drag_ != DT_NOT_DRAGGING) {
        vec2i mp = gui_->getMousePos();
        lineedit_cursor_.y =
            Text::getCharIndex(vec2i{tp.x, tp.y}, {mp.x, tp.y});
        lineedit_blink_time_ = 0.f;
    }
    lineedit_cursor_.x = min(max(lineedit_cursor_.x, 0),
                             static_cast<int>(lineedit_text_.length()));
    lineedit_cursor_.y = min(max(lineedit_cursor_.y, 0),
                             static_cast<int>(lineedit_text_.length()));

    // Update text offset

    float dt = gui_->getDeltaTime();
    float barW = static_cast<float>(rect_.w - 12);
    float textW = static_cast<float>(Text::getSize().x);
    float cursorX = static_cast<float>(
        Text::getCursorPos(vec2i{0, 0}, lineedit_cursor_.y).x);
    float target =
        min(max(lineedit_scroll_offset_, cursorX - barW + 12), cursorX - 12);
    target = max(0.f, min(target, textW - barW));

    float delta =
        max(fabs(lineedit_scroll_offset_ - target) * 10.f * dt, dt * 256.f);
    float smooth = (lineedit_scroll_offset_ < target)
                       ? min(lineedit_scroll_offset_ + delta, target)
                       : max(lineedit_scroll_offset_ - delta, target);
    lineedit_scroll_offset_ = force_scroll_update_ ? target : smooth;
    force_scroll_update_ = false;

    // Update blink time
    lineedit_blink_time_ = fmod(lineedit_blink_time_ + dt, 1.f);
}

void WgLineEdit::onDraw() {
    recti r = rect_;
    vec2i tp = TextPosition();
    bool active = isCapturingText();

    const char* str = active ? lineedit_text_.c_str() : text.get();

    // Draw the background box graphic.
    if (lineedit_show_background_) {
        auto& textbox = GuiDraw::getTextBox();
        textbox.base.draw(r);
        if (isCapturingText()) {
            textbox.active.draw(r);
        } else if (isMouseOver()) {
            textbox.hover.draw(r);
        }
    }

    Renderer::pushScissorRect(r.x + 3, r.y + 1, r.w - 6, r.h - 2);

    // Draw highlighted text.
    if (active && lineedit_cursor_.x != lineedit_cursor_.y) {
        std::string hlstr = Text::escapeMarkup(str);

        int cx = Text::getEscapedCharIndex(str, lineedit_cursor_.x);
        int cy = Text::getEscapedCharIndex(str, lineedit_cursor_.y);
        if (cx > cy) swapValues(cx, cy);

        Str::insert(hlstr, cy, "{tc}{bc}{sc}");
        Str::insert(hlstr, cx, "{tc:000F}{bc:FFFF}{sc:0000}");

        lineedit_style_.textFlags |= Text::MARKUP;
        lineedit_style_.textColor = Color32a(lineedit_style_.textColor, 255);
        Text::arrange(Text::ML, lineedit_style_, hlstr.c_str());
        Text::draw(tp);
        lineedit_style_.textFlags &= ~Text::MARKUP;
    }
    // Draw regular text.
    else {
        lineedit_style_.textColor =
            Color32a(lineedit_style_.textColor, isEnabled() ? 255 : 128);
        Text::arrange(Text::ML, lineedit_style_, str);
        Text::draw(tp);
    }

    // Draw the cursor position I-beam graphic.
    if (active && is_editable_ && lineedit_blink_time_ < 0.5f) {
        Text::arrange(Text::ML, lineedit_style_, str);
        Text::CursorPos pos = Text::getCursorPos(tp, lineedit_cursor_.y);
        Draw::fill({pos.x, pos.y, 1, pos.h}, Colors::white);
    }

    Renderer::popScissorRect();
}

void WgLineEdit::hideBackground() { lineedit_show_background_ = 0; }

void WgLineEdit::setMaxLength(int n) { lineedit_max_length_ = max(0, n); }

void WgLineEdit::setNumerical(bool numerical) { is_numerical_ = numerical; }

void WgLineEdit::setEditable(bool editable) { is_editable_ = editable; }

void WgLineEdit::DeleteSection() {
    if (is_editable_ && lineedit_cursor_.x != lineedit_cursor_.y) {
        if (lineedit_cursor_.x > lineedit_cursor_.y)
            swapValues(lineedit_cursor_.x, lineedit_cursor_.y);
        Str::erase(lineedit_text_, lineedit_cursor_.x,
                   lineedit_cursor_.y - lineedit_cursor_.x);
        lineedit_cursor_.y = lineedit_cursor_.x;
        force_scroll_update_ = true;
    }
}

vec2i WgLineEdit::TextPosition() const {
    recti r = rect_;
    return {r.x - static_cast<int>(lineedit_scroll_offset_) + 6, r.y + r.h / 2};
}

// ================================================================================================
// WgSpinner

WgSpinner::~WgSpinner() { delete spinner_lineedit_; }

WgSpinner::WgSpinner(GuiContext* gui) : GuiWidget(gui) {
    spinner_is_up_pressed_ = false;
    spinner_repeat_timer_ = 0.f;
    spinner_min_ = INT_MIN;
    spinner_max_ = INT_MAX;
    spinner_step_size_ = 1.0;
    spinner_min_decimal_places_ = 0;
    spinner_max_decimal_places_ = 6;
    SpinnerUpdateValue(0.0);

    spinner_lineedit_ = new WgLineEdit(gui_);
    spinner_lineedit_->setNumerical(true);
    spinner_lineedit_->setMaxLength(12);
    spinner_lineedit_->text.bind(&spinner_text_);
    spinner_lineedit_->onChange.bind(this, &WgSpinner::SpinnerOnTextChange);
    spinner_lineedit_->hideBackground();
}

void WgSpinner::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            spinner_lineedit_->deselect();
            startCapturingMouse();
            recti r = SpinnerButtonRect();
            spinner_is_up_pressed_ = (evt.y <= r.y + r.h / 2);
            double sign = spinner_is_up_pressed_ ? 1.0 : -1.0;
            SpinnerUpdateValue(value.get() + spinner_step_size_ * sign);
            spinner_repeat_timer_ = 0.5f;
        }
        evt.setHandled();
    }
}

void WgSpinner::onMouseRelease(MouseRelease& evt) {
    if (evt.button == Mouse::LMB && isCapturingMouse()) {
        stopCapturingMouse();
    }
}

void WgSpinner::onArrange(recti r) {
    GuiWidget::onArrange(r);
    r.w -= 14;
    spinner_lineedit_->arrange(r);
}

void WgSpinner::onTick() {
    vec2i mpos = gui_->getMousePos();
    if (IsInside(SpinnerButtonRect(), mpos.x, mpos.y)) {
        captureMouseOver();
    }

    spinner_lineedit_->tick();

    GuiWidget::onTick();
    if (isMouseOver() || spinner_lineedit_->isMouseOver()) {
        GuiMain::setTooltip(getTooltip());
    }

    // Propagate settings to the text edits.
    spinner_lineedit_->setEnabled(isEnabled());

    // Check if the user is pressing the increment/decrement button.
    if (isCapturingMouse()) {
        spinner_repeat_timer_ -= gui_->getDeltaTime();
        if (spinner_repeat_timer_ <= 0.f) {
            double sign = spinner_is_up_pressed_ ? 1.0 : -1.0;
            SpinnerUpdateValue(value.get() + spinner_step_size_ * sign);
            spinner_repeat_timer_ = 0.05f;
        }
    }

    // Check if the text display is still up to date.
    double val = value.get();
    if (spinner_display_value_ != val) SpinnerUpdateText();
}

void WgSpinner::onDraw() {
    auto& button = GuiDraw::getButton();
    auto& icons = GuiDraw::getIcons();
    auto& textbox = GuiDraw::getTextBox();

    // Draw the background box graphic.
    recti rtext = SideL(rect_, rect_.w - 12);
    textbox.base.draw(rtext, TileRect2::L);
    if (spinner_lineedit_->isCapturingText()) {
        rtext.w -= 2;
        textbox.active.draw(rtext, TileRect2::L);
    } else if (spinner_lineedit_->isMouseOver()) {
        rtext.w -= 2;
        textbox.hover.draw(rtext, TileRect2::L);
    }
    spinner_lineedit_->draw();

    // Draw the buttons.
    recti r = SpinnerButtonRect();
    button.base.draw(r, TileRect2::R);

    if (isCapturingMouse()) {
        if (spinner_is_up_pressed_) {
            recti top = {r.x, r.y, r.w, r.h / 2 + 1};
            button.pressed.draw(top, TileRect2::TR);
        } else {
            recti btm = {r.x, r.y + r.h / 2 - 1, r.w, r.h / 2 + 1};
            button.pressed.draw(btm, TileRect2::BR);
        }
    } else if (isEnabled() && isMouseOver()) {
        if (gui_->getMousePos().y < r.y + r.h / 2) {
            recti top = {r.x, r.y, r.w, r.h / 2 + 1};
            button.hover.draw(top, TileRect2::TR);
        } else {
            recti btm = {r.x, r.y + r.h / 2 - 1, r.w, r.h / 2 + 1};
            button.hover.draw(btm, TileRect2::BR);
        }
    }

    uint32_t col = Color32(isEnabled() ? 255 : 128);

    Draw::sprite(icons.arrow, {r.x + r.w / 2, r.y + r.h * 1 / 4}, col,
                 Draw::ROT_90 | Draw::FLIP_H);
    Draw::sprite(icons.arrow, {r.x + r.w / 2, r.y + r.h * 3 / 4}, col,
                 Draw::ROT_90);
}

void WgSpinner::setRange(double min, double max) {
    spinner_min_ = min, spinner_max_ = max;
}

void WgSpinner::setStep(double step) { spinner_step_size_ = step; }

void WgSpinner::setPrecision(int minDecimalPlaces, int maxDecimalPlaces) {
    spinner_min_decimal_places_ = minDecimalPlaces;
    spinner_max_decimal_places_ = maxDecimalPlaces;
    SpinnerUpdateText();
}

void WgSpinner::SpinnerUpdateValue(double v) {
    double prev = value.get();
    value.set(max(spinner_min_, min(spinner_max_, v)));
    SpinnerUpdateText();
    if (value.get() != prev) onChange.call();
}

void WgSpinner::SpinnerUpdateText() {
    spinner_display_value_ = value.get();
    spinner_text_.clear();
    Str::appendVal(spinner_text_, spinner_display_value_,
                   spinner_min_decimal_places_, spinner_max_decimal_places_);
}

void WgSpinner::SpinnerOnTextChange() {
    double v = 0.0;
    if (Str::parse(spinner_text_.c_str(), v)) {
        SpinnerUpdateValue(v);
    } else {
        SpinnerUpdateText();
    }
}

recti WgSpinner::SpinnerButtonRect() { return SideR(rect_, 14); }

};  // namespace Vortex
