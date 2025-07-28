#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogCustomSnap : public EditorDialog, public InputHandler {
   public:
    ~DialogCustomSnap();
    DialogCustomSnap();

    void onChange();

   private:
    void myCreateWidgets();

    int myCustomSnap;
};

};  // namespace Vortex
#pragma once
