#include <Core/GuiWidget.h>

#include <Core/GuiContext.h>
#include <Core/GuiManager.h>

#include <System/File.h>

namespace Vortex {

#define MY_GUI ((GuiContextImpl*)gui_)

// ================================================================================================
// GuiWidget :: constructor / destructor.

GuiContext* gui_;
recti rect_;
int width_;
int height_;
uint32_t flags_;

GuiWidget::GuiWidget(GuiContext* gui)
    : gui_(gui),
      rect_({0, 0, 128, 24}),
      width_(128),
      height_(24),
      flags_(WF_ENABLED) {}

GuiWidget::~GuiWidget() {
    MY_GUI->removeWidget(this);
    GuiManager::removeWidget(this);
}

// ================================================================================================
// GuiWidget :: input handling.

void GuiWidget::captureMouseOver() { GuiManager::makeMouseOver(this); }

void GuiWidget::blockMouseOver() { GuiManager::blockMouseOver(this); }

void GuiWidget::unblockMouseOver() { GuiManager::unblockMouseOver(this); }

bool GuiWidget::isMouseOver() const {
    return GuiManager::getMouseOver() == this;
}

void GuiWidget::startCapturingMouse() { GuiManager::startCapturingMouse(this); }

void GuiWidget::stopCapturingMouse() { GuiManager::stopCapturingMouse(this); }

bool GuiWidget::isCapturingMouse() const {
    return GuiManager::getMouseCapture() == this;
}

void GuiWidget::startCapturingText() { GuiManager::captureText(this); }

void GuiWidget::stopCapturingText() { GuiManager::stopCapturingText(this); }

bool GuiWidget::isCapturingText() const {
    return GuiManager::getTextCapture() == this;
}

void GuiWidget::onMouseCaptureLost() {}

void GuiWidget::onTextCaptureLost() {}

void GuiWidget::startCapturingFocus() {
    MY_GUI->grabFocus(this);
    SetFlags(flags_, WF_IN_FOCUS, true);
}

void GuiWidget::stopCapturingFocus() {
    MY_GUI->releaseFocus(this);
    SetFlags(flags_, WF_IN_FOCUS, false);
}

bool GuiWidget::isCapturingFocus() const {
    return HasFlags(flags_, WF_IN_FOCUS);
}

// ================================================================================================
// GuiWidget :: update functions.

void GuiWidget::updateSize() { onUpdateSize(); }

void GuiWidget::onUpdateSize() {}

void GuiWidget::arrange(recti r) { onArrange(r); }

void GuiWidget::onArrange(recti r) { rect_ = r; }

void GuiWidget::tick() {
    if (!isCapturingFocus()) {
        onTick();
    }
}

void GuiWidget::draw() {
    if (!isCapturingFocus()) {
        onDraw();
    }
}

void GuiWidget::onTick() {
    if (HasFlags(flags_, WF_ENABLED)) {
        vec2i mpos = gui_->getMousePos();
        if (IsInside(rect_, mpos.x, mpos.y)) {
            captureMouseOver();
        }
        handleInputs(gui_->getEvents());
        if (isMouseOver()) {
            GuiMain::setTooltip(getTooltip());
        }
    }
}

void GuiWidget::onDraw() {}

// ================================================================================================
// GuiWidget :: get / set functions.

void GuiWidget::setId(const std::string& id) {
    GuiManager::setWidgetId(this, id);
}

void GuiWidget::setWidth(int w) { width_ = w; }

void GuiWidget::setHeight(int h) { height_ = h; }

void GuiWidget::setSize(int w, int h) {
    width_ = w;
    height_ = h;
}

int GuiWidget::getWidth() const { return width_; }

int GuiWidget::getHeight() const { return height_; }

vec2i GuiWidget::getSize() const { return {width_, height_}; }

recti GuiWidget::getRect() const { return rect_; }

std::string GuiWidget::getTooltip() const {
    return GuiManager::getTooltip(this);
}

void GuiWidget::setEnabled(bool value) { SetFlags(flags_, WF_ENABLED, value); }

void GuiWidget::setTooltip(const std::string& text) {
    GuiManager::setTooltip(this, text);
}

bool GuiWidget::isEnabled() const { return HasFlags(flags_, WF_ENABLED); }

GuiContext* GuiWidget::getGui() const { return gui_; }

};  // namespace Vortex
