#pragma once

#include <Dialogs/Dialog.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>
#include <Editor/StreamGenerator.h>

namespace Vortex {

class DialogGenerateNotes : public EditorDialog, public InputHandler {
   public:
    ~DialogGenerateNotes();
    DialogGenerateNotes();

    void onChanges(int changes) override;

   private:
    void myCreateWidgets();
    void myGenerateNotes();

    StreamGenerator streamGenerator_;
    WgDroplist* spacingDroplist_;
    int footSelectionIndex_;
    int spacingValue_;
};

};  // namespace Vortex
