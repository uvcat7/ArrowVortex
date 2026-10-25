#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogJumpTo : public EditorDialog, public InputHandler {
   public:
    ~DialogJumpTo();
    DialogJumpTo();

    void onJump();

   private:
    void myCreateWidgets();

    std::string myJumpToTarget;
    int myTargetType;
    WgSelectList* myTypeList;
};

};  // namespace Vortex
#pragma once
