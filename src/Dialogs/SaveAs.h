#pragma once

#include <string>

#include <Core/Widgets.h>
#include <Dialogs/Dialog.h>

#include <Simfile/Simfile.h>

namespace Vortex {

/// Asks which format the simfile is saved in, and shows the name that format
/// wants. Picking a format rewrites the name, which is still free to edit.
class DialogSaveAs : public EditorDialog {
   public:
    ~DialogSaveAs();
    DialogSaveAs();

   private:
    void onFormatChanged();
    void onSave();
    void onCancel();

    /// Fills the name field with the name the current format asks for.
    void myUpdateName();

    WgLineEdit* myNameField;

    int myFormat;
    std::string myName;
};

};  // namespace Vortex
