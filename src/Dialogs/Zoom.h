#pragma once

#include <Dialogs/Dialog.h>

#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogZoom : public EditorDialog {
   public:
    ~DialogZoom();
    DialogZoom();

    void onTick() override;
    void onAction(int id);

   private:
    void myCreateWidgets();

    double myZoomLevel, myScaleLevel;
};

};  // namespace Vortex
