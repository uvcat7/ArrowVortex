#pragma once

#include <Core/Utils.h>
#include <Core/GuiManager.h>
#include <Core/GuiDraw.h>
#include <Core/GuiDialog.h>

#include <map>
#include <vector>

namespace Vortex {

// ================================================================================================
// Gui context.

class GuiContextImpl : public GuiContext {
   public:
    GuiContextImpl();
    ~GuiContextImpl() override;

    void tick(recti view, float deltaTime, InputEvents& events) override;
    void draw() override;

    void closeDialogs() override;

    recti getView() override;
    vec2i getMousePos() override;
    float getDeltaTime() override;
    InputEvents& getEvents() override;

    bool bind(const std::string& slot, const int* v) override;
    bool bind(const std::string& slot, const uint32_t* v) override;
    bool bind(const std::string& slot, const long* v) override;
    bool bind(const std::string& slot, const uint64_t* v) override;
    bool bind(const std::string& slot, const float* v) override;
    bool bind(const std::string& slot, const double* v) override;
    bool bind(const std::string& slot, const bool* v) override;
    bool bind(const std::string& slot, const char* str) override;
    bool bind(const std::string& slot, const std::string* str) override;

    bool bind(const std::string& slot, int* v) override;
    bool bind(const std::string& slot, uint32_t* v) override;
    bool bind(const std::string& slot, long* v) override;
    bool bind(const std::string& slot, uint64_t* v) override;
    bool bind(const std::string& slot, float* v) override;
    bool bind(const std::string& slot, double* v) override;
    bool bind(const std::string& slot, bool* v) override;
    bool bind(const std::string& slot, std::string* str) override;
    bool bind(const std::string& slot, Functor::Generic* f) override;

    void addSlot(ValueSlot* slot, const char* name);
    void addSlot(TextSlot* slot, const char* name);
    void addSlot(CallSlot* slot, const char* name);

    void removeSlot(ValueSlot* slot);
    void removeSlot(TextSlot* slot);
    void removeSlot(CallSlot* slot);

    void removeWidget(GuiWidget* w);

    void addDialog(DialogData* f);
    void removeDialog(DialogData* f);

    void grabFocus(GuiWidget* w);
    void releaseFocus(GuiWidget* w);

   private:
    std::map<std::string, ValueSlot*> value_slots_;
    std::map<std::string, TextSlot*> text_slots_;
    std::map<std::string, CallSlot*> call_slots_;

    recti view_rect_;
    vec2i mouse_position_;
    float delta_time_;
    InputEvents* input_events_;

    std::vector<DialogData*> dialogs_;
    std::vector<GuiWidget*> focus_widgets_;
};

};  // namespace Vortex
