#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Draw.h>

namespace Vortex {

class DialogTempoBreakdown : public EditorDialog {
   public:
    void onUpdateSize() override;
    void onTick() override;
    void onDraw() override;

    ~DialogTempoBreakdown();
    DialogTempoBreakdown();

   private:
    struct TempoList;
    TempoList* myList;
};

};  // namespace Vortex
