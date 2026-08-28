#pragma once

#include <Core/Widgets.h>
#include <System/System.h>

#include <climits>

namespace Vortex {

class DialogData : public GuiWidget {
   public:
    enum ActionType {
        ACT_NONE,
        ACT_DRAG,
        ACT_CLOSE,
        ACT_MINIMIZE,
        ACT_PIN,
        ACT_RESIZE,
        ACT_RESIZE_L,
        ACT_RESIZE_T,
        ACT_RESIZE_R,
        ACT_RESIZE_B,
        ACT_RESIZE_BL,
        ACT_RESIZE_BR,
        ACT_RESIZE_TL,
        ACT_RESIZE_TR,
    };
    struct BaseAction {
        ActionType type;
    };
    struct DragAction : public BaseAction {
        vec2i offset;
    };
    struct ResizeAction : public BaseAction {
        vec2i anchor, offset;
        int dirH, dirV;
    };

    DialogData(GuiContext* gui, GuiDialog* dialog);
    ~DialogData();

    void onMousePress(MousePress& evt) override;
    void onMouseRelease(MouseRelease& evt) override;

    void arrange();
    void tick();
    void draw();

    GuiDialog* dialog_ptr_;
    GuiContext* gui_;

    bool is_pinnable_ : 1 = true;
    bool is_closeable_ : 1 = true;
    bool is_minimizable_ : 1 = true;
    bool is_draggable_ : 1 = true;
    bool is_horizontally_resizable_ : 1 = false;
    bool is_vertically_resizable_ : 1 = false;

    bool request_close_ : 1 = false;
    bool request_pin_ : 1 = false;
    bool request_minimize_ : 1 = false;
    bool request_move_to_top_ : 1 = false;

    bool pinned_state_ : 1 = false;
    bool minimized_state_ : 1 = false;

   private:
    friend class GuiDialog;

    void ClampRect();
    void HandleResize();
    void HandleDrag();
    void UpdateMouseCursor();

    ActionType GetAction(int x, int y) const;
    void FinishActions();

    vec2i min_size_ = {0, 0};
    vec2i max_size_ = {INT_MAX, INT_MAX};
    vec2i pinned_position_ = {0, 0};

    std::string dialog_title_;

    BaseAction* current_action_ = nullptr;
};

};  // namespace Vortex
