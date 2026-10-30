#pragma once

#include <Dialogs/Dialog.h>

namespace Vortex {

class DialogPreviewSettings : public EditorDialog {
   public:
    ~DialogPreviewSettings();
    DialogPreviewSettings();

    void onTick() override;
    void onAction(int id);

   private:
    void myCreateWidgets();

    bool isEnabled_;
    int drawMode_;
    int cmod_;
    double xmod_;
    bool isBeatLines_;
    bool isReverse_;
    bool isFakeNotes_;
    bool isFakeRegions_;
    bool isCModEnabled_;
    bool isXModEnabled_;
    bool isNotesCropped_;
};

};  // namespace Vortex
