#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Draw.h>
#include <Core/WidgetsLayout.h>

namespace Vortex {

class DialogChartList : public EditorDialog {
   public:
    void onChanges(int changes) override;
    void onUpdateSize() override;
    void onTick() override;
    void onDraw() override;

    ~DialogChartList();
    DialogChartList();

   private:
    struct ChartButton;
    struct ChartList;
    ChartList* myList;
};

};  // namespace Vortex
