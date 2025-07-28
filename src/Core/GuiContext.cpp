#include <Core/GuiContext.h>
#include <Core/GuiWidget.h>
#include <Core/GuiDialog.h>
#include <Core/MapUtils.h>
#include <Core/VectorUtils.h>

namespace Vortex {

GuiContext::~GuiContext() {}

GuiContextImpl::~GuiContextImpl() { Vec::release(dialogs_); }

GuiContextImpl::GuiContextImpl() {
    view_rect_ = {0, 0, INT_MAX, INT_MAX};
    input_events_ = nullptr;
}

recti GuiContextImpl::getView() { return view_rect_; }

vec2i GuiContextImpl::getMousePos() { return mouse_position_; }

float GuiContextImpl::getDeltaTime() { return delta_time_; }

InputEvents& GuiContextImpl::getEvents() { return *input_events_; }

void GuiContextImpl::tick(recti view, float deltaTime, InputEvents& events) {
    view_rect_ = view;

    view_rect_.w = max(view_rect_.w, 0);
    view_rect_.h = max(view_rect_.h, 0);

    delta_time_ = deltaTime;
    input_events_ = &events;

    // Update the mouse position.
    for (MouseMove* move = nullptr; events.next(move);) {
        mouse_position_ = {move->x, move->y};
    }

    // Rearrange or delete dialogs if requested.
    FOR_VECTOR_REVERSE(dialogs_, i) {
        auto dialog = dialogs_[i];
        if (dialog->request_close_) {
            dialogs_.erase_values(dialog);
            delete dialog;
        } else if (dialog->request_move_to_top_) {
            dialogs_.erase(i);
            dialogs_.push_back(dialog);
            dialog->request_move_to_top_ = 0;
        }
    }

    // Arrange the dialogs and widgets.
    FOR_VECTOR_REVERSE(dialogs_, i) { dialogs_[i]->arrange(); }

    // Tick widgets with focus first.
    FOR_VECTOR_REVERSE(focus_widgets_, i) {
        if (focus_widgets_[i]->isEnabled()) {
            focus_widgets_[i]->onTick();
        }
    }

    // Tick the dialogs.
    FOR_VECTOR_REVERSE(dialogs_, i) { dialogs_[i]->tick(); }

    input_events_ = nullptr;
}

void GuiContextImpl::draw() {
    // Draw the dialogs.
    FOR_VECTOR_FORWARD(dialogs_, i) { dialogs_[i]->draw(); }

    // Draw widgets with focus at the top.
    FOR_VECTOR_FORWARD(focus_widgets_, i) { focus_widgets_[i]->onDraw(); }
}

void GuiContextImpl::removeWidget(GuiWidget* w) {
    focus_widgets_.erase_values(w);
}

void GuiContextImpl::addDialog(DialogData* f) { dialogs_.push_back(f); }

void GuiContextImpl::removeDialog(DialogData* f) { dialogs_.erase_values(f); }

void GuiContextImpl::grabFocus(GuiWidget* w) {
    FOR_VECTOR_FORWARD(focus_widgets_, i) {
        if (focus_widgets_[i] == w) return;
    }
    focus_widgets_.push_back(w);
}

void GuiContextImpl::releaseFocus(GuiWidget* w) {
    focus_widgets_.erase_values(w);
}

// ================================================================================================
// GuiContextImpl :: slot bindings.

bool GuiContextImpl::bind(const std::string& slot, const int* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const uint32_t* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const long* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const uint64_t* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const float* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const double* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const bool* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const char* str) {
    auto s = Map::findVal(text_slots_, slot);
    if (s) (*s)->bind(str);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, const std::string* str) {
    auto s = Map::findVal(text_slots_, slot);
    if (s) (*s)->bind(str);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, int* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, uint32_t* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, long* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, uint64_t* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, float* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, double* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, bool* v) {
    auto s = Map::findVal(value_slots_, slot);
    if (s) (*s)->bind(v);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, std::string* str) {
    auto s = Map::findVal(text_slots_, slot);
    if (s) (*s)->bind(str);
    return s != nullptr;
}

bool GuiContextImpl::bind(const std::string& slot, Functor::Generic* f) {
    auto s = Map::findVal(call_slots_, slot);
    if (s)
        (*s)->bind(f);
    else
        delete f;
    return s != nullptr;
}

void GuiContextImpl::addSlot(ValueSlot* slot, const char* name) {
    value_slots_.insert({name, slot});
}

void GuiContextImpl::addSlot(TextSlot* slot, const char* name) {
    text_slots_.insert({name, slot});
}

void GuiContextImpl::addSlot(CallSlot* slot, const char* name) {
    call_slots_.insert({name, slot});
}

void GuiContextImpl::removeSlot(ValueSlot* slot) {
    Map::eraseVals(value_slots_, slot);
}

void GuiContextImpl::removeSlot(TextSlot* slot) {
    Map::eraseVals(text_slots_, slot);
}

void GuiContextImpl::removeSlot(CallSlot* slot) {
    Map::eraseVals(call_slots_, slot);
}

GuiContext* GuiContext::create() { return new GuiContextImpl; }

};  // namespace Vortex
