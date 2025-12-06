#include <Core/Widgets.h>

#include <Core/Xmr.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>

namespace Vortex {

// ================================================================================================
// WgSeperator

WgSeperator::~WgSeperator() = default;

WgSeperator::WgSeperator(GuiContext* gui) : GuiWidget(gui) { height_ = 2; }

void WgSeperator::onDraw() {
    recti r = rect_;
    Draw::fill({r.x, r.y, r.w, r.h / 2}, Color32(13));
    Draw::fill({r.x, r.y + r.h / 2, r.w, r.h / 2}, Color32(77));
}

// ================================================================================================
// WgLabel

WgLabel::~WgLabel() = default;

WgLabel::WgLabel(GuiContext* gui) : GuiWidget(gui) {}

void WgLabel::onDraw() {
    recti r = rect_;

    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    if (!isEnabled()) style.textColor = Color32(166);

    Text::arrange(Text::MC, style, text.get());
    Text::draw({r.x + 2, r.y, r.w - 4, r.h});
}

// ================================================================================================
// WgButton

WgButton::~WgButton() = default;

WgButton::WgButton(GuiContext* gui) : GuiWidget(gui) {}

void WgButton::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            startCapturingMouse();
            isDown.set(true);
            counter.set(counter.get() + 1);
            onPress.call();
        }
        evt.setHandled();
    }
}

void WgButton::onMouseRelease(MouseRelease& evt) {
    stopCapturingMouse();
    isDown.set(false);
}

void WgButton::onDraw() {
    auto& button = GuiDraw::getButton();

    // Draw the button graphic.
    button.base.draw(rect_);

    // Draw the button text.
    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    if (!isEnabled()) style.textColor = GuiDraw::getMisc().colDisabled;

    recti r = rect_;
    Renderer::pushScissorRect(Shrink(rect_, 2));
    Text::arrange(Text::MC, style, text.get());
    Text::draw({r.x + 2, r.y, r.w - 4, r.h});
    Renderer::popScissorRect();

    // Draw interaction effects.
    if (isCapturingMouse()) {
        button.pressed.draw(rect_);
    } else if (isMouseOver()) {
        button.hover.draw(rect_);
    }
}

// ================================================================================================
// WgCheckbox

WgCheckbox::~WgCheckbox() = default;

WgCheckbox::WgCheckbox(GuiContext* gui) : GuiWidget(gui) {}

void WgCheckbox::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            startCapturingMouse();
            value.set(!value.get());
            onChange.call();
        }
        evt.setHandled();
    }
}

void WgCheckbox::onMouseRelease(MouseRelease& evt) {
    if (isCapturingMouse() && evt.button == Mouse::LMB) {
        stopCapturingMouse();
    }
}

void WgCheckbox::onDraw() {
    auto& icons = GuiDraw::getIcons();
    auto& textbox = GuiDraw::getTextBox();
    auto& misc = GuiDraw::getMisc();

    // Draw the checkbox graphic.
    recti box = GetCheckboxRect();
    bool checked = value.get();
    textbox.base.draw(box);
    if (checked)
        Draw::sprite(icons.check, {box.x + box.w / 2, box.y + box.h / 2});

    // Draw the description text.
    recti r = rect_;

    TextStyle style;
    style.textFlags = Text::MARKUP | Text::ELLIPSES;
    if (!isEnabled()) style.textColor = misc.colDisabled;

    Text::arrange(Text::ML, style, text.get());
    Text::draw({r.x + 22, r.y, r.w - 24, r.h});
}

recti WgCheckbox::GetCheckboxRect() const {
    recti r = rect_;
    return {r.x + 2, r.y + r.h / 2 - 8, 16, 16};
}

// ================================================================================================
// WgSlider

WgSlider::~WgSlider() = default;

WgSlider::WgSlider(GuiContext* gui) : GuiWidget(gui) {
    slider_begin_ = 0.0;
    slider_end_ = 1.0;
}

void WgSlider::setRange(double begin, double end) {
    slider_begin_ = begin, slider_end_ = end;
}

void WgSlider::onMousePress(MousePress& evt) {
    if (isMouseOver()) {
        if (isEnabled() && evt.button == Mouse::LMB && evt.unhandled()) {
            startCapturingMouse();
            SliderDrag(evt.x, evt.y);
        }
        evt.setHandled();
    }
}

void WgSlider::onMouseRelease(MouseRelease& evt) {
    if (evt.button == Mouse::LMB && isCapturingMouse()) {
        SliderDrag(evt.x, evt.y);
        stopCapturingMouse();
    }
}

void WgSlider::onTick() {
    GuiWidget::onTick();

    if (isCapturingMouse()) {
        vec2i mpos = gui_->getMousePos();
        SliderDrag(mpos.x, mpos.y);
    }
}

void WgSlider::onDraw() {
    recti r = rect_;

    auto& button = GuiDraw::getButton();

    // Draw the the entire bar graphic.
    recti bar = {r.x + 3, r.y + r.h / 2 - 1, r.w - 6, 1};
    Draw::fill(bar, Color32(0, 255));
    bar.y += 1;
    Draw::fill(bar, Color32(77, 255));

    // Draw the draggable button graphic.
    if (slider_begin_ != slider_end_) {
        int boxX = static_cast<int>(static_cast<double>(bar.w) *
                                    (value.get() - slider_begin_) /
                                    (slider_end_ - slider_begin_));
        recti box = {bar.x + min(max(boxX, 0), bar.w) - 4, bar.y - 8, 8, 16};

        button.base.draw(box, 0);
        if (isCapturingMouse()) {
            button.pressed.draw(box, 0);
        } else if (isMouseOver()) {
            button.hover.draw(box, 0);
        }
    }
}

void WgSlider::SliderUpdateValue(double v) {
    double prev = value.get();
    v = min(v, max(slider_begin_, slider_end_));
    v = max(v, min(slider_begin_, slider_end_));
    value.set(v);
    if (value.get() != prev) onChange.call();
}

void WgSlider::SliderDrag(int x, int y) {
    recti r = rect_;
    r.w = max(r.w, 1);
    double val = slider_begin_ +
                 (slider_end_ - slider_begin_) *
                     (static_cast<double>(x - r.x) / static_cast<double>(r.w));
    SliderUpdateValue(val);
}

};  // namespace Vortex
